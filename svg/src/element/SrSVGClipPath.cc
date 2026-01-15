// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGClipPath.h"

namespace serval {
namespace svg {
namespace element {

void SrSVGClipPath::OnRender(canvas::SrCanvas*, SrSVGRenderContext&) {
  // invisible container, do nothing here.
}

bool SrSVGClipPath::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "clipPathUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      clip_path_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      clip_path_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "clip-rule") == 0) {
    if (strcmp(value, "evenodd") == 0) {
      clip_rule_ = SR_SVG_EO_FILL;
    } else {
      clip_rule_ = SR_SVG_FILL;
    }
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
