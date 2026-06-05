// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMask.h"

#include <cstring>

namespace serval {
namespace svg {
namespace element {

namespace {

float ResolveMaskLength(const SrSVGLength& length,
                        const SrSVGRenderContext& context,
                        SrSVGLengthType length_type,
                        SrSVGObjectBoundingBoxUnitType unit_type) {
  SrSVGRenderContext mutable_context = context;
  if (unit_type == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    mutable_context.view_port = SrSVGBox{0.f, 0.f, 1.f, 1.f};
    mutable_context.view_box = mutable_context.view_port;
  }
  return convert_serval_length_to_float(&length, &mutable_context, length_type);
}

}  // namespace

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
    x_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

SrSVGBox SrSVGMask::ResolveMaskRegion(const SrSVGBox& object_bounds,
                                      const SrSVGRenderContext& context) const {
  float x = ResolveMaskLength(x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                              mask_units_);
  float y =
      ResolveMaskLength(y_, context, SR_SVG_LENGTH_TYPE_VERTICAL, mask_units_);
  float width = ResolveMaskLength(width_, context,
                                  SR_SVG_LENGTH_TYPE_HORIZONTAL, mask_units_);
  float height = ResolveMaskLength(height_, context,
                                   SR_SVG_LENGTH_TYPE_VERTICAL, mask_units_);

  if (mask_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    x = object_bounds.left + x * object_bounds.width;
    y = object_bounds.top + y * object_bounds.height;
    width *= object_bounds.width;
    height *= object_bounds.height;
  }

  return SrSVGBox{x, y, width, height};
}

}  // namespace element
}  // namespace svg
}  // namespace serval
