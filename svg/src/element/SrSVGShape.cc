// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGShape.h"

#include <ctype.h>
#include <math.h>

#include "canvas/SrCanvas.h"
#include "element/SrSVGMarker.h"
#include "element/SrSVGTypes.h"
#include "parser/SrSVGTraversalState.h"

namespace serval {
namespace svg {
namespace element {

namespace {

class ScopedRenderState {
 public:
  explicit ScopedRenderState(SrSVGRenderState& state)
      : state_(state), saved_(state) {}
  ~ScopedRenderState() { state_ = saved_; }
  ScopedRenderState(const ScopedRenderState&) = delete;
  ScopedRenderState& operator=(const ScopedRenderState&) = delete;

 private:
  SrSVGRenderState& state_;
  SrSVGRenderState saved_;
};

}  // namespace

static inline uint32_t ResolveEffectiveColor(
    const SrSVGNode& node, const SrSVGRenderContext& context) {
  if (node.color_) {
    return node.color_->color;
  }
  if (node.inherit_color_) {
    return node.inherit_color_->color;
  }
  if (context.has_default_color) {
    return context.default_color;
  }
  return NSVG_RGBA(0, 0, 0, 255);
}

void SrSVGShape::AppendChild(SrSVGNodeBase* node) {
  // SVGShape should not have child.
}

void SrSVGShape::OnRender(canvas::SrCanvas* canvas,
                          SrSVGRenderContext& context) {
  // Recursive marker references must restore the enclosing invocation's state.
  ScopedRenderState scoped_render_state(render_state_);
  render_state_.fill =
      ResolvePaint(&SrSVGNode::fill_, &SrSVGNode::inherit_fill_paint_);
  render_state_.stroke =
      ResolvePaint(&SrSVGNode::stroke_, &SrSVGNode::inherit_stroke_paint_);

  // Resolve context paints without modifying the definition or inheriting the
  // referencing shape's ordinary style into the marker.
  SrSVGPaint no_paint{};
  no_paint.type = SERVAL_PAINT_NONE;
  auto context_paint = [&](void* value) -> SrSVGPaint* {
    auto* paint = static_cast<SrSVGPaint*>(value);
    // Paint servers need the context element's bounding box and coordinate
    // system. Do not incorrectly resolve them against the marker geometry.
    // Solid colors and none are supported until that backend bridge exists.
    return paint && paint->type != SERVAL_PAINT_IRI ? paint : &no_paint;
  };
  auto resolve_context_paint = [&](SrSVGPaint* paint) -> SrSVGPaint* {
    if (!paint) {
      return nullptr;
    }
    if (paint->type == SERVAL_PAINT_CONTEXT_FILL) {
      return context_paint(context.context_fill);
    }
    if (paint->type == SERVAL_PAINT_CONTEXT_STROKE) {
      return context_paint(context.context_stroke);
    }
    return paint;
  };
  render_state_.fill = resolve_context_paint(render_state_.fill);
  render_state_.stroke = resolve_context_paint(render_state_.stroke);

  if (render_state_.fill && render_state_.fill->type == SERVAL_PAINT_COLOR) {
    SrSVGColor& svgColor = render_state_.fill->content.color;
    if (svgColor.type == SERVAL_CURRENT_COLOR) {
      svgColor.color = ResolveEffectiveColor(*this, context);
    }
  }

  if (render_state_.stroke &&
      render_state_.stroke->type == SERVAL_PAINT_COLOR) {
    SrSVGColor& svgColor = render_state_.stroke->content.color;
    if (svgColor.type == SERVAL_CURRENT_COLOR) {
      svgColor.color = ResolveEffectiveColor(*this, context);
    }
  }

  if (stroke_width_) {
    render_state_.stroke_width = convert_serval_length_to_float(
        &(*stroke_width_), &context, SR_SVG_LENGTH_TYPE_OTHER);
  } else if (inherit_stroke_width_) {
    render_state_.stroke_width = convert_serval_length_to_float(
        &(*inherit_stroke_width_), &context, SR_SVG_LENGTH_TYPE_OTHER);
  } else {
    SrSVGLength stroke_width{.value = 1.0f, .unit = SR_SVG_UNITS_PX};
    render_state_.stroke_width = convert_serval_length_to_float(
        &stroke_width, &context, SR_SVG_LENGTH_TYPE_OTHER);
  }

  if (render_state_.stroke_width <= 0.f)
    render_state_.stroke = nullptr;

  const float shape_opacity = SrSVGNode::ClampOpacity(opacity_.value_or(1.f));
  if (shape_opacity <= 0.f) {
    return;
  }
  const auto markers = SrSVGMarker::ResolveMarkers(*this, context);
  const bool isolate_opacity = markers.has_references && shape_opacity < 1.f;
  const float marker_opacity =
      context.marker_fallback_opacity ? *context.marker_fallback_opacity : 1.f;
  render_state_.opacity =
      (isolate_opacity ? 1.f : shape_opacity) * marker_opacity;

  if (fill_opacity_) {
    render_state_.fill_opacity =
        SrSVGNode::ClampOpacity(fill_opacity_.value_or(1.0)) *
        render_state_.opacity;
  } else if (inherit_fill_opacity_) {
    render_state_.fill_opacity =
        SrSVGNode::ClampOpacity(inherit_fill_opacity_.value_or(1.0)) *
        render_state_.opacity;
  } else {
    render_state_.fill_opacity = render_state_.opacity;
  }

  if (stroke_opacity_) {
    render_state_.stroke_opacity =
        SrSVGNode::ClampOpacity(stroke_opacity_.value_or(1.0)) *
        render_state_.opacity;
  } else if (inherit_stroke_opacity_) {
    render_state_.stroke_opacity =
        SrSVGNode::ClampOpacity(inherit_stroke_opacity_.value_or(1.0)) *
        render_state_.opacity;
  } else {
    render_state_.stroke_opacity = render_state_.opacity;
  }
  render_state_.fill_rule = fill_rule_;
  render_state_.vector_effect = vector_effect_;

  size_t dash_array_length = stroke_dash_array_.size();
  float* dash_array = stroke_dash_array_.data();
  float dash_offset = 0.f;
  if (stroke_dash_offset_) {
    resolve_serval_length(&*stroke_dash_offset_, &context,
                          SR_SVG_LENGTH_TYPE_OTHER, &dash_offset);
  }

  SRSVGStrokeState stroke_state = {.stroke_line_join = stroke_join_,
                                   .stroke_line_cap = stroke_cap_,
                                   .stroke_miter_limit = stoke_miter_limit_,
                                   .stroke_dash_offset = dash_offset,
                                   .dash_array = dash_array,
                                   .dash_array_length = dash_array_length};

  render_state_.stroke_state = &stroke_state;

  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  parser::SrSVGOffscreenScope opacity_budget(context);
  const bool has_opacity_layer =
      isolate_opacity && opacity_budget.Reserve(*canvas, 1) &&
      canvas->TryBeginOpacityLayer(nullptr, shape_opacity);
  if (isolate_opacity && !has_opacity_layer) {
    render_state_.opacity *= shape_opacity;
    render_state_.fill_opacity *= shape_opacity;
    render_state_.stroke_opacity *= shape_opacity;
  }
  this->onDraw(canvas, context);
  SrSVGMarker::RenderMarkers(canvas, context, *this, render_state_, markers);
  if (has_opacity_layer) {
    canvas->EndOpacityLayer();
  }
}

bool SrSVGShape::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "fill-rule") == 0) {
    if (strcmp(value, "evenodd") == 0) {
      fill_rule_ = SR_SVG_EO_FILL;
    } else if (strcmp(value, "nonzero") == 0) {
      fill_rule_ = SR_SVG_FILL;
    } else {
      return false;
    }
    return true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGShape::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  // TODO(renzhongyue): to be implemented in basic shapes.
  return path_factory->CreateMutable();
}

}  // namespace element
}  // namespace svg
}  // namespace serval
