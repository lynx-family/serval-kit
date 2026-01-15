// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGEllipse.h"

#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGEllipse::onDraw(canvas::SrCanvas* canvas,
                          SrSVGRenderContext& context) const {
  float center_x = convert_serval_length_to_float(
      &cx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float center_y = convert_serval_length_to_float(&cy_, &context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  float radius_x = convert_serval_length_to_float(
      &rx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float radius_y = convert_serval_length_to_float(&ry_, &context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  uint8_t type = 0;
  if (this->stroke_ && this->stroke_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagStroke;
  }
  if (this->fill_ && this->fill_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagFill;
  }
  canvas->DrawEllipse(id_.c_str(), center_x, center_y, radius_x, radius_y,
                      render_state_);
}

std::unique_ptr<canvas::Path> SrSVGEllipse::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  float center_x = convert_serval_length_to_float(
      &cx_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float center_y = convert_serval_length_to_float(&cy_, context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  float radius_x = convert_serval_length_to_float(
      &rx_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float radius_y = convert_serval_length_to_float(&ry_, context,
                                                  SR_SVG_LENGTH_TYPE_VERTICAL);
  auto path =
      path_factory->CreateEllipse(center_x, center_y, radius_x, radius_y);
  if (path) {
    path->Transform(transform_);
  }
  return path;
};

bool SrSVGEllipse::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "cx") == 0) {
    cx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "cy") == 0) {
    cy_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "rx") == 0) {
    rx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "ry") == 0) {
    ry_ = make_serval_length(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
