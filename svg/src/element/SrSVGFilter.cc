// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGFilter.h"
#include <cstring>
#include "element/SrSVGFilterPrimitives.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGFilter::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
  } else if (strcmp(name, "filterUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else if (strcmp(name, "primitiveUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else {
    return SrSVGContainer::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFilter::ShouldRenderSourceGraphicOnTop() const {
  // Only render source graphic on top for drop-shadow patterns:
  // requires BOTH blur/offset AND a color transform. A standalone
  // feGaussianBlur (plain blur) should NOT re-draw the original on top.
  bool has_blur = false;
  bool has_offset = false;
  bool has_color = false;
  const auto& filter_children = children();
  for (auto* child : filter_children) {
    if (!child) {
      continue;
    }
    if (child->Tag() == SrSVGTag::kFeGaussianBlur) {
      has_blur = true;
    } else if (child->Tag() == SrSVGTag::kFeOffset) {
      has_offset = true;
    } else if (child->Tag() == SrSVGTag::kFeColorMatrix) {
      has_color = true;
    }
  }
  // Drop-shadow pattern: offset present AND (blur or color transform).
  // Plain blur without offset is NOT a shadow — don't overlay original.
  return has_offset && (has_blur || has_color);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
