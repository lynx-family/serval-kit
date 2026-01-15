// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGContainer::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "transform") == 0) {
    ParseTransform(value, transform_);
    return true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGContainer::OnRender(canvas::SrCanvas* canvas,
                              SrSVGRenderContext& context) {
  canvas->Transform(transform_);
  for (SrSVGNodeBase* child : children_) {
    if (child) {
      auto node = static_cast<SrSVGNode*>(child);
      SrSVGPaint* local_fill_paint = node->inherit_fill_paint_;
      SrSVGPaint* local_stroke_paint = node->inherit_stroke_paint_;
      SrSVGPaint* local_clip_path = node->inherit_clip_path_;
      std::optional<SrSVGLength> local_stroke_width =
          node->inherit_stroke_width_;
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
      } else {
      }

      if (node->stroke_opacity_) {
        node->inherit_stroke_opacity_ = node->stroke_opacity_;
      } else if (stroke_opacity_) {
        node->inherit_stroke_opacity_ = stroke_opacity_;
      } else if (inherit_stroke_opacity_) {
        node->inherit_stroke_opacity_ = inherit_stroke_opacity_;
      }

      if (node->color_) {
        node->inherit_color_ = node->color_;
      } else if (color_) {
        node->inherit_color_ = color_;
      } else if (inherit_color_) {
        node->inherit_color_ = inherit_color_;
      }

      child->Render(canvas, context);

      node->inherit_fill_paint_ = local_fill_paint;
      node->inherit_stroke_paint_ = local_stroke_paint;
      node->inherit_clip_path_ = local_clip_path;
      node->inherit_fill_opacity_ = local_fill_opacity;
      node->inherit_opacity_ = local_opacity;
      node->inherit_stroke_opacity_ = local_stroke_opacity;
      node->inherit_stroke_width_ = local_stroke_width;
      node->inherit_color_ = local_color;
    }
  }
}

std::unique_ptr<canvas::Path> SrSVGContainer::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  std::unique_ptr<canvas::Path> path = path_factory->CreateMutable();
  for (SrSVGNodeBase* child : children_) {
    if (child) {
      path_factory->Op(path.get(), child->AsPath(path_factory, context).get(),
                       canvas::OP::UNION);
    }
  }
  return path;
}

void SrSVGContainer::AppendChild(SrSVGNodeBase* node) {
  children_.push_back(node);
}

SrSVGContainer::~SrSVGContainer() = default;

bool SrSVGContainer::HasChildren() const {
  return !children_.empty();
}

}  // namespace element
}  // namespace svg
}  // namespace serval
