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
    return ParseLengthAttribute(value, cx_);
  } else if (strcmp(name, "cy") == 0) {
    return ParseLengthAttribute(value, cy_);
  } else if (strcmp(name, "r") == 0) {
    return ParseLengthAttribute(value, r_, LengthRange::kNonNegative);
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

bool SrSVGCircle::ResolveGeometry(const SrSVGRenderContext& context, float& x,
                                  float& y, float& radius) const {
  if (!resolve_serval_length(&cx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &x) ||
      !resolve_serval_length(&cy_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &y) ||
      !resolve_serval_length(&r_, &context, SR_SVG_LENGTH_TYPE_OTHER, &radius))
    return false;
  const SrSVGBox box{x - radius, y - radius, 2 * radius, 2 * radius};
  return serval_svg_valid_box(&box);
}

void SrSVGCircle::onDraw(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const {
  float x, y, radius;
  if (!ResolveGeometry(context, x, y, radius))
    return;
  canvas->DrawCircle(id_.c_str(), x, y, radius, render_state_);
}

std::unique_ptr<canvas::Path> SrSVGCircle::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  float x, y, radius;
  if (!path_factory || !context || !ResolveGeometry(*context, x, y, radius))
    return nullptr;
  auto path = path_factory->CreateCircle(x, y, radius);
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
