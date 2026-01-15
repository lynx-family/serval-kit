// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGStop.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGStop::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "offset") == 0) {
    stop_.offset = make_serval_length(value);
    return true;
  } else if (strcmp(name, "stop-color") == 0) {
    stop_.stopColor = make_serval_color(value);
    return true;
  } else if (strcmp(name, "stop-opacity") == 0) {
    stop_.stopOpacity = make_serval_length(value);
    return true;
  }
  return false;
}

float SrSVGStop::offset(SrSVGRenderContext& context) const {
  if (stop_.offset.unit == SR_SVG_UNITS_PERCENTAGE) {
    return convert_serval_length_to_float(&(stop_.offset), &context,
                                          SR_SVG_LENGTH_TYPE_NUMERIC);
  } else if (stop_.offset.unit == SR_SVG_UNITS_NUMBER) {
    return stop_.offset.value;
  }
  return 0.f;
}

float SrSVGStop::opacity(SrSVGRenderContext& context) const {
  if (stop_.stopOpacity.unit == SR_SVG_UNITS_PERCENTAGE) {
    return convert_serval_length_to_float(&(stop_.stopOpacity), &context,
                                          SR_SVG_LENGTH_TYPE_NUMERIC);
  } else if (stop_.stopOpacity.unit == SR_SVG_UNITS_NUMBER) {
    return stop_.stopOpacity.value;
  }
  return 1.f;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
