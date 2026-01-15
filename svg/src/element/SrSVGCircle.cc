// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGCircle.h"

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGCircle::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "cx") == 0) {
    cx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "cy") == 0) {
    cy_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "r") == 0) {
    r_ = make_serval_length(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

void SrSVGCircle::onDraw(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const {
  float center_x = convert_serval_length_to_float(
      &this->cx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float center_y = convert_serval_length_to_float(&cy_, &context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  float radius =
      convert_serval_length_to_float(&r_, &context, SR_SVG_LENGTH_TYPE_OTHER);
  uint8_t type = 0;
  if (this->stroke_ && this->stroke_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagStroke;
  }
  if (this->fill_ && this->fill_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagFill;
  }
  canvas->DrawCircle(id_.c_str(), center_x, center_y, radius, render_state_);
}

std::unique_ptr<canvas::Path> SrSVGCircle::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  float center_x = convert_serval_length_to_float(
      &cx_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float center_y = convert_serval_length_to_float(&cy_, context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  float radius =
      convert_serval_length_to_float(&r_, context, SR_SVG_LENGTH_TYPE_OTHER);
  auto path = path_factory->CreateCircle(center_x, center_y, radius);
  if (path) {
    path->Transform(transform_);
  }
  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
