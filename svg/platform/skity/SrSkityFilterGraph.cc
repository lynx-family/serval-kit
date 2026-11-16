// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/skity/SrSkityFilterGraph.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "platform/skity/SrSkityMatrixCanvas.h"
#include "skity/effect/color_filter.hpp"
#include "skity/effect/image_filter.hpp"

namespace serval {
namespace svg {
namespace skity {

std::shared_ptr<::skity::ColorFilter> BuildSkityColorFilter(
    const canvas::SrFilterPrimitiveModel& primitive);

namespace {
constexpr int kSourceGraphic = -1;
constexpr int kSourceAlpha = -2;
constexpr size_t kMaxPrimitives = 128;
constexpr size_t kMaxReplayVisits = 256;
constexpr size_t kMaxSourceReplays = 16;
constexpr size_t kMaxLayerDepth = 64;
constexpr double kMaxLayerBytes = 64.0 * 1024.0 * 1024.0;

::skity::Rect Rect(const SrSVGBox& box) {
  return ::skity::Rect::MakeXYWH(box.left, box.top, box.width, box.height);
}

bool FiniteBox(const SrSVGBox& box) {
  return std::isfinite(box.left) && std::isfinite(box.top) &&
         std::isfinite(box.width) && std::isfinite(box.height) &&
         std::isfinite(box.left + box.width) &&
         std::isfinite(box.top + box.height);
}

bool StandardName(const std::string& name) {
  return name == "SourceGraphic" || name == "SourceAlpha" ||
         name == "BackgroundImage" || name == "BackgroundAlpha" ||
         name == "FillPaint" || name == "StrokePaint";
}

bool CompositeMode(const std::string& value, ::skity::BlendMode* mode) {
  using Mode = ::skity::BlendMode;
  if (value == "over")
    *mode = Mode::kSrcOver;
  else if (value == "in")
    *mode = Mode::kSrcIn;
  else if (value == "out")
    *mode = Mode::kSrcOut;
  else if (value == "atop")
    *mode = Mode::kSrcATop;
  else if (value == "xor")
    *mode = Mode::kXor;
  else
    return false;
  return true;
}

std::shared_ptr<::skity::ColorFilter> AlphaMatrix(float scale, float bias) {
  const float matrix[20] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, scale, bias,
  };
  return ::skity::ColorFilters::Matrix(matrix);
}

std::shared_ptr<::skity::ColorFilter> AlphaToOpaqueRGB(float scale,
                                                       float bias) {
  const float matrix[20] = {
      0, 0, 0, scale, bias, 0, 0, 0, scale, bias,
      0, 0, 0, scale, bias, 0, 0, 0, 0,     1,
  };
  return ::skity::ColorFilters::Matrix(matrix);
}

std::shared_ptr<::skity::ColorFilter> InvertedRedToAlpha() {
  const float matrix[20] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 1,
  };
  return ::skity::ColorFilters::Matrix(matrix);
}

bool MatrixAlphaOnly(const canvas::SrFilterPrimitiveModel& primitive,
                     bool input_alpha_only) {
  if (primitive.color_matrix_type == "luminanceToAlpha")
    return true;
  const auto& values = primitive.color_matrix_values;
  for (size_t row = 0; row < 3; ++row) {
    for (size_t column = 0; column < 5; ++column) {
      if (input_alpha_only && column < 3)
        continue;
      if (values[row * 5 + column] != 0.f)
        return false;
    }
  }
  return true;
}
}  // namespace

bool SrSkityFilterGraph::CombineCosts(const Cost& outer, const Cost& inner,
                                      Cost* combined) {
  // A child is expanded into its parent's recorded source. Bound the whole
  // active chain, including linear layers, before starting another capture.
  if (!combined || outer.source_replays == 0 || inner.source_replays == 0 ||
      outer.source_replays > kMaxSourceReplays ||
      inner.source_replays > kMaxSourceReplays / outer.source_replays ||
      outer.visits > kMaxReplayVisits ||
      inner.visits > (kMaxReplayVisits - outer.visits) / outer.source_replays ||
      outer.depth > kMaxLayerDepth ||
      inner.depth > kMaxLayerDepth - outer.depth ||
      !std::isfinite(outer.workspace_bytes) || outer.workspace_bytes < 0 ||
      !std::isfinite(inner.workspace_bytes) || inner.workspace_bytes < 0 ||
      outer.workspace_bytes + inner.workspace_bytes > kMaxLayerBytes) {
    return false;
  }
  *combined = {outer.source_replays * inner.source_replays,
               outer.visits + outer.source_replays * inner.visits,
               outer.depth + inner.depth,
               outer.workspace_bytes + inner.workspace_bytes};
  return true;
}

std::unique_ptr<SrSkityFilterGraph> SrSkityFilterGraph::Compile(
    const canvas::SrFilterModel& model, const ::skity::Matrix& transform,
    float content_scale) {
  if (!FiniteBox(model.region) || model.region.width <= 0.f ||
      model.region.height <= 0.f || model.primitives.empty() ||
      model.primitives.size() > kMaxPrimitives || !transform.IsFinite() ||
      transform.HasPersp() || !std::isfinite(content_scale) ||
      content_scale <= 0.f) {
    return nullptr;
  }
  ::skity::Matrix inverse;
  if (!transform.Invert(&inverse))
    return nullptr;
  auto graph = std::unique_ptr<SrSkityFilterGraph>(new SrSkityFilterGraph());
  graph->model_ = model;
  std::unordered_map<std::string, int> names{{"SourceGraphic", kSourceGraphic},
                                             {"SourceAlpha", kSourceAlpha}};
  auto resolve = [&names](const std::string& name, int* index) {
    auto found = names.find(name);
    if (found == names.end())
      return false;
    *index = found->second;
    return true;
  };
  auto alpha_only = [&graph](int index) {
    return index == kSourceAlpha ||
           (index >= 0 && graph->nodes_[index].alpha_only);
  };
  auto accumulate = [&graph](Node* node, int input) {
    if (input < 0) {
      ++node->visits;
      ++node->sources;
      node->depth = std::max(node->depth, size_t{3});
    } else {
      const auto& dependency = graph->nodes_[input];
      node->visits += dependency.visits;
      node->sources += dependency.sources;
      node->depth = std::max(node->depth, dependency.depth + 2);
    }
  };
  for (const auto& primitive : model.primitives) {
    if (!FiniteBox(primitive.subregion) || primitive.result.empty() ||
        StandardName(primitive.result))
      return nullptr;
    Node node;
    node.primitive = primitive;
    node.sources = 0;
    if (primitive.type != canvas::SrFilterPrimitiveType::kFlood) {
      if (!resolve(primitive.input, &node.first))
        return nullptr;
      accumulate(&node, node.first);
      node.alpha_only = alpha_only(node.first);
    }
    using Type = canvas::SrFilterPrimitiveType;
    switch (primitive.type) {
      case Type::kGaussianBlur:
        if (!std::isfinite(primitive.std_deviation_x) ||
            !std::isfinite(primitive.std_deviation_y))
          return nullptr;
        // An invalid finite negative deviation disables this primitive, not
        // the remaining filter chain. Retain identity-blur clipping behavior.
        if (primitive.std_deviation_x < 0.f ||
            primitive.std_deviation_y < 0.f) {
          node.primitive.std_deviation_x = 0.f;
          node.primitive.std_deviation_y = 0.f;
        }
        break;
      case Type::kOffset:
        if (!std::isfinite(primitive.dx) || !std::isfinite(primitive.dy))
          return nullptr;
        break;
      case Type::kColorMatrix:
        if (primitive.color_matrix_type != "luminanceToAlpha") {
          if (primitive.color_matrix_type != "matrix" ||
              primitive.color_matrix_values.size() != 20)
            return nullptr;
          for (float value : primitive.color_matrix_values)
            if (!std::isfinite(value))
              return nullptr;
        }
        node.alpha_only = MatrixAlphaOnly(primitive, node.alpha_only);
        break;
      case Type::kFlood:
        if (!std::isfinite(primitive.flood_opacity))
          return nullptr;
        node.alpha_only = (primitive.flood_color & 0x00ffffff) == 0 ||
                          primitive.flood_opacity <= 0.f;
        break;
      case Type::kComposite:
      case Type::kBlend: {
        if (!resolve(primitive.input2, &node.second))
          return nullptr;
        accumulate(&node, node.second);
        // Binary composition adds an output layer inside the primitive's
        // isolation layer, in addition to each input's compositing layer.
        ++node.depth;
        node.alpha_only = node.alpha_only && alpha_only(node.second);
        ::skity::BlendMode unused;
        if (primitive.type == Type::kBlend) {
          if (primitive.blend_mode != "normal")
            return nullptr;
        } else if (primitive.composite_operator == "arithmetic") {
          // Alpha difference, used by inset shadows.
          // Other arithmetic coefficients/color inputs require a general
          // arithmetic backend and must not silently become Porter-Duff out.
          if (!node.alpha_only || primitive.k1 != 0.f || primitive.k2 != -1.f ||
              primitive.k3 != 1.f || primitive.k4 != 0.f)
            return nullptr;
          node.depth += 2;
        } else if (!CompositeMode(primitive.composite_operator, &unused)) {
          return nullptr;
        }
        break;
      }
    }
    if (node.visits > kMaxReplayVisits || node.sources > kMaxSourceReplays ||
        node.depth > kMaxLayerDepth)
      return nullptr;
    names[primitive.result] = static_cast<int>(graph->nodes_.size());
    graph->nodes_.push_back(std::move(node));
  }
  // Account for filter outsets and both intermediate blur passes, not just
  // the visible filter region. A small viewport can still request enormous
  // offscreen textures through an untrusted standard deviation or transform.
  double max_pixels = 0.0;
  // Skity rasterizes layers using transform basis lengths, rather than the
  // transformed axis-aligned bounds, and applies surface content scale after
  // the canvas transform. Near-parallel bases can make MapRect underestimate
  // the texture area substantially.
  const double scale_x =
      std::hypot(transform.GetScaleX(), transform.GetSkewY()) * content_scale;
  const double scale_y =
      std::hypot(transform.GetSkewX(), transform.GetScaleY()) * content_scale;
  auto include_bounds = [&](const SrSVGBox& bounds, double sigma_x,
                            double sigma_y) {
    const double width = std::ceil(std::max(0.f, bounds.width) * scale_x) +
                         2 * std::ceil(3.0 * sigma_x * scale_x);
    const double height = std::ceil(std::max(0.f, bounds.height) * scale_y) +
                          2 * std::ceil(3.0 * sigma_y * scale_y);
    if (!std::isfinite(width) || !std::isfinite(height) || width > 16384.0 ||
        height > 16384.0)
      return false;
    max_pixels = std::max(max_pixels, width * height);
    return true;
  };
  if (!include_bounds(model.region, 0.0, 0.0))
    return nullptr;
  for (const auto& node : graph->nodes_) {
    const auto& primitive = node.primitive;
    const bool blur =
        primitive.type == canvas::SrFilterPrimitiveType::kGaussianBlur;
    // Blur captures the full filter input before clipping its output.
    if (!include_bounds(blur ? model.region : primitive.subregion,
                        blur ? primitive.std_deviation_x : 0.0,
                        blur ? primitive.std_deviation_y : 0.0))
      return nullptr;
  }
  // RGBA8 layers plus conservative headroom for texture alignment, captured
  // source layers and two-pass blur workspaces. This is an admission bound,
  // not a promise about total process memory.
  const double workspace_bytes =
      max_pixels * 4.0 * (graph->nodes_.back().depth + 6);
  if (workspace_bytes > kMaxLayerBytes)
    return nullptr;
  // Even a flood-only parent captures its children before discarding them.
  // Its zero source references must not erase their recording-work budget.
  graph->cost_ = {std::max(size_t{1}, graph->nodes_.back().sources),
                  graph->nodes_.back().visits, graph->nodes_.back().depth,
                  workspace_bytes};
  return graph;
}

void SrSkityFilterGraph::DrawLayer(int input, ::skity::Canvas* target,
                                   const ::skity::DisplayList& source,
                                   const ::skity::Rect& region,
                                   const ::skity::Paint& paint) const {
  target->SaveLayer(region, paint);
  target->DrawColor(0, ::skity::BlendMode::kSrc);
  DrawInput(input, target, source);
  target->Restore();
}

void SrSkityFilterGraph::DrawInput(int input, ::skity::Canvas* target,
                                   const ::skity::DisplayList& source) const {
  if (input < 0) {
    const auto base = target->GetTotalMatrix();
    ::skity::Paint input_paint;
    if (input == kSourceAlpha) {
      input_paint.SetColorFilter(AlphaMatrix(1.f, 0.f));
    }
    target->SaveLayer(Rect(model_.region), input_paint);
    target->DrawColor(0, ::skity::BlendMode::kSrc);
    SrSkityMatrixCanvas replay(target, base, ::skity::Matrix(), base);
    source.Draw(&replay);
    target->Restore();
    return;
  }
  const auto& node = nodes_[input];
  const auto& primitive = node.primitive;
  if (primitive.subregion.width <= 0.f || primitive.subregion.height <= 0.f)
    return;
  const auto region = Rect(primitive.subregion);
  const auto filter_region = Rect(model_.region);
  // A primitive subregion clips its output, not the inputs of this primitive
  // or its dependencies. Isolate the result before applying the output mask;
  // an early canvas clip would incorrectly cull a blur's sampling halo.
  target->SaveLayer(filter_region, ::skity::Paint());
  target->DrawColor(0, ::skity::BlendMode::kSrc);
  target->Save();
  using Type = canvas::SrFilterPrimitiveType;
  switch (primitive.type) {
    case Type::kFlood: {
      auto color = ::skity::Color4fFromColor(primitive.flood_color);
      color.a *= std::clamp(primitive.flood_opacity, 0.f, 1.f);
      ::skity::Paint paint;
      paint.SetFillColor(color);
      target->DrawRect(region, paint);
      break;
    }
    case Type::kOffset:
      target->Translate(primitive.dx, primitive.dy);
      DrawInput(node.first, target, source);
      break;
    case Type::kGaussianBlur: {
      ::skity::Paint blur;
      if (primitive.std_deviation_x > 0.f || primitive.std_deviation_y > 0.f)
        blur.SetImageFilter(::skity::ImageFilters::Blur(
            primitive.std_deviation_x, primitive.std_deviation_y));
      // Capture the full input before applying the output mask below. Pixels
      // outside the output subregion can contribute to the Gaussian blur.
      DrawLayer(node.first, target, source, filter_region, blur);
      break;
    }
    case Type::kColorMatrix: {
      ::skity::Paint matrix;
      matrix.SetColorFilter(BuildSkityColorFilter(primitive));
      DrawLayer(node.first, target, source, filter_region, matrix);
      break;
    }
    case Type::kBlend:
    case Type::kComposite: {
      ::skity::Paint output;
      const bool difference = primitive.type == Type::kComposite &&
                              primitive.composite_operator == "arithmetic";
      // Encode alpha into an opaque RGB channel at its full range:
      // 1 - min((1 - alpha2) + alpha1, 1) = max(alpha2 - alpha1, 0).
      // Half-range alpha encoding would introduce quantization and produce a
      // nonzero result even for two transparent inputs on an RGBA8 surface.
      if (difference)
        output.SetColorFilter(InvertedRedToAlpha());
      target->SaveLayer(filter_region, output);
      target->DrawColor(0, ::skity::BlendMode::kSrc);
      ::skity::Paint destination;
      if (difference)
        destination.SetColorFilter(AlphaToOpaqueRGB(-1.f, 1.f));
      DrawLayer(node.second, target, source, filter_region, destination);
      ::skity::Paint foreground;
      if (difference) {
        foreground.SetColorFilter(AlphaToOpaqueRGB(1.f, 0.f));
        foreground.SetBlendMode(::skity::BlendMode::kPlus);
      } else if (primitive.type == Type::kComposite) {
        ::skity::BlendMode mode;
        CompositeMode(primitive.composite_operator, &mode);
        foreground.SetBlendMode(mode);
      }
      DrawLayer(node.first, target, source, filter_region, foreground);
      target->Restore();
      break;
    }
  }
  target->Restore();
  ::skity::Paint mask_layer;
  mask_layer.SetBlendMode(::skity::BlendMode::kDstIn);
  target->SaveLayer(filter_region, mask_layer);
  target->DrawColor(0, ::skity::BlendMode::kSrc);
  ::skity::Paint mask;
  mask.SetFillColor(::skity::Color_WHITE);
  target->DrawRect(region, mask);
  target->Restore();
  target->Restore();
}

void SrSkityFilterGraph::Draw(::skity::Canvas* target,
                              const ::skity::DisplayList& source) const {
  target->Save();
  target->ClipRect(Rect(model_.region));
  DrawInput(static_cast<int>(nodes_.size()) - 1, target, source);
  target->Restore();
}

}  // namespace skity
}  // namespace svg
}  // namespace serval
