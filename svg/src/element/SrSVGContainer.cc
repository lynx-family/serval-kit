// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGContainer.h"

#include "parser/SrSVGTraversalState.h"

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
    case SrSVGTag::kMarker:
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
  parser::SrSVGOffscreenScope opacity_budget(context);
  const bool has_opacity_layer =
      opacity_ && group_opacity < 1.f && opacity_budget.Reserve(*canvas, 1) &&
      canvas->TryBeginOpacityLayer(nullptr, group_opacity);
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
  if (!child || !child->IsSVGNode()) {
    return;
  }
  ScopedInheritance inheritance(*static_cast<SrSVGNode*>(child), *this);
  child->Render(canvas, context);
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
  if (!child || !child->IsSVGNode()) {
    return false;
  }
  ScopedInheritance inheritance(*static_cast<SrSVGNode*>(child), *this);
  bool rendered = false;
  if (IsContainerTag(child->Tag())) {
    auto* container = static_cast<SrSVGContainer*>(child);
    rendered = container->RenderChildPathAt(canvas, context, path, depth + 1);
  }
  return rendered;
}

std::unique_ptr<canvas::Path> SrSVGContainer::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  parser::SrSVGRenderScope scope(context);
  if (!path_factory || !context || !scope)
    return nullptr;
  std::unique_ptr<canvas::Path> path = path_factory->CreateMutable();
  if (!path)
    return nullptr;
  for (SrSVGNodeBase* child : children_) {
    parser::SrSVGRenderScope child_scope(context);
    if (!child_scope)
      break;
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
