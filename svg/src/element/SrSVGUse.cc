// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGUse.h"
#include <optional>

#include "canvas/SrCanvas.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif  // __ANDROID__

namespace serval {
namespace svg {
namespace element {

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
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGUse::AppendChild(SrSVGNodeBase*) {
  // Do nothing, <use> should not append any child.
}

bool SrSVGUse::OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  return true;
}

void SrSVGUse::OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) {
  IDMapper* id_mapper = static_cast<IDMapper*>(context.id_mapper);
  if (!href_.empty()) {
    SrSVGNodeBase* render_target = (*id_mapper)[href_];
    if (render_target) {
      renderRealNode(render_target, canvas, context);
    }
  }
}

std::unique_ptr<canvas::Path> SrSVGUse::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  IDMapper* id_mapper = static_cast<IDMapper*>(context->id_mapper);
  if (!href_.empty()) {
    SrSVGNodeBase* target = (*id_mapper)[href_];
    if (target) {
      return target->AsPath(path_factory, context);
    }
  }
  return nullptr;
}

void SrSVGUse::renderRealNode(SrSVGNodeBase* nodeBase, canvas::SrCanvas* canvas,
                              SrSVGRenderContext& context) {
  if (!nodeBase || !nodeBase->IsSVGNode()) {
    return;
  }
  SrSVGNode* node = static_cast<SrSVGNode*>(nodeBase);
  SrSVGPaint* local_fill_paint = node->inherit_fill_paint_;
  SrSVGPaint* local_stroke_paint = node->inherit_stroke_paint_;
  SrSVGPaint* local_clip_path = node->inherit_clip_path_;
  std::optional<SrSVGLength> local_stroke_width = node->inherit_stroke_width_;
  std::optional<float> local_opacity = node->inherit_opacity_;
  std::optional<float> local_fill_opacity = node->inherit_fill_opacity_;
  std::optional<float> local_stroke_opacity = node->inherit_stroke_opacity_;
  std::optional<SrSVGColor> local_color = node->inherit_color_;

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
    node->inherit_fill_paint_ = stroke_;
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

  if (node->stroke_width_) {
    node->inherit_stroke_width_ = node->stroke_width_;
  } else if (stroke_width_) {
    node->inherit_stroke_width_ = stroke_width_;
  } else if (inherit_stroke_width_) {
    node->inherit_stroke_width_ = stroke_width_;
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

  float x = convert_serval_length_to_float(&x_, &context,
                                           SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float y = convert_serval_length_to_float(&y_, &context,
                                           SR_SVG_LENGTH_TYPE_VERTICAL);
  canvas->Transform(transform_);
  canvas->Translate(x, y);

  if (node->color_) {
    node->inherit_color_ = node->color_;
  } else if (color_) {
    node->inherit_color_ = color_;
  } else if (inherit_color_) {
    node->inherit_color_ = inherit_color_;
  }

  // render the real node
  node->Render(canvas, context);

  node->inherit_fill_paint_ = local_fill_paint;
  node->inherit_stroke_paint_ = local_stroke_paint;
  node->inherit_fill_opacity_ = local_fill_opacity;
  node->inherit_opacity_ = local_opacity;
  node->inherit_stroke_opacity_ = local_stroke_opacity;
  node->inherit_stroke_width_ = local_stroke_width;
  node->inherit_clip_path_ = local_clip_path;
  node->inherit_color_ = local_color;
}

bool SrSVGUse::HasChildren() const {
  return false;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
