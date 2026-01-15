// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGSVG.h"

#include "element/SrSVGContainer.h"
#include "element/SrSVGTypes.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGLog.h"

namespace serval {
namespace svg {
namespace element {

SrSVGSVG::SrSVGSVG(SrSVGTag tag)
    : SrSVGContainer(tag),
      preserve_aspect_radio_(make_default_preserve_aspect_radio()) {}

bool SrSVGSVG::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "viewBox") == 0) {
    view_box_ = make_serval_view_box(value);
    LOGV("viewBox =[%f, %f, %f, %f]", view_box_.left, view_box_.top,
         view_box_.width, view_box_.height);
    return true;
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    preserve_aspect_radio_ = make_preserve_aspect_radio(value);
    LOGV("preserveAspectRatio =[%f, %f, %f]", preserve_aspect_radio_.scale,
         preserve_aspect_radio_.align_x, preserve_aspect_radio_.align_y);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

SrSVGSVG::~SrSVGSVG() = default;

bool SrSVGSVG::OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  if (IsZero(context.view_port.width) || IsZero(context.view_port.height) ||
      IsZero(context.view_box.width) || IsZero(context.view_box.height)) {
    return false;
  }
  canvas->SetViewBox(context.view_port.left, context.view_port.top,
                     context.view_port.width, context.view_port.height);
  float xform[6];
  calculate_view_box_transform(&(context.view_port), &(view_box_),
                               preserve_aspect_radio_, xform);
  canvas->Transform(xform);
  return true;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
