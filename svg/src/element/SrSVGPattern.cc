// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGPattern.h"

#include <cstring>

namespace serval {
namespace svg {
namespace element {

bool SrSVGPattern::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    return ParseLengthAttribute(value, x_);
  } else if (strcmp(name, "y") == 0) {
    return ParseLengthAttribute(value, y_);
  } else if (strcmp(name, "width") == 0) {
    return ParseLengthAttribute(value, width_, LengthRange::kNonNegative);
  } else if (strcmp(name, "height") == 0) {
    return ParseLengthAttribute(value, height_, LengthRange::kNonNegative);
  } else if (strcmp(name, "patternUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      pattern_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else {
      pattern_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
    has_pattern_units_ = true;
    return true;
  } else if (strcmp(name, "patternContentUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      pattern_content_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      pattern_content_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    has_pattern_content_units_ = true;
    return true;
  } else if (strcmp(name, "patternTransform") == 0) {
    if (!TryParseTransform(value, pattern_transform_))
      return false;
    has_pattern_transform_ = true;
    return true;
  } else if (strcmp(name, "viewBox") == 0) {
    if (!parse_serval_view_box(value, &view_box_))
      return false;
    has_view_box_ = true;
    return true;
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    if (!parse_serval_preserve_aspect_ratio(value, &preserve_aspect_ratio_))
      return false;
    has_preserve_aspect_ratio_ = true;
    return true;
  } else if (strcmp(name, "xlink:href") == 0 || strcmp(name, "href") == 0) {
    if (value[0] == '#') {
      href_ = std::string(value + 1);
    } else {
      href_.clear();
    }
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

void SrSVGPattern::OnRender(canvas::SrCanvas* canvas,
                            SrSVGRenderContext& context) {
  // Pattern is a non-rendering paint server. It stays in IDMapper and should
  // be resolved lazily when a shape references url(#pattern-id).
  (void)canvas;
  (void)context;
}

void SrSVGPattern::RenderContent(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  const_cast<SrSVGPattern*>(this)->SrSVGContainer::OnRender(canvas, context);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
