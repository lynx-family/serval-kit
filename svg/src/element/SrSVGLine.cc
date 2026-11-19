// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGLine.h"

#include <cmath>

#include "canvas/SrCanvas.h"
#include "element/SrSVGShape.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGLine::ResolveGeometry(const SrSVGRenderContext& context, float& x1,
                                float& y1, float& x2, float& y2) const {
  return resolve_serval_length(&x1_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                               &x1) &&
         resolve_serval_length(&y1_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                               &y1) &&
         resolve_serval_length(&x2_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                               &x2) &&
         resolve_serval_length(&y2_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                               &y2) &&
         std::isfinite(x2 - x1) && std::isfinite(y2 - y1);
}

void SrSVGLine::onDraw(canvas::SrCanvas* canvas,
                       SrSVGRenderContext& context) const {
  float x1, y1, x2, y2;
  if (!HasEffectiveStroke() || !ResolveGeometry(context, x1, y1, x2, y2))
    return;
  canvas->DrawLine(id_.c_str(), x1, y1, x2, y2, render_state_);
}

bool SrSVGLine::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x1") == 0) {
    return ParseLengthAttribute(value, x1_);
  } else if (strcmp(name, "y1") == 0) {
    return ParseLengthAttribute(value, y1_);
  } else if (strcmp(name, "x2") == 0) {
    return ParseLengthAttribute(value, x2_);
  } else if (strcmp(name, "y2") == 0) {
    return ParseLengthAttribute(value, y2_);
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGLine::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  float x1, y1, x2, y2;
  if (!path_factory || !context || !ResolveGeometry(*context, x1, y1, x2, y2))
    return nullptr;
  auto path = path_factory->CreateLine(x1, y1, x2, y2);
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
