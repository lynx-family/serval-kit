// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGUse.h"
#include <cstring>
#include <optional>

#include "canvas/SrCanvas.h"
#include "parser/SrSVGTraversalState.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif  // __ANDROID__

namespace serval {
namespace svg {
namespace element {

namespace {

parser::SrSVGTraversalState* GetTraversalState(SrSVGRenderContext& context) {
  return static_cast<parser::SrSVGTraversalState*>(context.traversal_state);
}

parser::SrSVGTraversalState* GetTraversalState(
    const SrSVGRenderContext* context) {
  return context ? static_cast<parser::SrSVGTraversalState*>(
                       context->traversal_state)
                 : nullptr;
}

bool TryEnterUseReference(parser::SrSVGTraversalState* state,
                          const std::string& href) {
  return state && !href.empty() && state->active_use_ids.insert(href).second;
}

void LeaveUseReference(parser::SrSVGTraversalState* state,
                       const std::string& href) {
  if (!state || href.empty()) {
    return;
  }
  state->active_use_ids.erase(href);
}

void ReportUseCycle(parser::SrSVGTraversalState* state, const std::string& href,
                    const char* message) {
  if (!state) {
    return;
  }
  state->Report(SR_SVG_DIAGNOSTIC_USE_REFERENCE_CYCLE, message, href.c_str(),
                false);
}

float ResolveUseLength(const SrSVGLength& length, SrSVGRenderContext* context,
                       SrSVGLengthType length_type) {
  if (!context) {
    return length.value;
  }
  return convert_serval_length_to_float(&length, context, length_type);
}

void BuildUseTransform(float x, float y, const float (&use_transform)[6],
                       float (&out)[6]) {
  memcpy(out, use_transform, sizeof(float) * 6);
  xform_pre_translate(out, x, y);
}

}  // namespace

bool SrSVGUse::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "href") == 0) {
    if (value[0] == '#') {
      // only support in doc reference begin with #.
      href_ = std::string(value + 1);
    }
    return true;
  } else if (strcmp(name, "xlink:href") == 0) {
    if (value[0] == '#') {
      // only support in doc reference begin with #.
      href_ = std::string(value + 1);
    }
    return true;
  } else if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "stroke-linecap") == 0) {
    has_stroke_cap_ = true;
  } else if (strcmp(name, "stroke-linejoin") == 0) {
    has_stroke_join_ = true;
  } else if (strcmp(name, "stroke-miterlimit") == 0) {
    has_stroke_miter_limit_ = true;
  } else if (strcmp(name, "stroke-dashoffset") == 0) {
    has_stroke_dash_offset_ = true;
  } else if (strcmp(name, "stroke-dasharray") == 0) {
    has_stroke_dash_array_ = true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGUse::AppendChild(SrSVGNodeBase*) {
  // Do nothing, <use> should not append any child.
}

bool SrSVGUse::OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  SrSVGNode::OnPrepareToRender(canvas, context);
  return true;
}

void SrSVGUse::OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) {
  IDMapper* id_mapper = static_cast<IDMapper*>(context.id_mapper);
  auto* traversal_state = GetTraversalState(context);
  if (!id_mapper || href_.empty() || !traversal_state) {
    return;
  }
  if (!TryEnterUseReference(traversal_state, href_)) {
    ReportUseCycle(traversal_state, href_,
                   "Skipped recursive <use> reference.");
    return;
  }

  auto it = id_mapper->find(href_);
  if (it != id_mapper->end() && it->second) {
    renderRealNode(it->second, canvas, context);
  }
  LeaveUseReference(traversal_state, href_);
}

std::unique_ptr<canvas::Path> SrSVGUse::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  if (!context) {
    return nullptr;
  }
  IDMapper* id_mapper = static_cast<IDMapper*>(context->id_mapper);
  auto* traversal_state = GetTraversalState(context);
  if (!id_mapper || href_.empty() || !traversal_state) {
    return nullptr;
  }
  if (!TryEnterUseReference(traversal_state, href_)) {
    ReportUseCycle(traversal_state, href_,
                   "Skipped recursive <use> path expansion.");
    return nullptr;
  }

  auto it = id_mapper->find(href_);
  if (it != id_mapper->end() && it->second) {
    SrSVGNodeBase* referenced_node = it->second;
    if (referenced_node->Tag() == SrSVGTag::kSvg) {
      LeaveUseReference(traversal_state, href_);
      return nullptr;
    }

    auto path = referenced_node->AsPath(path_factory, context);
    LeaveUseReference(traversal_state, href_);
    if (!path) {
      return nullptr;
    }
    const float x =
        ResolveUseLength(x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
    const float y = ResolveUseLength(y_, context, SR_SVG_LENGTH_TYPE_VERTICAL);
    float use_transform[6];
    if (include_transform) {
      BuildUseTransform(x, y, transform_, use_transform);
    } else {
      xform_set_translation(use_transform, x, y);
    }
    return path->CreateTransformCopy(use_transform);
  }
  LeaveUseReference(traversal_state, href_);
  return nullptr;
}

void SrSVGUse::renderRealNode(SrSVGNodeBase* nodeBase, canvas::SrCanvas* canvas,
                              SrSVGRenderContext& context) {
  if (!nodeBase || !nodeBase->IsSVGNode()) {
    return;
  }
  if (nodeBase->Tag() == SrSVGTag::kSvg) {
    return;
  }
  SrSVGNode* node = static_cast<SrSVGNode*>(nodeBase);
  SrSVGPaint* local_fill_paint = node->inherit_fill_paint_;
  SrSVGPaint* local_stroke_paint = node->inherit_stroke_paint_;
  SrSVGPaint* local_clip_path = node->inherit_clip_path_;
  SrSVGPaint* local_mask = node->inherit_mask_;
  std::optional<SrSVGLength> local_stroke_width = node->inherit_stroke_width_;
  std::optional<float> local_fill_opacity = node->inherit_fill_opacity_;
  std::optional<float> local_stroke_opacity = node->inherit_stroke_opacity_;
  std::optional<SrSVGColor> local_color = node->inherit_color_;
  SrSVGStrokeCap local_stroke_cap = node->stroke_cap_;
  SrSVGStrokeJoin local_stroke_join = node->stroke_join_;
  float local_stroke_miter_limit = node->stoke_miter_limit_;
  float local_stroke_dash_offset = node->stroke_dash_offset_;
  std::vector<float> local_stroke_dash_array = node->stroke_dash_array_;

  if (node->fill_) {
    node->inherit_fill_paint_ = node->fill_;
  } else if (fill_) {
    node->inherit_fill_paint_ = fill_;
  } else if (inherit_fill_paint_) {
    node->inherit_fill_paint_ = inherit_fill_paint_;
  }

  if (node->stroke_) {
    node->inherit_stroke_paint_ = node->stroke_;
  } else if (stroke_) {
    node->inherit_stroke_paint_ = stroke_;
  } else if (inherit_stroke_paint_) {
    node->inherit_stroke_paint_ = inherit_stroke_paint_;
  }

  if (node->clip_path_) {
    node->inherit_clip_path_ = node->clip_path_;
  } else if (clip_path_) {
    node->inherit_clip_path_ = clip_path_;
  } else if (inherit_clip_path_) {
    node->inherit_clip_path_ = inherit_clip_path_;
  }

  if (node->mask_) {
    node->inherit_mask_ = node->mask_;
  } else if (mask_) {
    node->inherit_mask_ = mask_;
  } else if (inherit_mask_) {
    node->inherit_mask_ = inherit_mask_;
  }

  if (node->stroke_width_) {
    node->inherit_stroke_width_ = node->stroke_width_;
  } else if (stroke_width_) {
    node->inherit_stroke_width_ = stroke_width_;
  } else if (inherit_stroke_width_) {
    node->inherit_stroke_width_ = inherit_stroke_width_;
  }

  if (node->fill_opacity_) {
    node->inherit_fill_opacity_ = node->fill_opacity_;
  } else if (fill_opacity_) {
    node->inherit_fill_opacity_ = fill_opacity_;
  } else if (inherit_fill_opacity_) {
    node->inherit_fill_opacity_ = inherit_fill_opacity_;
  }

  if (node->stroke_opacity_) {
    node->inherit_stroke_opacity_ = node->stroke_opacity_;
  } else if (stroke_opacity_) {
    node->inherit_stroke_opacity_ = stroke_opacity_;
  } else if (inherit_stroke_opacity_) {
    node->inherit_stroke_opacity_ = inherit_stroke_opacity_;
  }

  float x = ResolveUseLength(x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float y = ResolveUseLength(y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL);
  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  canvas->Translate(x, y);

  if (node->color_) {
    node->inherit_color_ = node->color_;
  } else if (color_) {
    node->inherit_color_ = color_;
  } else if (inherit_color_) {
    node->inherit_color_ = inherit_color_;
  }

  const float use_opacity =
      opacity_ ? SrSVGNode::ClampOpacity(opacity_.value_or(1.f)) : 1.f;
  if (opacity_ && use_opacity <= 0.f) {
    node->inherit_fill_paint_ = local_fill_paint;
    node->inherit_stroke_paint_ = local_stroke_paint;
    node->inherit_fill_opacity_ = local_fill_opacity;
    node->inherit_stroke_opacity_ = local_stroke_opacity;
    node->inherit_stroke_width_ = local_stroke_width;
    node->inherit_clip_path_ = local_clip_path;
    node->inherit_mask_ = local_mask;
    node->inherit_color_ = local_color;
    return;
  }
  const bool has_opacity_layer = opacity_ && use_opacity < 1.f;
  if (has_opacity_layer) {
    // TODO: Compute tight layer bounds for use opacity. Correctness requires
    // whole-use composition; passing nullptr keeps the result correct but may
    // allocate a larger offscreen layer than necessary.
    canvas->BeginOpacityLayer(nullptr, use_opacity);
  }

  if (has_stroke_cap_) {
    node->stroke_cap_ = stroke_cap_;
  }
  if (has_stroke_join_) {
    node->stroke_join_ = stroke_join_;
  }
  if (has_stroke_miter_limit_) {
    node->stoke_miter_limit_ = stoke_miter_limit_;
  }
  if (has_stroke_dash_offset_) {
    node->stroke_dash_offset_ = stroke_dash_offset_;
  }
  if (has_stroke_dash_array_) {
    node->stroke_dash_array_ = stroke_dash_array_;
  }

  // render the real node
  node->Render(canvas, context);

  if (has_opacity_layer) {
    canvas->EndOpacityLayer();
  }

  node->inherit_fill_paint_ = local_fill_paint;
  node->inherit_stroke_paint_ = local_stroke_paint;
  node->inherit_fill_opacity_ = local_fill_opacity;
  node->inherit_stroke_opacity_ = local_stroke_opacity;
  node->inherit_stroke_width_ = local_stroke_width;
  node->inherit_clip_path_ = local_clip_path;
  node->inherit_mask_ = local_mask;
  node->inherit_color_ = local_color;
  node->stroke_cap_ = local_stroke_cap;
  node->stroke_join_ = local_stroke_join;
  node->stoke_miter_limit_ = local_stroke_miter_limit;
  node->stroke_dash_offset_ = local_stroke_dash_offset;
  node->stroke_dash_array_ = local_stroke_dash_array;
}

bool SrSVGUse::HasChildren() const {
  return false;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
