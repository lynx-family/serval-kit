// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGPath.h"

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGPath::onDraw(canvas::SrCanvas* canvas,
                       SrSVGRenderContext& context) const {
  uint8_t render_flag = 0;
  if (fill_ && fill_->type != SERVAL_PAINT_NONE) {
    render_flag |= kRenderTypeFlagFill;
    if (fill_rule_ && fill_rule_ == SR_SVG_EO_FILL) {
      render_flag |= kRenderTypeFillRule;
    }
  }
  if (stroke_ && stroke_->type != SERVAL_PAINT_NONE) {
    render_flag |= kRenderTypeFlagStroke;
  }
  if (path_) {
    canvas->DrawPath(id_.c_str(), path_->ops, path_->n_ops, path_->args,
                     path_->n_args, render_state_);
  }
}

bool SrSVGPath::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "d") == 0) {
    path_ = make_serval_path(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGPath::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  return path_factory->CreatePath(path_->ops, path_->n_ops, path_->args,
                                  path_->n_args);
}

SrSVGPath::~SrSVGPath() {
  release_serval_path(path_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
