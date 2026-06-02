// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGNode.h"
#if defined(_WIN32) || defined(_WIN64)
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

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
      !paint->content.iri || strlen(paint->content.iri) == 0) {
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

bool ParseNumberWithSuffix(const std::string& value, double* number,
                           std::string* suffix) {
  if (!number || !suffix) {
    return false;
  }
  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str()) {
    return false;
  }
  *number = parsed;
  *suffix = end ? end : "";
  return true;
}

std::string AddAnimatedScalarValue(const std::string& base,
                                   const std::string& addition) {
  double base_number = 0.0;
  double addition_number = 0.0;
  std::string base_suffix;
  std::string addition_suffix;
  if (!ParseNumberWithSuffix(base, &base_number, &base_suffix) ||
      !ParseNumberWithSuffix(addition, &addition_number, &addition_suffix) ||
      base_suffix != addition_suffix) {
    return "";
  }
  std::ostringstream stream;
  stream << base_number + addition_number << base_suffix;
  return stream.str();
}

}  // namespace
void SrSVGNodeBase::Render(canvas::SrCanvas* const canvas,
                           SrSVGRenderContext& context) {
  canvas->Save();
  canvas->SetRenderContext(&context);
  OnPrepareToRender(canvas, context);
  bool filter_clipped = false;
  bool filter_layer_active = false;
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
    if (svg_node->filter_ && svg_node->filter_->type == SERVAL_PAINT_IRI) {
      SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
      bool has_bounds = false;
      if (auto path = svg_node->AsPath(canvas->PathFactory(), &context)) {
        bounds = path->GetBounds();
        has_bounds = bounds.width > 0.f && bounds.height > 0.f;

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
        bool has_stroke = false;
        if (svg_node->stroke_ && svg_node->stroke_->type != SERVAL_PAINT_NONE) {
          has_stroke = true;
        } else if (svg_node->inherit_stroke_paint_ &&
                   svg_node->inherit_stroke_paint_->type != SERVAL_PAINT_NONE) {
          // If not overridden locally
          if (!svg_node->stroke_)
            has_stroke = true;
        }

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

      canvas::SrFilterModel filter_model;
      if (filter_node) {
        if (filter_node->BuildFilterModel(bounds, has_bounds, context,
                                          &filter_model)) {
          if (filter_model.region.width <= 0.f ||
              filter_model.region.height <= 0.f) {
            filter_output_empty = true;
          } else if (canvas->SupportsFilterModel(filter_model)) {
            canvas->BeginFilterLayer(&filter_model.region, filter_model);
            filter_layer_active = true;
            canvas->Save();
            clip_to_box(filter_model.region);
            filter_clipped = true;
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
  if (IsSVGNode() && Tag() != SrSVGTag::kMask) {
    auto* svg_node = static_cast<SrSVGNode*>(this);
    SrSVGPaint* local_mask =
        svg_node->mask_ != nullptr ? svg_node->mask_ : svg_node->inherit_mask_;
    if (local_mask && local_mask->type == SERVAL_PAINT_IRI) {
      IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
      if (nodes) {
        std::string id(local_mask->content.iri + 1);
        auto it = nodes->find(id);
        if (it != nodes->end() && it->second &&
            it->second->Tag() == SrSVGTag::kMask) {
          auto* mask_node = static_cast<SrSVGMask*>(it->second);
          SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
          bool has_bounds = false;
          if (auto path = svg_node->AsPath(canvas->PathFactory(), &context)) {
            bounds = path->GetBounds();
            has_bounds = true;
          }

          SrSVGBox mask_region{0.f, 0.f, 0.f, 0.f};
          bool has_mask_region = false;
          bool has_empty_mask_region = false;
          const bool can_resolve_mask_region =
              has_bounds ||
              mask_node->mask_units() == SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
          if (can_resolve_mask_region) {
            const SrSVGBox object_bounds =
                has_bounds ? bounds : context.view_port;
            mask_region = mask_node->ResolveMaskRegion(object_bounds, context);
            has_mask_region =
                mask_region.width > 0.f && mask_region.height > 0.f;
            if (!has_mask_region) {
              masked = true;
              has_empty_mask_region = true;
            }
          }

          if (!has_empty_mask_region) {
            canvas->BeginMaskLayer(has_mask_region ? &mask_region : nullptr,
                                   mask_node->mask_is_luminance());
            if (has_mask_region) {
              canvas->Save();
              clip_to_box(mask_region);
              OnRender(canvas, context);
              canvas->Restore();
            } else {
              OnRender(canvas, context);
            }
            canvas->BeginMaskContentLayer();
            canvas->Save();
            if (has_mask_region) {
              clip_to_box(mask_region);
            }
            if (has_bounds && mask_node->mask_content_units() ==
                                  SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
              float xform[6] = {bounds.width,  0.f,         0.f,
                                bounds.height, bounds.left, bounds.top};
              canvas->Transform(xform);
            }
            mask_node->Render(canvas, context);
            canvas->Restore();
            canvas->EndMaskContentLayer();
            canvas->EndMaskLayer();
            masked = true;
          }
        }
      }
    }
  }
  if (!masked && !filter_output_empty) {
    OnRender(canvas, context);
  }
  if (filter_layer_active) {
    if (filter_clipped) {
      canvas->Restore();
    }
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
    release_serval_paint(fill_);
    fill_ = make_serval_paint(value);
  } else if (strcmp(name, "stroke") == 0) {
    release_serval_paint(stroke_);
    stroke_ = make_serval_paint(value);
  } else if (strcmp(name, "opacity") == 0) {
    opacity_ = Atof(value);
  } else if (strcmp(name, "stroke-width") == 0) {
    stroke_width_ = make_serval_length(value);
  } else if (strcmp(name, "stroke-dasharray") == 0) {
    ParseStrokeDashArray(value);
  } else if (strcmp(name, "stroke-dashoffset") == 0) {
    stroke_dash_offset_ = Atof(value);
  } else if (strcmp(name, "stroke-linecap") == 0) {
    stroke_cap_ = resolve_stroke_line_cap(value);
  } else if (strcmp(name, "stroke-linejoin") == 0) {
    stroke_join_ = resolve_stroke_line_join(value);
  } else if (strcmp(name, "stroke-miterlimit") == 0) {
    stoke_miter_limit_ = Atof(value);
  } else if (strcmp(name, "fill-opacity") == 0) {
    fill_opacity_ = Atof(value);
  } else if (strcmp(name, "stroke-opacity") == 0) {
    stroke_opacity_ = Atof(value);
  } else if (strcmp(name, "vector-effect") == 0) {
    if (strcmp(value, "non-scaling-stroke") == 0) {
      vector_effect_ = SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE;
    } else {
      vector_effect_ = SR_SVG_VECTOR_EFFECT_NONE;
    }
  } else if (strcmp(name, "clip-path") == 0) {
    clip_path_ = make_serval_paint(value);
  } else if (strcmp(name, "mask") == 0) {
    mask_ = make_serval_paint(value);
  } else if (strcmp(name, "filter") == 0) {
    filter_ = make_serval_paint(value);
  } else if (strcmp(name, "transform") == 0) {
    ParseTransform(value, transform_);
  } else if (strcmp(name, "transform-origin") == 0) {
    ParseTransformOrigin(value);
  } else if (strcmp(name, "transform-box") == 0) {
    ParseTransformBox(value);
  } else if (strcmp(name, "color") == 0) {
    color_ = make_serval_color(value);
  } else if (strcmp(name, "style") == 0) {
    ParseStyle(value);
  }
  return false;
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
              .emplace(target_attribute, base_it == base_attributes_.end()
                                             ? ""
                                             : base_it->second)
              .first;
    }

    SrSVGAnimation::Effect effect;
    if (!animation->Evaluate(seconds, id_mapper, presentation_it->second,
                             &effect) ||
        effect.attribute.empty()) {
      continue;
    }
    if (presentation_values.find(effect.attribute) == presentation_values.end()) {
      auto base_it = base_attributes_.find(effect.attribute);
      presentation_values.emplace(effect.attribute,
                                  base_it == base_attributes_.end()
                                      ? ""
                                      : base_it->second);
    }
    if (animated_attributes_.find(effect.attribute) ==
        animated_attributes_.end()) {
      auto base_it = base_attributes_.find(effect.attribute);
      animated_attributes_[effect.attribute] =
          base_it == base_attributes_.end() ? "" : base_it->second;
    }
    std::string value = effect.value;
    if (effect.attribute == "transform" && effect.additive) {
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
    presentation_values[effect.attribute] = value;
    ParseAndSetAttribute(effect.attribute.c_str(), value.c_str());
  }
}

void SrSVGNode::RestoreAnimatedAttributes() {
  for (const auto& entry : animated_attributes_) {
    if (!entry.second.empty()) {
      ParseAndSetAttribute(entry.first.c_str(), entry.second.c_str());
    } else if (entry.first == "transform") {
      ParseAndSetAttribute(entry.first.c_str(), "");
    }
  }
  animated_attributes_.clear();
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
      clip_path_ != nullptr ? clip_path_ : inherit_clip_path_;
  if (local_clip_path && local_clip_path->type == SERVAL_PAINT_IRI &&
      local_clip_path->content.iri &&
      strlen(local_clip_path->content.iri) > 0) {
    const char* iri_str = local_clip_path->content.iri;
    std::string id(iri_str + 1);
    IDMapper* nodes = static_cast<IDMapper*>(context.id_mapper);
    if (nodes) {
      // Use find() to safely look up ID, avoid throwing exceptions
      auto it = nodes->find(id);
      if (it != nodes->end()) {
        SrSVGClipPath* clip_path_node = static_cast<SrSVGClipPath*>(it->second);
        if (clip_path_node) {
          std::unique_ptr<canvas::Path> path =
              clip_path_node->AsPath(canvas->PathFactory(), &context);
          if (clip_path_node->clip_path_units() ==
              SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
            auto node_path = this->AsPath(canvas->PathFactory(), &context);
            if (node_path) {
              SrSVGBox svg_box = node_path->GetBounds();
              float xform[6] = {svg_box.width, 0,          0, svg_box.height,
                                svg_box.left,  svg_box.top};
              path = path->CreateTransformCopy(xform);
            }
          }
          if (path) {
            canvas->ClipPath(path.get(), clip_path_node->clip_rule());
          }
        }
      }
    }
  }

  PrepareIRIResource(canvas, context, fill_ ? fill_ : inherit_fill_paint_);
  PrepareIRIResource(canvas, context, stroke_ ? stroke_ : inherit_stroke_paint_);
  return false;
}

void SrSVGNode::ParseTransform(const char* str, float* xform) {
  xform_identity(xform);
  float tmp_xform[6];
  while (*str) {
    if (strncmp(str, "matrix", 6) == 0) {
      str += ParseMatrix(tmp_xform, str);
    } else if (strncmp(str, "translate", 9) == 0) {
      str += ParseTranslate(tmp_xform, str);
    } else if (strncmp(str, "scale", 5) == 0) {
      str += ParseScale(tmp_xform, str);
    } else if (strncmp(str, "rotate", 6) == 0) {
      str += ParseRotate(tmp_xform, str);
    } else if (strncmp(str, "skewX", 5) == 0) {
      str += ParseSkewX(tmp_xform, str);
    } else if (strncmp(str, "skewY", 5) == 0) {
      str += ParseSkewY(tmp_xform, str);
    } else {
      ++str;
      continue;
    }
    xform_multiply(xform, tmp_xform);
  }
}

int SrSVGNode::ParseMatrix(float* xform, const char* str) {
  float args[6];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 6, &arg_index);
  if (arg_index == 6) {
    memcpy(xform, args, sizeof(float) * 6);
  }
  return length;
}

int SrSVGNode::ParseTranslate(float* xform, const char* str) {
  float args[2];
  int arg_index = 0;
  float t[6];
  int length = ParseTransformArgs(str, args, 2, &arg_index);
  if (arg_index == 1) {
    args[1] = 0.f;
  }
  xform_set_translation(t, args[0], args[1]);
  memcpy(xform, t, sizeof(float) * 6);
  return length;
}

int SrSVGNode::ParseScale(float* xform, const char* str) {
  float args[2];
  int arg_index = 0;
  float t[6];
  int length = ParseTransformArgs(str, args, 2, &arg_index);
  if (arg_index == 1) {
    args[1] = args[0];
  }
  xform_set_scale(t, args[0], args[1]);
  memcpy(xform, t, sizeof(float) * 6);
  return length;
}

int SrSVGNode::ParseRotate(float* xform, const char* str) {
  float args[3];
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 3, &arg_index);
  if (arg_index == 1) {
    args[1] = 0.f;
    args[2] = 0.f;
  }
  float tmp_form[6];
  xform_identity(xform);
  if (arg_index > 1) {
    xform_set_translation(tmp_form, -args[1], -args[2]);
    xform_pre_multiply(xform, tmp_form);
  }
  xform_set_rotation(tmp_form, args[0] / 180.0f * M_PI);
  xform_pre_multiply(xform, tmp_form);
  if (arg_index > 1) {
    xform_set_translation(tmp_form, args[1], args[2]);
    xform_pre_multiply(xform, tmp_form);
  }
  return length;
}

int SrSVGNode::ParseSkewX(float* xform, const char* str) {
  float args[1] = {0.f};
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 1, &arg_index);
  xform_set_skewX(xform, args[0] / 180.0f * M_PI);
  return length;
}

int SrSVGNode::ParseSkewY(float* xform, const char* str) {
  float args[1] = {0.f};
  int arg_index = 0;
  int length = ParseTransformArgs(str, args, 1, &arg_index);
  xform_set_skewY(xform, args[0] / 180.0f * M_PI);
  return length;
}

int SrSVGNode::ParseTransformArgs(const char* str, float* args,
                                  int max_args_number, int* arg_index) {
  const char* end = nullptr;
  const char* ptr = nullptr;
  char it[64];

  *arg_index = 0;
  ptr = str;
  while (*ptr && *ptr != '(') {
    ++ptr;
  }
  if (*ptr == 0) {
    return 1;
  }
  end = ptr;
  while (*end && *end != ')') {
    ++end;
  }
  if (*end == 0) {
    return 1;
  }
  while (ptr < end) {
    if (*ptr == '-' || *ptr == '+' || *ptr == '.' || isdigit(*ptr)) {
      ptr = ParseNumber(ptr, it, 64);
      if (*arg_index < max_args_number) {
        args[(*arg_index)++] = (float)Atof(it);
      }
    } else {
      ++ptr;
    }
  }
  return (int)(end - str);
}

void SrSVGNode::ParseStrokeDashArray(const char* value) {
  stroke_dash_array_.clear();
  if (!value) {
    return;
  }

  const char* ptr = value;
  char token[64];
  while (*ptr) {
    while (*ptr && (isspace(*ptr) || *ptr == ',')) {
      ++ptr;
    }
    if (!*ptr) {
      break;
    }

    ptr = ParseNumber(ptr, token, sizeof(token));
    if (token[0] != '\0') {
      stroke_dash_array_.push_back(static_cast<float>(Atof(token)));
    }
  }
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

  *x = reference_box.left +
       ResolveOriginLength(transform_origin_x_length_, context, reference_box,
                           SR_SVG_LENGTH_TYPE_HORIZONTAL);
  *y = reference_box.top +
       ResolveOriginLength(transform_origin_y_length_, context, reference_box,
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
  std::memcpy(xform, centered, sizeof(float) * 6);
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

  return ParseAndSetAttribute(name, value);
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

const char* SrSVGNode::ParseNumber(const char* s, char* it, const int size) {
  const int last = size - 1;
  int i = 0;

  // sign
  if (*s == '-' || *s == '+') {
    if (i < last)
      it[i++] = *s;
    s++;
  }
  // integer part
  while (*s && isdigit(*s)) {
    if (i < last)
      it[i++] = *s;
    s++;
  }
  if (*s == '.') {
    // decimal point
    if (i < last)
      it[i++] = *s;
    s++;
    // fraction part
    while (*s && isdigit(*s)) {
      if (i < last)
        it[i++] = *s;
      s++;
    }
  }
  // exponent
  if ((*s == 'e' || *s == 'E') && (s[1] != 'm' && s[1] != 'x')) {
    if (i < last)
      it[i++] = *s;
    s++;
    if (*s == '-' || *s == '+') {
      if (i < last)
        it[i++] = *s;
      s++;
    }
    while (*s && isdigit(*s)) {
      if (i < last)
        it[i++] = *s;
      s++;
    }
  }
  it[i] = '\0';

  return s;
}

double SrSVGNode::Atof(const char* s) {
  char* cur = (char*)s;
  char* end = NULL;
  double res = 0.0, sign = 1.0;
  long long intPart = 0, fracPart = 0;
  char hasIntPart = 0, hasFracPart = 0;

  // Parse optional sign
  if (*cur == '+') {
    cur++;
  } else if (*cur == '-') {
    sign = -1;
    cur++;
  }

  // Parse integer part
  if (isdigit(*cur)) {
    // Parse digit sequence
    intPart = strtoll(cur, &end, 10);
    if (cur != end) {
      res = (double)intPart;
      hasIntPart = 1;
      cur = end;
    }
  }

  // Parse fractional part.
  if (*cur == '.') {
    cur++;  // Skip '.'
    if (isdigit(*cur)) {
      // Parse digit sequence
      fracPart = strtoll(cur, &end, 10);
      if (cur != end) {
        res += (double)fracPart / pow(10.0, (double)(end - cur));
        hasFracPart = 1;
        cur = end;
      }
    }
  }

  // A valid number should have integer or fractional part.
  if (!hasIntPart && !hasFracPart)
    return 0.0;

  // Parse optional exponent
  if (*cur == 'e' || *cur == 'E') {
    long expPart = 0;
    cur++;                            // skip 'E'
    expPart = strtol(cur, &end, 10);  // Parse digit sequence with sign
    if (cur != end) {
      res *= pow(10.0, (double)expPart);
    }
  }

  return res * sign;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
