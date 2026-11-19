// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGEllipse.h"

#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGEllipse::ResolveGeometry(const SrSVGRenderContext& context, float& x,
                                   float& y, float& rx, float& ry) const {
  if (!resolve_serval_length(&cx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &x) ||
      !resolve_serval_length(&cy_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &y) ||
      !resolve_serval_length(&rx_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &rx) ||
      !resolve_serval_length(&ry_, &context, SR_SVG_LENGTH_TYPE_VERTICAL, &ry))
    return false;
  const SrSVGBox box{x - rx, y - ry, 2 * rx, 2 * ry};
  return serval_svg_valid_box(&box);
}

void SrSVGEllipse::onDraw(canvas::SrCanvas* canvas,
                          SrSVGRenderContext& context) const {
  float x, y, rx, ry;
  if (!ResolveGeometry(context, x, y, rx, ry))
    return;
  canvas->DrawEllipse(id_.c_str(), x, y, rx, ry, render_state_);
}

std::unique_ptr<canvas::Path> SrSVGEllipse::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  float x, y, rx, ry;
  if (!path_factory || !context || !ResolveGeometry(*context, x, y, rx, ry))
    return nullptr;
  auto path = path_factory->CreateEllipse(x, y, rx, ry);
  if (include_transform && path) {
    float xform[6];
    ResolvedTransform(xform, *context, path_factory);
    path->Transform(xform);
  }
  return path;
}

bool SrSVGEllipse::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "cx") == 0) {
    return ParseLengthAttribute(value, cx_);
  } else if (strcmp(name, "cy") == 0) {
    return ParseLengthAttribute(value, cy_);
  } else if (strcmp(name, "rx") == 0) {
    return ParseLengthAttribute(value, rx_, LengthRange::kNonNegative);
  } else if (strcmp(name, "ry") == 0) {
    return ParseLengthAttribute(value, ry_, LengthRange::kNonNegative);
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
