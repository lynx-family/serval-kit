// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGNode.h"

#include "parser/SrSVGTraversalState.h"
#if defined(_WIN32) || defined(_WIN64)
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "utils/SrSVGNumber.h"

#include "canvas/SrCanvas.h"
#include "element/SrSVGAnimation.h"
#include "element/SrSVGClipPath.h"
#include "element/SrSVGFilter.h"
#include "element/SrSVGFilterPrimitives.h"
#include "element/SrSVGMask.h"
#include "element/SrSVGTypes.h"
#include "utils/SrFloatComparison.h"

namespace serval {
namespace svg {
namespace element {

const float SrSVGNode::s_stroke_miter_limit = 4.f;

float SrSVGNode::ClampOpacity(float opacity) {
  return ClampUnitFloat(opacity);
}
bool SrPreparePattern(canvas::SrCanvas* canvas, SrSVGNodeBase* node,
                      SrSVGRenderContext& context);

namespace {

SrSVGLength MakePercentage(float value) {
  return SrSVGLength{value, SR_SVG_UNITS_PERCENTAGE};
}

std::string Trim(const std::string& value) {
  const auto start = std::find_if_not(value.begin(), value.end(), [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](char c) {
                     return std::isspace(static_cast<unsigned char>(c));
                   }).base();
  if (start >= end) {
    return "";
  }
  return std::string(start, end);
}

std::vector<std::string> SplitCssTokens(const std::string& value) {
  std::vector<std::string> tokens;
  std::stringstream stream(value);
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

bool IsHorizontalOriginKeyword(const std::string& token) {
  return token == "left" || token == "right";
}

bool IsVerticalOriginKeyword(const std::string& token) {
  return token == "top" || token == "bottom";
}

SrSVGLength OriginKeywordToLength(const std::string& token) {
  if (token == "right" || token == "bottom") {
    return MakePercentage(100.f);
  }
  if (token == "center") {
    return MakePercentage(50.f);
  }
  return MakePercentage(0.f);
}

SrSVGLength OriginTokenToLength(const std::string& token) {
  if (token == "left" || token == "right" || token == "top" ||
      token == "bottom" || token == "center") {
    return OriginKeywordToLength(token);
  }
  return make_serval_length(token.c_str());
}

float ResolveOriginLength(const SrSVGLength& length,
                          const SrSVGRenderContext& context,
                          const SrSVGBox& reference_box,
                          SrSVGLengthType length_type) {
  if (length.unit == SR_SVG_UNITS_PERCENTAGE) {
    const float size = length_type == SR_SVG_LENGTH_TYPE_VERTICAL
                           ? reference_box.height
                           : reference_box.width;
    return length.value / 100.f * size;
  }
  SrSVGRenderContext mutable_context = context;
  return convert_serval_length_to_float(&length, &mutable_context, length_type);
}

void PrepareIRIResource(canvas::SrCanvas* canvas, SrSVGRenderContext& context,
                        SrSVGPaint* paint) {
  if (!canvas || !paint || paint->type != SERVAL_PAINT_IRI ||
      !paint->content.iri || paint->content.iri[0] != '#') {
    return;
  }
  std::string id{paint->content.iri + 1};
  IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
  if (!nodes) {
    return;
  }
  auto it = nodes->find(id);
  if (it != nodes->end() && it->second) {
    SrPreparePattern(canvas, it->second, context);
  }
}

std::string AddAnimatedScalarValue(const std::string& base,
                                   const std::string& addition) {
  double base_number = 0.0;
  double addition_number = 0.0;
  const char* base_suffix =
      SrSVGParseNumberWithSuffix(base.c_str(), &base_number, nullptr);
  if (!base_suffix ||
      base_suffix != SrSVGParseNumberWithSuffix(addition.c_str(),
                                                &addition_number, nullptr)) {
    return "";
  }
  std::ostringstream stream;
  stream << base_number + addition_number << base_suffix;
  return stream.str();
}

void ReplacePaint(SrSVGPaint*& paint, const char* value) {
  auto* replacement = make_serval_paint(value);
  if (replacement) {
    release_serval_paint(paint);
    paint = replacement;
  }
}

bool HasZeroDefaultAnimatedAttribute(const std::string& name) {
  return name == "x" || name == "y" || name == "x1" || name == "y1" ||
         name == "x2" || name == "y2" || name == "cx" || name == "cy" ||
         name == "r" || name == "rx" || name == "ry" || name == "width" ||
         name == "height" || name == "dx" || name == "dy" ||
         name == "stdDeviation" || name == "offset";
}

bool ParseNumberAttribute(const char* value, float* out,
                          bool allow_percent = false) {
  SrSVGLength length;
  if (!parse_serval_length(value, &length))
    return false;
  if (allow_percent && length.unit == SR_SVG_UNITS_PERCENTAGE) {
    *out = length.value / 100.f;
  } else if (length.unit == SR_SVG_UNITS_NUMBER) {
    *out = length.value;
  } else {
    return false;
  }
  return true;
}

bool ParseOpacityAttribute(const char* value, std::optional<float>& out) {
  float opacity;
  if (!ParseNumberAttribute(value, &opacity, true))
    return false;
  out = std::clamp(opacity, 0.f, 1.f);
  return true;
}

}  // namespace
void SrSVGNodeBase::Render(canvas::SrCanvas* const canvas,
                           SrSVGRenderContext& context) {
  parser::SrSVGRenderScope render_scope(&context);
  if (!render_scope)
    return;
  parser::SrSVGViewBoxScope view_box_scope(context);
  canvas->Save();
  canvas->SetRenderContext(&context);
  OnPrepareToRender(canvas, context);
  parser::SrSVGOffscreenScope filter_budget(context);
  bool filter_layer_active = false;
  canvas::SrFilterModel filter_model;
  bool filter_output_empty = false;
  auto clip_to_box = [&canvas](const SrSVGBox& box) {
    auto clip_path = canvas->PathFactory()->CreateRect(
        box.left, box.top, 0.f, 0.f, box.width, box.height);
    if (clip_path) {
      canvas->ClipPath(clip_path.get(), SR_SVG_FILL);
    }
  };
  if (canvas->SupportsFilters() && IsSVGNode() && Tag() != SrSVGTag::kMask &&
      Tag() != SrSVGTag::kFilter) {
    auto* svg_node = static_cast<SrSVGNode*>(this);
    if (svg_node->filter_ && svg_node->filter_->type == SERVAL_PAINT_IRI &&
        svg_node->filter_->content.iri[0] == '#') {
      SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
      bool has_bounds = false;
      if (auto path = svg_node->AsPath(canvas->PathFactory(), &context)) {
        bounds = path->GetBounds();
        has_bounds = serval_svg_valid_box(&bounds);

        // Expand bounds by stroke width
        float stroke_w = 0.f;
        if (svg_node->stroke_width_.has_value()) {
          stroke_w = convert_serval_length_to_float(
              &*svg_node->stroke_width_, &context, SR_SVG_LENGTH_TYPE_OTHER);
        } else if (svg_node->inherit_stroke_width_.has_value()) {
          stroke_w = convert_serval_length_to_float(
              &*svg_node->inherit_stroke_width_, &context,
              SR_SVG_LENGTH_TYPE_OTHER);
        }

        // Check if stroke is actually drawn
        const auto* stroke = svg_node->ResolvePaint(
            &SrSVGNode::stroke_, &SrSVGNode::inherit_stroke_paint_);
        const bool has_stroke = stroke && stroke->type != SERVAL_PAINT_NONE;

        if (has_stroke) {
          // Default stroke width is 1.0 if not specified but stroke is present?
          // Logic in Render usually handles defaults. Here we just want to be safe.
          if (stroke_w <= 0.f &&
              (!svg_node->stroke_width_.has_value() &&
               !svg_node->inherit_stroke_width_.has_value())) {
            stroke_w = 1.f;
          }

          if (stroke_w > 0.f) {
            float half_w = stroke_w * 0.5f;
            bounds.left -= half_w;
            bounds.top -= half_w;
            bounds.width += stroke_w;
            bounds.height += stroke_w;
            has_bounds = serval_svg_valid_box(&bounds);
          }
        }
      }

      SrSVGFilter* filter_node = nullptr;
      if (context.id_mapper) {
        IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
        std::string id(svg_node->filter_->content.iri + 1);
        auto it = nodes->find(id);
        if (it != nodes->end()) {
          if (it->second && it->second->Tag() == SrSVGTag::kFilter) {
            filter_node = static_cast<SrSVGFilter*>(it->second);
          }
        }
      }

      if (filter_node) {
        if (filter_node->BuildFilterModel(bounds, has_bounds, context,
                                          &filter_model)) {
          if (filter_model.region.width <= 0.f ||
              filter_model.region.height <= 0.f) {
            filter_output_empty = true;
          } else if (serval_svg_valid_box(&filter_model.region) &&
                     canvas->SupportsFilterModel(filter_model) &&
                     filter_budget.Reserve(
                         *canvas, 5, 1 + 5 * filter_model.primitives.size()) &&
                     canvas->TryBeginFilterLayer(&filter_model.region,
                                                 filter_model)) {
            filter_layer_active = true;
            canvas->Save();
            clip_to_box(filter_model.region);
          }
          // Unsupported filter graphs fall back to rendering the source
          // element without the filter. Only an explicitly empty filter region
          // should suppress the source output.
        }
      }
    }
  }

  // Mask rendering via explicit source and mask-content phases. Backends can
  // map these phases to their own off-screen layer and DstIn primitives.
  bool masked = false;
  if (!filter_output_empty && IsSVGNode() && Tag() != SrSVGTag::kMask) {
    auto* svg_node = static_cast<SrSVGNode*>(this);
    SrSVGPaint* local_mask =
        svg_node->ResolvePaint(&SrSVGNode::mask_, &SrSVGNode::inherit_mask_);
    if (local_mask && local_mask->type == SERVAL_PAINT_IRI) {
      IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
      if (nodes && local_mask->content.iri[0] == '#') {
        std::string id(local_mask->content.iri + 1);
        auto it = nodes->find(id);
        if (it != nodes->end() && it->second &&
            it->second->Tag() == SrSVGTag::kMask) {
          auto* mask_node = static_cast<SrSVGMask*>(it->second);
          parser::SrSVGMaskScope mask_scope(
              static_cast<parser::SrSVGTraversalState*>(
                  context.traversal_state),
              mask_node);
          if (mask_scope) {
            SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
            bool has_bounds = false;
            if (auto path = svg_node->AsPath(canvas->PathFactory(), &context)) {
              bounds = path->GetBounds();
              has_bounds = serval_svg_valid_box(&bounds);
            }

            SrSVGBox mask_region{0.f, 0.f, 0.f, 0.f};
            bool has_mask_region = false;
            bool has_empty_mask_region = false;
            const bool can_resolve_mask_region =
                has_bounds || mask_node->mask_units() ==
                                  SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
            if (can_resolve_mask_region) {
              const SrSVGBox object_bounds =
                  has_bounds ? bounds : context.view_port;
              mask_region =
                  mask_node->ResolveMaskRegion(object_bounds, context);
              has_mask_region = serval_svg_valid_box(&mask_region);
              if (!has_mask_region) {
                has_empty_mask_region = true;
                const bool finite = std::isfinite(mask_region.left) &&
                                    std::isfinite(mask_region.top) &&
                                    std::isfinite(mask_region.width) &&
                                    std::isfinite(mask_region.height);
                masked = finite &&
                         (mask_region.width <= 0 || mask_region.height <= 0);
              }
            }

            parser::SrSVGOffscreenScope mask_budget(context);
            if (!has_empty_mask_region && mask_budget.Reserve(*canvas, 2) &&
                canvas->TryBeginMaskLayer(
                    has_mask_region ? &mask_region : nullptr,
                    mask_node->mask_is_luminance())) {
              if (has_mask_region) {
                canvas->Save();
                clip_to_box(mask_region);
                OnRender(canvas, context);
                canvas->Restore();
              } else {
                OnRender(canvas, context);
              }
              mask_scope.EnterContent();
              if (canvas->TryBeginMaskContentLayer()) {
                canvas->Save();
                if (has_mask_region) {
                  clip_to_box(mask_region);
                }
                if (has_bounds &&
                    mask_node->mask_content_units() ==
                        SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
                  float xform[6] = {bounds.width,  0.f,         0.f,
                                    bounds.height, bounds.left, bounds.top};
                  canvas->Transform(xform);
                }
                mask_node->Render(canvas, context);
                canvas->Restore();
                canvas->EndMaskContentLayer();
              }
              canvas->EndMaskLayer();
              masked = true;
            }
          }
        }
      }
    }
  }
  if (!masked && !filter_output_empty) {
    OnRender(canvas, context);
  }
  if (filter_layer_active) {
    canvas->Restore();
    canvas->EndFilterLayer();
  }
  canvas->Restore();
}

bool SrSVGNode::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "id") == 0) {
    id_ = value;
  } else if (strcmp(name, "onclick") == 0 || strcmp(name, "onClick") == 0 ||
             strcmp(name, "data-click") == 0 ||
             strcmp(name, "data-action") == 0) {
    click_event_ = value;
  } else if (strcmp(name, "fill") == 0) {
    ReplacePaint(fill_, value);
  } else if (strcmp(name, "stroke") == 0) {
    ReplacePaint(stroke_, value);
  } else if (strcmp(name, "opacity") == 0) {
    return ParseOpacityAttribute(value, opacity_);
  } else if (strcmp(name, "stroke-width") == 0) {
    return ParseLengthAttribute(value, stroke_width_,
                                LengthRange::kNonNegative);
  } else if (strcmp(name, "stroke-dasharray") == 0) {
    return ParseStrokeDashArray(value);
  } else if (strcmp(name, "stroke-dashoffset") == 0) {
    return ParseLengthAttribute(value, stroke_dash_offset_);
  } else if (strcmp(name, "stroke-linecap") == 0) {
    if (strcmp(value, "butt") && strcmp(value, "round") &&
        strcmp(value, "square"))
      return false;
    stroke_cap_ = resolve_stroke_line_cap(value);
  } else if (strcmp(name, "stroke-linejoin") == 0) {
    if (strcmp(value, "miter") && strcmp(value, "round") &&
        strcmp(value, "bevel"))
      return false;
    stroke_join_ = resolve_stroke_line_join(value);
  } else if (strcmp(name, "stroke-miterlimit") == 0) {
    float limit;
    if (!ParseNumberAttribute(value, &limit) || limit < 1.f)
      return false;
    stoke_miter_limit_ = limit;
  } else if (strcmp(name, "fill-opacity") == 0) {
    return ParseOpacityAttribute(value, fill_opacity_);
  } else if (strcmp(name, "stroke-opacity") == 0) {
    return ParseOpacityAttribute(value, stroke_opacity_);
  } else if (strcmp(name, "vector-effect") == 0) {
    if (strcmp(value, "non-scaling-stroke") == 0) {
      vector_effect_ = SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE;
    } else {
      vector_effect_ = SR_SVG_VECTOR_EFFECT_NONE;
    }
  } else if (strcmp(name, "clip-path") == 0) {
    ReplacePaint(clip_path_, value);
  } else if (strcmp(name, "mask") == 0) {
    ReplacePaint(mask_, value);
  } else if (strcmp(name, "filter") == 0) {
    ReplacePaint(filter_, value);
  } else if (strcmp(name, "transform") == 0) {
    return TryParseTransform(value, transform_);
  } else if (strcmp(name, "transform-origin") == 0) {
    ParseTransformOrigin(value);
  } else if (strcmp(name, "transform-box") == 0) {
    ParseTransformBox(value);
  } else if (strcmp(name, "color") == 0) {
    color_ = make_serval_color(value);
  } else if (strcmp(name, "style") == 0) {
    ParseStyle(value);
  } else {
    return false;
  }
  return true;
}

void SrSVGNode::StoreAttribute(const char* name, const char* value) {
  if (name && value) {
    base_attributes_[name] = value;
  }
}

void SrSVGNode::AddAnimation(SrSVGAnimation* animation) {
  if (animation && std::find(animations_.begin(), animations_.end(),
                             animation) == animations_.end()) {
    animations_.push_back(animation);
  }
}

void SrSVGNode::ApplyAnimations(double seconds, const IDMapper* id_mapper) {
  std::unordered_map<std::string, std::string> presentation_values;
  for (auto* animation : animations_) {
    if (!animation) {
      continue;
    }
    const std::string target_attribute = animation->TargetAttributeName();
    if (target_attribute.empty()) {
      continue;
    }
    auto presentation_it = presentation_values.find(target_attribute);
    if (presentation_it == presentation_values.end()) {
      auto base_it = base_attributes_.find(target_attribute);
      presentation_it =
          presentation_values
              .emplace(target_attribute,
                       base_it == base_attributes_.end() ? "" : base_it->second)
              .first;
    }

    SrSVGAnimation::Effect effect;
    if (!animation->Evaluate(seconds, id_mapper, presentation_it->second,
                             &effect) ||
        effect.attribute.empty()) {
      continue;
    }
    if (presentation_values.find(effect.attribute) ==
        presentation_values.end()) {
      auto base_it = base_attributes_.find(effect.attribute);
      presentation_values.emplace(
          effect.attribute,
          base_it == base_attributes_.end() ? "" : base_it->second);
    }
    std::string value = effect.value;
    if (effect.transform && effect.additive) {
      auto& presentation = presentation_values[effect.attribute];
      if (!presentation.empty()) {
        value = presentation + " " + value;
      }
    } else if (effect.additive) {
      auto& presentation = presentation_values[effect.attribute];
      if (!presentation.empty()) {
        const std::string added =
            AddAnimatedScalarValue(presentation, effect.value);
        if (!added.empty()) {
          value = added;
        }
      }
    }
    auto [record, first] = animated_attributes_.try_emplace(effect.attribute);
    if (first) {
      if (effect.attribute == "stroke-dashoffset") {
        record->second = stroke_dash_offset_;
      } else {
        auto base = base_attributes_.find(effect.attribute);
        if (base != base_attributes_.end())
          record->second = std::optional<std::string>(base->second);
      }
    }
    presentation_values[effect.attribute] = value;
    if (effect.path_data && effect.attribute == "d" &&
        SetAnimatedPathData(effect.path_data)) {
      continue;
    }
    ParseAndSetAttribute(effect.attribute.c_str(), value.c_str());
  }
}

void SrSVGNode::RestoreAnimatedAttributes() {
  for (auto& [name, saved] : animated_attributes_) {
    if (auto* offset = std::get_if<std::optional<SrSVGLength>>(&saved)) {
      stroke_dash_offset_ = *offset;
    } else if (auto* text = std::get_if<std::optional<std::string>>(&saved)) {
      RestoreAnimatedAttribute(name, *text);
    }
  }
  animated_attributes_.clear();
}

void SrSVGNode::RestoreAnimatedAttribute(
    const std::string& name, const std::optional<std::string>& base_value) {
  if (base_value.has_value()) {
    if (ParseAndSetAttribute(name.c_str(), base_value->c_str()))
      return;
  }
  ClearAnimatedAttribute(name);
}

void SrSVGNode::ClearAnimatedAttribute(const std::string& name) {
  if (name == "fill") {
    release_serval_paint(fill_);
    fill_ = nullptr;
  } else if (name == "stroke") {
    release_serval_paint(stroke_);
    stroke_ = nullptr;
  } else if (name == "opacity") {
    opacity_.reset();
  } else if (name == "fill-opacity") {
    fill_opacity_.reset();
  } else if (name == "stroke-opacity") {
    stroke_opacity_.reset();
  } else if (name == "stroke-width") {
    stroke_width_.reset();
  } else if (name == "fill-rule") {
    ParseAndSetAttribute("fill-rule", "nonzero");
  } else if (name == "stroke-dasharray") {
    stroke_dash_array_.clear();
  } else if (name == "stroke-dashoffset") {
    stroke_dash_offset_.reset();
  } else if (name == "stroke-linecap") {
    stroke_cap_ = SR_SVG_STROKE_CAP_BUTT;
  } else if (name == "stroke-linejoin") {
    stroke_join_ = SR_SVG_STROKE_JOIN_MITER;
  } else if (name == "stroke-miterlimit") {
    stoke_miter_limit_ = s_stroke_miter_limit;
  } else if (name == "vector-effect") {
    vector_effect_ = SR_SVG_VECTOR_EFFECT_NONE;
  } else if (name == "clip-path") {
    release_serval_paint(clip_path_);
    clip_path_ = nullptr;
  } else if (name == "mask") {
    release_serval_paint(mask_);
    mask_ = nullptr;
  } else if (name == "filter") {
    release_serval_paint(filter_);
    filter_ = nullptr;
  } else if (name == "transform") {
    xform_identity(transform_);
  } else if (name == "transform-origin") {
    has_transform_origin_ = false;
    transform_origin_x_length_ = SrSVGLength{0.f, SR_SVG_UNITS_NUMBER};
    transform_origin_y_length_ = SrSVGLength{0.f, SR_SVG_UNITS_NUMBER};
    transform_origin_x_ = 0.f;
    transform_origin_y_ = 0.f;
  } else if (name == "transform-box") {
    transform_box_ = SrSVGTransformBox::kViewBox;
  } else if (name == "color") {
    color_.reset();
  } else if (name == "d") {
    ParseAndSetAttribute(name.c_str(), "");
  } else if (name == "points") {
    ParseAndSetAttribute(name.c_str(), "");
  } else if (name == "viewBox") {
    ParseAndSetAttribute(name.c_str(), "0 0 0 0");
  } else if (HasZeroDefaultAnimatedAttribute(name)) {
    ParseAndSetAttribute(name.c_str(), "0");
  }
}

SrSVGPaint* SrSVGNode::ResolvePaint(SrSVGPaint* SrSVGNode::*local,
                                    SrSVGPaint* SrSVGNode::*inherited) const {
  for (auto* node = this; node; node = node->authored_parent_) {
    if (auto* paint = node->*local)
      return paint;
    if (auto* paint = node->*inherited)
      return paint;
  }
  return nullptr;
}

SrSVGNode::~SrSVGNode() {
  release_serval_paint(fill_);
  release_serval_paint(stroke_);
  release_serval_paint(clip_path_);
  release_serval_paint(mask_);
  release_serval_paint(filter_);
}

bool SrPreparePattern(canvas::SrCanvas* canvas, SrSVGNodeBase* node,
                      SrSVGRenderContext& context) {
  switch (node->Tag()) {
    case SrSVGTag::kLinearGradient:
    case SrSVGTag::kRadialGradient:
    case SrSVGTag::kPattern:
      // paint will be generated on platform layer indexed by id.
      node->Render(canvas, context);
      return true;
    default:
      return false;
  }
}

bool SrSVGNode::OnPrepareToRender(canvas::SrCanvas* canvas,
                                  SrSVGRenderContext& context) const {
  SrSVGPaint* local_clip_path =
      ResolvePaint(&SrSVGNode::clip_path_, &SrSVGNode::inherit_clip_path_);
  if (local_clip_path && local_clip_path->type == SERVAL_PAINT_IRI &&
      local_clip_path->content.iri && local_clip_path->content.iri[0] == '#') {
    auto* nodes = static_cast<IDMapper*>(context.id_mapper);
    if (nodes) {
      auto it = nodes->find(local_clip_path->content.iri + 1);
      if (it != nodes->end() && it->second &&
          it->second->Tag() == SrSVGTag::kClipPath) {
        auto* clip = static_cast<SrSVGClipPath*>(it->second);
        auto path = clip->AsPath(canvas->PathFactory(), &context);
        if (path && clip->clip_path_units() ==
                        SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
          if (auto node_path = AsPath(canvas->PathFactory(), &context)) {
            const SrSVGBox box = node_path->GetBounds();
            if (std::isfinite(box.left) && std::isfinite(box.top) &&
                std::isfinite(box.width) && std::isfinite(box.height)) {
              const float xform[6] = {box.width,  0.f,      0.f,
                                      box.height, box.left, box.top};
              path = path->CreateTransformCopy(xform);
            }
          }
        }
        if (path)
          canvas->ClipPath(path.get(), clip->clip_rule());
      }
    }
  }

  PrepareIRIResource(
      canvas, context,
      ResolvePaint(&SrSVGNode::fill_, &SrSVGNode::inherit_fill_paint_));
  PrepareIRIResource(
      canvas, context,
      ResolvePaint(&SrSVGNode::stroke_, &SrSVGNode::inherit_stroke_paint_));
  return false;
}

bool SrSVGNode::TryParseTransform(const char* str, float* xform) {
  return str && SrSVGParseTransform(str, strlen(str), xform, nullptr);
}

void SrSVGNode::ParseTransform(const char* str, float* xform) {
  if (!TryParseTransform(str, xform))
    xform_identity(xform);
}

namespace {
int ParseTransformFunction(float* out, const char* str, const char* name) {
  if (!str || !out || strncmp(str, name, strlen(name)))
    return 0;
  SrSVGParseCursor cursor = SrSVGCursor(str, strlen(str));
  float result[6];
  if (!SrSVGParseTransformFunction(&cursor, result, nullptr) ||
      cursor.pos - str >= INT_MAX)
    return 0;
  memcpy(out, result, sizeof(result));
  return (int)(cursor.pos - str);
}
}  // namespace

int SrSVGNode::ParseMatrix(float* out, const char* str) {
  return ParseTransformFunction(out, str, "matrix");
}

int SrSVGNode::ParseTranslate(float* out, const char* str) {
  return ParseTransformFunction(out, str, "translate");
}

int SrSVGNode::ParseScale(float* out, const char* str) {
  return ParseTransformFunction(out, str, "scale");
}

int SrSVGNode::ParseRotate(float* out, const char* str) {
  return ParseTransformFunction(out, str, "rotate");
}

int SrSVGNode::ParseSkewX(float* out, const char* str) {
  return ParseTransformFunction(out, str, "skewX");
}

int SrSVGNode::ParseSkewY(float* out, const char* str) {
  return ParseTransformFunction(out, str, "skewY");
}

int SrSVGNode::ParseTransformArgs(const char* str, float* args,
                                  int max_args_number, int* arg_index) {
  if (!str || !arg_index || max_args_number < 0)
    return 0;
  SrSVGParseCursor cursor = SrSVGCursor(str, strlen(str));
  size_t count = 0;
  *arg_index = 0;
  if (!SrSVGParseTransformArgs(&cursor, args, (size_t)max_args_number, &count,
                               nullptr) ||
      cursor.pos - str >= INT_MAX)
    return 0;
  *arg_index = (int)count;
  return (int)(cursor.pos - str);
}

bool SrSVGNode::ParseStrokeDashArray(const char* value) {
  if (!serval_svg_valid_dash_array(value))
    return false;
  std::vector<float> dashes;
  auto cursor = SrSVGCursor(value, strlen(value));
  SrSVGCursorSpaces(&cursor);
  if (!SrSVGCursorMatch(&cursor, "none")) {
    while (!SrSVGCursorEnd(&cursor)) {
      float entry;
      if (!SrSVGParseCoordinate(&cursor, &entry, nullptr))
        return false;
      dashes.push_back(entry);
      SrSVGCursorSeparator(&cursor);
    }
  }
  stroke_dash_array_ = std::move(dashes);
  return true;
}

void SrSVGNode::ParseTransformOrigin(const char* value) {
  if (!value) {
    return;
  }
  const auto tokens = SplitCssTokens(Trim(value));
  if (tokens.empty()) {
    return;
  }

  SrSVGLength x = MakePercentage(50.f);
  SrSVGLength y = MakePercentage(50.f);
  if (tokens.size() == 1) {
    const std::string& token = tokens[0];
    if (IsVerticalOriginKeyword(token)) {
      y = OriginKeywordToLength(token);
    } else if (token == "center") {
      x = MakePercentage(50.f);
      y = MakePercentage(50.f);
    } else {
      x = OriginTokenToLength(token);
    }
  } else {
    const std::string& first = tokens[0];
    const std::string& second = tokens[1];
    if (IsHorizontalOriginKeyword(first)) {
      x = OriginKeywordToLength(first);
      y = OriginTokenToLength(second);
    } else if (IsVerticalOriginKeyword(first)) {
      x = OriginTokenToLength(second);
      y = OriginKeywordToLength(first);
    } else if (IsHorizontalOriginKeyword(second)) {
      x = OriginKeywordToLength(second);
      y = OriginTokenToLength(first);
    } else if (IsVerticalOriginKeyword(second)) {
      x = OriginTokenToLength(first);
      y = OriginKeywordToLength(second);
    } else {
      x = OriginTokenToLength(first);
      y = OriginTokenToLength(second);
    }
  }
  transform_origin_x_length_ = x;
  transform_origin_y_length_ = y;
  transform_origin_x_ = x.value;
  transform_origin_y_ = y.value;
  has_transform_origin_ = true;
}

void SrSVGNode::ParseTransformBox(const char* value) {
  if (!value) {
    return;
  }
  const std::string token = Trim(value);
  if (token == "fill-box" || token == "content-box") {
    transform_box_ = SrSVGTransformBox::kFillBox;
  } else if (token == "stroke-box" || token == "border-box") {
    transform_box_ = SrSVGTransformBox::kStrokeBox;
  } else if (token == "view-box") {
    transform_box_ = SrSVGTransformBox::kViewBox;
  }
}

bool SrSVGNode::ResolveTransformOrigin(
    float* x, float* y, const SrSVGRenderContext& context,
    canvas::PathFactory* path_factory) const {
  if (!has_transform_origin_ || !x || !y) {
    return false;
  }

  SrSVGBox reference_box = context.view_box;
  if (reference_box.width == 0.f || reference_box.height == 0.f) {
    reference_box = context.view_port;
  }

  if (path_factory && transform_box_ != SrSVGTransformBox::kViewBox) {
    SrSVGRenderContext mutable_context = context;
    auto path = AsPath(path_factory, &mutable_context, false);
    if (path) {
      if (transform_box_ == SrSVGTransformBox::kStrokeBox) {
        float stroke_width = 0.f;
        if (stroke_width_) {
          stroke_width = convert_serval_length_to_float(
              &(*stroke_width_), &mutable_context, SR_SVG_LENGTH_TYPE_OTHER);
        } else if (inherit_stroke_width_) {
          stroke_width = convert_serval_length_to_float(
              &(*inherit_stroke_width_), &mutable_context,
              SR_SVG_LENGTH_TYPE_OTHER);
        }
        if (stroke_width > 0.f) {
          auto stroke_path = path_factory->CreateStrokePath(
              path.get(), stroke_width, stroke_cap_, stroke_join_,
              stoke_miter_limit_);
          if (stroke_path) {
            path = std::move(stroke_path);
          }
        }
      }
      const SrSVGBox bounds = path->GetBounds();
      if (bounds.width > 0.f || bounds.height > 0.f) {
        reference_box = bounds;
      }
    }
  }

  *x = reference_box.left + ResolveOriginLength(transform_origin_x_length_,
                                                context, reference_box,
                                                SR_SVG_LENGTH_TYPE_HORIZONTAL);
  *y = reference_box.top + ResolveOriginLength(transform_origin_y_length_,
                                               context, reference_box,
                                               SR_SVG_LENGTH_TYPE_VERTICAL);
  return true;
}

void SrSVGNode::ResolvedTransform(float (&xform)[6],
                                  const SrSVGRenderContext& context,
                                  canvas::PathFactory* path_factory) const {
  std::memcpy(xform, transform_, sizeof(float) * 6);
  float origin_x = 0.f;
  float origin_y = 0.f;
  if (!ResolveTransformOrigin(&origin_x, &origin_y, context, path_factory)) {
    return;
  }

  float centered[6];
  xform_identity(centered);
  xform_pre_translate(centered, origin_x, origin_y);
  xform_multiply(centered, xform);
  xform_pre_translate(centered, -origin_x, -origin_y);
  if (std::all_of(std::begin(centered), std::end(centered),
                  [](float value) { return std::isfinite(value); }))
    std::memcpy(xform, centered, sizeof(centered));
}

static int IsSpace(char c) {
  if (c == 0)
    return 0;
  return strchr(" \t\n\v\f\r", c) != 0;
}

bool SrSVGNodeBase::ParseNameValue(const char* start, const char* end) {
  const char* str;
  const char* val;
  char name[512];
  char value[512];
  int n;

  str = start;
  while (str < end && *str != ':')
    ++str;

  val = str;

  // Right Trim
  while (str > start && (str == end || *str == ':' || IsSpace(*str)))
    --str;
  ++str;

  n = (int)(str - start);
  if (n > 511)
    n = 511;
  if (n)
    memcpy(name, start, n);
  name[n] = 0;

  while (val < end && (*val == ':' || IsSpace(*val)))
    ++val;

  n = (int)(end - val);
  if (n > 511)
    n = 511;
  if (n)
    memcpy(value, val, n);
  value[n] = 0;

  // A CSS declaration cannot recursively invoke the style attribute parser.
  return strcmp(name, "style") != 0 && ParseAndSetAttribute(name, value);
}

void SrSVGNodeBase::ParseStyle(const char* str) {
  const char* start;
  const char* end;
  while (*str) {
    // Left Trim
    while (*str && IsSpace(*str))
      ++str;
    start = str;
    int parenthesis_depth = 0;
    int quote_status = 0;  // 0: no quote, 1: single quote, 2: double quote
    while (*str) {
      if (*str == ';' && parenthesis_depth == 0 && quote_status == 0) {
        break;
      }
      if (*str == '(') {
        parenthesis_depth++;
      } else if (*str == ')') {
        if (parenthesis_depth > 0)
          parenthesis_depth--;
      } else if (*str == '\'') {
        if (quote_status == 0)
          quote_status = 1;
        else if (quote_status == 1)
          quote_status = 0;
      } else if (*str == '"') {
        if (quote_status == 0)
          quote_status = 2;
        else if (quote_status == 2)
          quote_status = 0;
      }
      ++str;
    }
    end = str;

    // Right Trim
    while (end > start && (*end == 0 || *end == ';' || IsSpace(*end)))
      --end;
    ++end;

    ParseNameValue(start, end);
    if (*str)
      ++str;
  }
}

const char* SrSVGNode::ParseNumber(const char* s, char* token, const int size) {
  if (!s || !token || size <= 0)
    return s;
  token[0] = '\0';
  SrSVGParseCursor cursor = {s, s, nullptr};
  float value;
  if (!SrSVGParseCoordinate(&cursor, &value, nullptr) || cursor.pos - s >= size)
    return s;
  const size_t length = (size_t)(cursor.pos - s);
  memcpy(token, s, length);
  token[length] = '\0';
  return cursor.pos;
}

double SrSVGNode::Atof(const char* s) {
  if (!s)
    return 0.0;
  SrSVGSkipSpaces(&s);
  double value;
  if (!SrSVGParseNumber(&s, &value) || fabs(value) > FLT_MAX)
    return 0.0;
  SrSVGSkipSpaces(&s);
  return *s ? 0.0 : value;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
