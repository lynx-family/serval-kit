// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

namespace {

bool IsContainerTag(SrSVGTag tag) {
  switch (tag) {
    case SrSVGTag::kSvg:
    case SrSVGTag::kG:
    case SrSVGTag::kDefs:
    case SrSVGTag::kClipPath:
    case SrSVGTag::kMask:
    case SrSVGTag::kFilter:
    case SrSVGTag::kPattern:
    case SrSVGTag::kLinearGradient:
    case SrSVGTag::kRadialGradient:
      return true;
    default:
      return false;
  }
}

}  // namespace

bool SrSVGContainer::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "transform") == 0) {
    ParseTransform(value, transform_);
    return true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGContainer::OnRender(canvas::SrCanvas* canvas,
                              SrSVGRenderContext& context) {
  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  const float group_opacity =
      opacity_ ? SrSVGNode::ClampOpacity(opacity_.value_or(1.f)) : 1.f;
  if (opacity_ && group_opacity <= 0.f) {
    return;
  }
  const bool has_opacity_layer = opacity_ && group_opacity < 1.f;
  if (has_opacity_layer) {
    // TODO: Compute tight layer bounds for group opacity. Correctness requires
    // whole-group composition; passing nullptr keeps the result correct but may
    // allocate a larger offscreen layer than necessary.
    canvas->BeginOpacityLayer(nullptr, group_opacity);
  }
  for (SrSVGNodeBase* child : children_) {
    RenderChild(canvas, context, child);
  }
  if (has_opacity_layer) {
    canvas->EndOpacityLayer();
  }
}

void SrSVGContainer::RenderChild(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context,
                                 SrSVGNodeBase* child) {
  if (!PrepareChild(child)) {
    return;
  }
  child->Render(canvas, context);
  RestoreChild(child);
}

bool SrSVGContainer::PrepareChild(SrSVGNodeBase* child) {
  if (!child || !child->IsSVGNode()) {
    return false;
  }
  auto node = static_cast<SrSVGNode*>(child);
  child_render_state_stack_.push_back(ChildRenderState{
      node->inherit_fill_paint_,
      node->inherit_stroke_paint_,
      node->inherit_clip_path_,
      node->inherit_mask_,
      node->inherit_stroke_width_,
      node->inherit_fill_opacity_,
      node->inherit_stroke_opacity_,
      node->inherit_color_,
  });

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

  if (node->color_) {
    node->inherit_color_ = node->color_;
  } else if (color_) {
    node->inherit_color_ = color_;
  } else if (inherit_color_) {
    node->inherit_color_ = inherit_color_;
  }

  return true;
}

void SrSVGContainer::RestoreChild(SrSVGNodeBase* child) {
  if (!child || !child->IsSVGNode() || child_render_state_stack_.empty()) {
    return;
  }
  auto node = static_cast<SrSVGNode*>(child);
  const ChildRenderState state = child_render_state_stack_.back();
  child_render_state_stack_.pop_back();
  node->inherit_fill_paint_ = state.fill_paint;
  node->inherit_stroke_paint_ = state.stroke_paint;
  node->inherit_clip_path_ = state.clip_path;
  node->inherit_mask_ = state.mask;
  node->inherit_fill_opacity_ = state.fill_opacity;
  node->inherit_stroke_opacity_ = state.stroke_opacity;
  node->inherit_stroke_width_ = state.stroke_width;
  node->inherit_color_ = state.color;
}

size_t SrSVGContainer::ChildCount() const {
  return children_.size();
}

bool SrSVGContainer::RenderChildAt(canvas::SrCanvas* canvas,
                                   SrSVGRenderContext& context, size_t index) {
  if (index >= children_.size()) {
    return false;
  }
  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  RenderChild(canvas, context, children_[index]);
  return true;
}

bool SrSVGContainer::RenderChildPathAt(canvas::SrCanvas* canvas,
                                       SrSVGRenderContext& context,
                                       const std::vector<size_t>& path,
                                       size_t depth) {
  if (depth >= path.size() || path[depth] >= children_.size()) {
    return false;
  }
  if (depth + 1 == path.size()) {
    float xform[6];
    ResolvedTransform(xform, context, canvas->PathFactory());
    canvas->Transform(xform);
    RenderChild(canvas, context, children_[path[depth]]);
    return true;
  }

  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);

  SrSVGNodeBase* child = children_[path[depth]];
  if (!PrepareChild(child)) {
    return false;
  }
  bool rendered = false;
  if (IsContainerTag(child->Tag())) {
    auto* container = static_cast<SrSVGContainer*>(child);
    rendered = container->RenderChildPathAt(canvas, context, path, depth + 1);
  }
  RestoreChild(child);
  return rendered;
}

std::unique_ptr<canvas::Path> SrSVGContainer::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  std::unique_ptr<canvas::Path> path = path_factory->CreateMutable();
  for (SrSVGNodeBase* child : children_) {
    if (child) {
      auto child_path = child->AsPath(path_factory, context);
      if (child_path) {
        path_factory->Op(path.get(), child_path.get(), canvas::OP::UNION);
      }
    }
  }
  if (include_transform && path) {
    float xform[6];
    ResolvedTransform(xform, *context, path_factory);
    path->Transform(xform);
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
