// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGUse.h"
#include <cstring>

#include "canvas/SrCanvas.h"
#include "parser/SrSVGTraversalState.h"
#include "utils/SrSVGPatternUtils.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif  // __ANDROID__

namespace serval {
namespace svg {
namespace element {

namespace {

bool IsSupportedUseTarget(SrSVGTag tag) {
  return tag != SrSVGTag::kSvg && tag != SrSVGTag::kMarker;
}

parser::SrSVGTraversalState* GetTraversalState(SrSVGRenderContext& context) {
  return static_cast<parser::SrSVGTraversalState*>(context.traversal_state);
}

parser::SrSVGTraversalState* GetTraversalState(
    const SrSVGRenderContext* context) {
  return context ? static_cast<parser::SrSVGTraversalState*>(
                       context->traversal_state)
                 : nullptr;
}

class UseReferenceScope {
 public:
  UseReferenceScope(parser::SrSVGTraversalState& state, const std::string& href)
      : state_(state) {
    auto& refs = state_.active_use_ids;
    for (size_t depth = 0; depth < state_.use_depth; ++depth) {
      if (*refs[depth] == href) {
        cycle_ = true;
        return;
      }
    }
    if (state_.use_depth == refs.size())
      return;
    refs[state_.use_depth++] = &href;
    entered_ = true;
  }
  UseReferenceScope(const UseReferenceScope&) = delete;
  UseReferenceScope& operator=(const UseReferenceScope&) = delete;
  ~UseReferenceScope() {
    if (entered_)
      --state_.use_depth;
  }
  explicit operator bool() const { return entered_; }
  bool IsCycle() const { return cycle_; }

 private:
  parser::SrSVGTraversalState& state_;
  bool entered_{false};
  bool cycle_{false};
};

void ReportUseCycle(parser::SrSVGTraversalState* state, const std::string& href,
                    const char* message) {
  if (!state) {
    return;
  }
  state->Report(SR_SVG_DIAGNOSTIC_USE_REFERENCE_CYCLE, message, href.c_str(),
                false);
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
    return ParseLengthAttribute(value, x_);
  } else if (strcmp(name, "y") == 0) {
    return ParseLengthAttribute(value, y_);
  } else if (strcmp(name, "width") == 0) {
    return ParseLengthAttribute(value, width_, LengthRange::kNonNegative);
  } else if (strcmp(name, "height") == 0) {
    return ParseLengthAttribute(value, height_, LengthRange::kNonNegative);
  } else if (strcmp(name, "stroke-linecap") == 0) {
    if (!SrSVGNode::ParseAndSetAttribute(name, value))
      return false;
    has_stroke_cap_ = true;
    return true;
  } else if (strcmp(name, "stroke-linejoin") == 0) {
    if (!SrSVGNode::ParseAndSetAttribute(name, value))
      return false;
    has_stroke_join_ = true;
    return true;
  } else if (strcmp(name, "stroke-miterlimit") == 0) {
    if (!SrSVGNode::ParseAndSetAttribute(name, value))
      return false;
    has_stroke_miter_limit_ = true;
    return true;
  } else if (strcmp(name, "stroke-dasharray") == 0) {
    if (!SrSVGNode::ParseAndSetAttribute(name, value))
      return false;
    has_stroke_dash_array_ = true;
    return true;
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
  UseReferenceScope reference(*traversal_state, href_);
  if (!reference) {
    if (reference.IsCycle())
      ReportUseCycle(traversal_state, href_,
                     "Skipped recursive <use> reference.");
    return;
  }

  auto it = id_mapper->find(href_);
  if (it != id_mapper->end() && it->second) {
    renderRealNode(it->second, canvas, context);
  }
}

std::unique_ptr<canvas::Path> SrSVGUse::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  parser::SrSVGRenderScope scope(context);
  if (!context || !scope)
    return nullptr;
  IDMapper* id_mapper = static_cast<IDMapper*>(context->id_mapper);
  auto* traversal_state = GetTraversalState(context);
  if (!id_mapper || href_.empty() || !traversal_state) {
    return nullptr;
  }
  UseReferenceScope reference(*traversal_state, href_);
  if (!reference) {
    if (reference.IsCycle())
      ReportUseCycle(traversal_state, href_,
                     "Skipped recursive <use> path expansion.");
    return nullptr;
  }

  auto it = id_mapper->find(href_);
  if (it != id_mapper->end() && it->second) {
    SrSVGNodeBase* referenced_node = it->second;
    if (!IsSupportedUseTarget(referenced_node->Tag())) {
      return nullptr;
    }

    auto path = referenced_node->AsPath(path_factory, context);
    if (!path) {
      return nullptr;
    }
    float x, y;
    if (!resolve_serval_length(&x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                               &x) ||
        !resolve_serval_length(&y_, context, SR_SVG_LENGTH_TYPE_VERTICAL, &y))
      return nullptr;
    float use_transform[6];
    xform_set_translation(use_transform, x, y);
    if (include_transform) {
      float form[6];
      ResolvedTransform(form, *context, path_factory);
      if (!ComposeAffineTransform(form, use_transform, use_transform))
        return nullptr;
    }
    SrSVGBox bounds;
    if (!TryMapBounds(path->GetBounds(), use_transform, &bounds))
      return nullptr;
    return path->CreateTransformCopy(use_transform);
  }
  return nullptr;
}

void SrSVGUse::renderRealNode(SrSVGNodeBase* nodeBase, canvas::SrCanvas* canvas,
                              SrSVGRenderContext& context) {
  if (!nodeBase || !nodeBase->IsSVGNode() ||
      !IsSupportedUseTarget(nodeBase->Tag())) {
    return;
  }
  float x, y, form[6], translation[6], use_transform[6];
  if (!resolve_serval_length(&x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &x) ||
      !resolve_serval_length(&y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &y))
    return;
  ResolvedTransform(form, context, canvas->PathFactory());
  xform_set_translation(translation, x, y);
  if (!ComposeAffineTransform(form, translation, use_transform))
    return;
  SrSVGNode* node = static_cast<SrSVGNode*>(nodeBase);
  ScopedInheritance inheritance(*node, *this);

  canvas->Transform(use_transform);

  const float use_opacity =
      opacity_ ? SrSVGNode::ClampOpacity(opacity_.value_or(1.f)) : 1.f;
  if (opacity_ && use_opacity <= 0.f) {
    return;
  }
  parser::SrSVGOffscreenScope opacity_budget(context);
  const bool has_opacity_layer =
      opacity_ && use_opacity < 1.f && opacity_budget.Reserve(*canvas, 1) &&
      canvas->TryBeginOpacityLayer(nullptr, use_opacity);

  SrSVGStrokeCap local_stroke_cap = node->stroke_cap_;
  SrSVGStrokeJoin local_stroke_join = node->stroke_join_;
  float local_stroke_miter_limit = node->stoke_miter_limit_;
  const auto local_stroke_dash_offset = node->stroke_dash_offset_;
  std::vector<float> local_stroke_dash_array = node->stroke_dash_array_;
  if (has_stroke_cap_) {
    node->stroke_cap_ = stroke_cap_;
  }
  if (has_stroke_join_) {
    node->stroke_join_ = stroke_join_;
  }
  if (has_stroke_miter_limit_) {
    node->stoke_miter_limit_ = stoke_miter_limit_;
  }
  if (stroke_dash_offset_) {
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
