// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGPolyLine.h"

#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGPolyLine::onDraw(canvas::SrCanvas* canvas,
                           SrSVGRenderContext& context) const {
  if (polygon_ && polygon_->n_points != 0) {
    canvas->DrawPolyline(id_.c_str(), polygon_->points, polygon_->n_points,
                         render_state_);
  }
}

bool SrSVGPolyLine::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "points") == 0) {
    polygon_ = make_serval_polygon(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGPolyLine::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  if (polygon_ && polygon_->n_points != 0) {
    auto path =
        path_factory->CreatePolyline(polygon_->points, polygon_->n_points);
    if (path) {
      path->Transform(transform_);
    }
  }
  return nullptr;
};

SrSVGPolyLine::~SrSVGPolyLine() {
  if (polygon_) {
    release_serval_polygon_path(polygon_);
  }
}

}  // namespace element
}  // namespace svg
}  // namespace serval
