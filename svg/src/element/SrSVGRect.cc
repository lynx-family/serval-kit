// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGRect.h"
#include "utils/SrFloatComparison.h"

#include <cstring>

namespace serval {
namespace svg {
namespace element {

bool SrSVGRect::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "rx") == 0) {
    rx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "ry") == 0) {
    ry_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
    return true;
  }
  // todo: call super after super being implemented.
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

void SrSVGRect::onDraw(canvas::SrCanvas* const canvas,
                       SrSVGRenderContext& context) const {
  // convert to platform pixel
  float xf = convert_serval_length_to_float(&x_, &context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float yf = convert_serval_length_to_float(&y_, &context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  float rx = convert_serval_length_to_float(&rx_, &context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float ry = convert_serval_length_to_float(&ry_, &context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  float wf = convert_serval_length_to_float(&width_, &context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float hf = convert_serval_length_to_float(&height_, &context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);

  if (FloatLess(rx, 0.f) && FloatLess(ry, 0.f)) {
    rx = 0.f;
    ry = 0.f;
  } else if (FloatLessOrEqual(rx, 0.f) && FloatsLarger(ry, 0.f)) {
    rx = ry;
  } else if (FloatLessOrEqual(ry, 0.f) && FloatsLarger(rx, 0.f)) {
    ry = rx;
  } else if (FloatsLarger(rx, wf / 2.0f)) {
    rx = wf / 2.0f;
  } else if (FloatsLarger(ry, hf / 2.0f)) {
    ry = wf / 2.0f;
  }

  // TODO: separate the draw function to fill() and stroke; And separate the
  // commands in base class since it's a common logic for all shapes.
  uint8_t type = 0;
  if (fill_ && fill_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagFill;
  }
  if (stroke_ && stroke_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagStroke;
  }
  canvas->DrawRect(id_.c_str(), xf, yf, rx, ry, wf, hf, render_state_);
}

std::unique_ptr<canvas::Path> SrSVGRect::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  float xf = convert_serval_length_to_float(&x_, context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float yf =
      convert_serval_length_to_float(&y_, context, SR_SVG_LENGTH_TYPE_VERTICAL);
  float rx = convert_serval_length_to_float(&rx_, context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float ry = convert_serval_length_to_float(&ry_, context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  float wf = convert_serval_length_to_float(&width_, context,
                                            SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float hf = convert_serval_length_to_float(&height_, context,
                                            SR_SVG_LENGTH_TYPE_VERTICAL);
  auto path = path_factory->CreateRect(xf, yf, rx, ry, wf, hf);
  if (path) {
    path->Transform(transform_);
  }
  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
