// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGLine.h"

#include "canvas/SrCanvas.h"
#include "element/SrSVGShape.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGLine::onDraw(canvas::SrCanvas* canvas,
                       SrSVGRenderContext& context) const {
  float x1 = convert_serval_length_to_float(&x1_, &context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float x2 = convert_serval_length_to_float(&x2_, &context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float y1 = convert_serval_length_to_float(&y1_, &context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  float y2 = convert_serval_length_to_float(&y2_, &context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  if (stroke_ && stroke_->type != SERVAL_PAINT_NONE) {
    canvas->DrawLine(id_.c_str(), x1, y1, x2, y2, render_state_);
  }
}

bool SrSVGLine::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x1") == 0) {
    x1_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y1") == 0) {
    y1_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "x2") == 0) {
    x2_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y2") == 0) {
    y2_ = make_serval_length(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGLine::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  float x1 = convert_serval_length_to_float(&x1_, context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float x2 = convert_serval_length_to_float(&x2_, context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float y1 = convert_serval_length_to_float(&y1_, context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  float y2 = convert_serval_length_to_float(&y2_, context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  auto path = path_factory->CreateLine(x1, y1, x2, y2);
  if (path) {
    path->Transform(transform_);
  }
  return path;
};

}  // namespace element
}  // namespace svg
}  // namespace serval
