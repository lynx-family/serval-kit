// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGRect.h"

#include <algorithm>

#include <cstring>

namespace serval {
namespace svg {
namespace element {

bool SrSVGRect::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    return ParseLengthAttribute(value, x_);
  } else if (strcmp(name, "y") == 0) {
    return ParseLengthAttribute(value, y_);
  } else if (strcmp(name, "rx") == 0) {
    return ParseLengthAttribute(value, rx_, LengthRange::kNonNegative);
  } else if (strcmp(name, "ry") == 0) {
    return ParseLengthAttribute(value, ry_, LengthRange::kNonNegative);
  } else if (strcmp(name, "width") == 0) {
    return ParseLengthAttribute(value, width_, LengthRange::kNonNegative);
  } else if (strcmp(name, "height") == 0) {
    return ParseLengthAttribute(value, height_, LengthRange::kNonNegative);
  }
  // todo: call super after super being implemented.
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

void SrSVGRect::ClearAnimatedAttribute(const std::string& name) {
  if (name == "rx")
    rx_ = {};
  else if (name == "ry")
    ry_ = {};
  else
    SrSVGShape::ClearAnimatedAttribute(name);
}

bool SrSVGRect::ResolveGeometry(const SrSVGRenderContext& context,
                                SrSVGBox& box, float& rx, float& ry) const {
  if (!resolve_serval_length(&x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &box.left) ||
      !resolve_serval_length(&y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                             &box.top) ||
      !resolve_serval_length(&width_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &box.width) ||
      !resolve_serval_length(&height_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                             &box.height) ||
      !serval_svg_valid_box(&box))
    return false;
  const bool has_rx = rx_.unit != SR_SVG_UNITS_UNKNOWN;
  const bool has_ry = ry_.unit != SR_SVG_UNITS_UNKNOWN;
  if (!resolve_serval_length(&rx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &rx) ||
      !resolve_serval_length(&ry_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &ry))
    return false;
  if (!has_rx)
    rx = ry;
  if (!has_ry)
    ry = rx;
  if (rx == 0.f || ry == 0.f) {
    rx = ry = 0.f;
  }
  rx = std::min(rx, box.width / 2.f);
  ry = std::min(ry, box.height / 2.f);
  return true;
}

void SrSVGRect::onDraw(canvas::SrCanvas* canvas,
                       SrSVGRenderContext& context) const {
  SrSVGBox box;
  float rx, ry;
  if (!ResolveGeometry(context, box, rx, ry))
    return;
  canvas->DrawRect(id_.c_str(), box.left, box.top, rx, ry, box.width,
                   box.height, render_state_);
}

std::unique_ptr<canvas::Path> SrSVGRect::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  SrSVGBox box;
  float rx, ry;
  if (!path_factory || !context || !ResolveGeometry(*context, box, rx, ry))
    return nullptr;
  auto path = path_factory->CreateRect(box.left, box.top, rx, ry, box.width,
                                       box.height);
  if (include_transform && path) {
    float xform[6];
    ResolvedTransform(xform, *context, path_factory);
    path->Transform(xform);
  }
  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
