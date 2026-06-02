// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGShape.h"

#include <ctype.h>
#include <math.h>

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

const uint8_t SrSVGShape::kRenderTypeFlagStroke = 1;
const uint8_t SrSVGShape::kRenderTypeFlagFill = 1 << 1;
const uint8_t SrSVGShape::kRenderTypeFillRule = 1 << 2;

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
  if (fill_) {
    render_state_.fill = fill_;
  } else if (inherit_fill_paint_) {
    render_state_.fill = inherit_fill_paint_;
  } else {
    render_state_.fill = nullptr;
  }

  if (stroke_) {
    render_state_.stroke = stroke_;
  } else if (inherit_stroke_paint_) {
    render_state_.stroke = inherit_stroke_paint_;
  } else {
    render_state_.stroke = nullptr;
  }

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

  if (opacity_) {
    render_state_.opacity = SrSVGNode::ClampOpacity(opacity_.value_or(1.0));
  } else {
    render_state_.opacity = 1.0;
  }

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

  SRSVGStrokeState stroke_state = {.stroke_line_join = stroke_join_,
                                   .stroke_line_cap = stroke_cap_,
                                   .stroke_miter_limit = stoke_miter_limit_,
                                   .stroke_dash_offset = stroke_dash_offset_,
                                   .dash_array = dash_array,
                                   .dash_array_length = dash_array_length};

  render_state_.stroke_state = &stroke_state;

  float xform[6];
  ResolvedTransform(xform, context, canvas->PathFactory());
  canvas->Transform(xform);
  this->onDraw(canvas, context);
}

bool SrSVGShape::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "fill-rule") == 0) {
    if (strcmp(value, "evenodd") == 0) {
      this->fill_rule_ = SR_SVG_EO_FILL;
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
