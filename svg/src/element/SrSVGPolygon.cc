// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGPolygon.h"

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGPolygon::onDraw(canvas::SrCanvas* canvas,
                          SrSVGRenderContext& context) const {
  uint8_t type = 0;
  if (stroke_ && stroke_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagStroke;
  }
  if (fill_ && fill_->type != SERVAL_PAINT_NONE) {
    type |= kRenderTypeFlagFill;
    if (fill_rule_ == SR_SVG_EO_FILL) {
      type |= kRenderTypeFillRule;
    }
  }

  if (polygon_ && polygon_->n_points != 0) {
    canvas->DrawPolygon(id_.c_str(), polygon_->points, polygon_->n_points,
                        render_state_);
  }
}
bool SrSVGPolygon::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "points") == 0) {
    polygon_ = make_serval_polygon(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGPolygon::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  if (polygon_ && polygon_->n_points != 0) {
    auto path =
        path_factory->CreatePolygon(polygon_->points, polygon_->n_points);
    if (path) {
      path->Transform(transform_);
    }
    return path;
  }
  return nullptr;
};

SrSVGPolygon::~SrSVGPolygon() {
  if (polygon_) {
    release_serval_polygon_path(polygon_);
  }
}

}  // namespace element
}  // namespace svg
}  // namespace serval
