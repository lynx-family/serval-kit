// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_UTILS_SRFLOATCOMPARISON_H_
#define SVG_INCLUDE_UTILS_SRFLOATCOMPARISON_H_

#include <math.h>

namespace serval {
namespace svg {

#define SERVAL_SVG_FLOAT_EPSILON 0.01f

inline bool FloatsEqual(const float first, const float second) {
  return fabs(first - second) < SERVAL_SVG_FLOAT_EPSILON;
}

inline bool FloatsNotEqual(const float first, const float second) {
  return fabs(first - second) >= SERVAL_SVG_FLOAT_EPSILON;
}

inline bool FloatsLarger(const float first, const float second) {
  return fabs(first - second) >= SERVAL_SVG_FLOAT_EPSILON && first > second;
}

inline bool FloatsLargerOrEqual(const float first, const float second) {
  return first > second || fabs(first - second) < SERVAL_SVG_FLOAT_EPSILON;
}

inline bool FloatLess(const float first, const float second) {
  return fabs(first - second) >= SERVAL_SVG_FLOAT_EPSILON && first < second;
}

inline bool FloatLessOrEqual(const float first, const float second) {
  return first < second || fabs(first - second) < SERVAL_SVG_FLOAT_EPSILON;
}

inline bool IsZero(const float f) {
  return FloatsEqual(f, 0.0f);
}

#undef SERVAL_SVG_FLOAT_EPSILON

}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_UTILS_SRFLOATCOMPARISON_H_
