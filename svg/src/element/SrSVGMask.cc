// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMask.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGMask::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "mask-type") == 0) {
    mask_is_luminance_ = strcmp(value, "alpha") != 0;
    return true;
  } else if (strcmp(name, "maskUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "maskContentUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "x") == 0) {
    x_ = Atof(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = Atof(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = Atof(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = Atof(value);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}
}  // namespace element
}  // namespace svg
}  // namespace serval
