// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGFilterPrimitives.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include "element/SrSVGTypes.h"
#include "parser/SrSVGParse.h"

namespace serval {
namespace svg {
namespace element {

namespace {
bool ParseUnitless(const char* value, float* out) {
  SrSVGLength length;
  if (!parse_serval_length(value, &length) ||
      length.unit != SR_SVG_UNITS_NUMBER)
    return false;
  *out = length.value;
  return true;
}
}  // namespace

bool SrSVGFilterPrimitive::ParseAndSetAttribute(const char* name,
                                                const char* value) {
  if (strcmp(name, "result") == 0) {
    result_ = value;
  } else if (strcmp(name, "in") == 0) {
    in_ = value;
  } else if (strcmp(name, "x") == 0) {
    return ParseLengthAttribute(value, x_);
  } else if (strcmp(name, "y") == 0) {
    return ParseLengthAttribute(value, y_);
  } else if (strcmp(name, "width") == 0) {
    return ParseLengthAttribute(value, width_, LengthRange::kNonNegative);
  } else if (strcmp(name, "height") == 0) {
    return ParseLengthAttribute(value, height_, LengthRange::kNonNegative);
  } else {
    return SrSVGNode::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeGaussianBlur::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "stdDeviation") == 0) {
    auto cursor = SrSVGCursor(value, strlen(value));
    SrSVGCursorSpaces(&cursor);
    float x, y;
    if (!SrSVGParseCoordinate(&cursor, &x, nullptr) || x < 0.f)
      return false;
    const char* end = cursor.pos;
    SrSVGCursorSpaces(&cursor);
    const bool comma = SrSVGCursorConsume(&cursor, ',');
    SrSVGCursorSpaces(&cursor);
    y = x;
    if (comma || !SrSVGCursorEnd(&cursor)) {
      if (cursor.pos == end || !SrSVGParseCoordinate(&cursor, &y, nullptr) ||
          y < 0.f)
        return false;
      SrSVGCursorSpaces(&cursor);
      if (!SrSVGCursorEnd(&cursor))
        return false;
    }
    std_deviation_x_ = x;
    std_deviation_y_ = y;
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeOffset::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "dx") == 0) {
    return ParseUnitless(value, &dx_);
  } else if (strcmp(name, "dy") == 0) {
    return ParseUnitless(value, &dy_);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeColorMatrix::ParseAndSetAttribute(const char* name,
                                              const char* value) {
  if (strcmp(name, "type") == 0) {
    type_ = value;
  } else if (strcmp(name, "values") == 0) {
    float values[20];
    size_t count = 0;
    auto cursor = SrSVGCursor(value, strlen(value));
    SrSVGCursorSpaces(&cursor);
    while (!SrSVGCursorEnd(&cursor)) {
      if (count == 20 ||
          !SrSVGParseCoordinate(&cursor, &values[count], nullptr))
        return false;
      ++count;
      const char* end = cursor.pos;
      SrSVGCursorSpaces(&cursor);
      if (SrSVGCursorEnd(&cursor))
        break;
      if (SrSVGCursorConsume(&cursor, ',')) {
        SrSVGCursorSpaces(&cursor);
        if (SrSVGCursorEnd(&cursor))
          return false;
      } else if (end == cursor.pos) {
        return false;
      }
    }
    if (count != 0 && count != 1 && count != 20)
      return false;
    values_.assign(values, values + count);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeComposite::ParseAndSetAttribute(const char* name,
                                            const char* value) {
  if (strcmp(name, "in2") == 0) {
    in2_ = value;
  } else if (strcmp(name, "operator") == 0) {
    operator_ = value;
  } else if (strcmp(name, "k1") == 0) {
    return ParseUnitless(value, &k1_);
  } else if (strcmp(name, "k2") == 0) {
    return ParseUnitless(value, &k2_);
  } else if (strcmp(name, "k3") == 0) {
    return ParseUnitless(value, &k3_);
  } else if (strcmp(name, "k4") == 0) {
    return ParseUnitless(value, &k4_);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeBlend::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "in2") == 0) {
    in2_ = value;
  } else if (strcmp(name, "mode") == 0) {
    mode_ = value;
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeFlood::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "flood-color") == 0) {
    auto* paint = make_serval_paint(value);
    if (!paint)
      return false;
    release_serval_paint(flood_color_);
    flood_color_ = paint;
  } else if (strcmp(name, "flood-opacity") == 0) {
    SrSVGLength opacity;
    if (!parse_serval_length(value, &opacity))
      return false;
    if (opacity.unit == SR_SVG_UNITS_PERCENTAGE)
      opacity.value /= 100.f;
    else if (opacity.unit != SR_SVG_UNITS_NUMBER)
      return false;
    flood_opacity_ = std::clamp(opacity.value, 0.f, 1.f);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

SrSVGFeFlood::~SrSVGFeFlood() {
  if (flood_color_)
    release_serval_paint(flood_color_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
