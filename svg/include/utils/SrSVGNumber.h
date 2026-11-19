// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_UTILS_SRSVGNUMBER_H_
#define SVG_INCLUDE_UTILS_SRSVGNUMBER_H_

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parser/SrSVGParse.h"

// Compatibility adapters share the bounded parser's grammar and range checks.
static inline bool SrSVGParseNumber(const char** cursor, double* out) {
  SrSVGParseCursor input = {*cursor, *cursor, NULL};
  if (!SrSVGParseScalar(&input, out, NULL))
    return false;
  *cursor = input.pos;
  return true;
}
static inline bool SrSVGParseFloat(const char** cursor, float* out) {
  SrSVGParseCursor input = {*cursor, *cursor, NULL};
  if (!SrSVGParseCoordinate(&input, out, NULL))
    return false;
  *cursor = input.pos;
  return true;
}

static inline void SrSVGSkipSpaces(const char** p) {
  while (isspace((unsigned char)**p))
    ++*p;
}

#endif  // SVG_INCLUDE_UTILS_SRSVGNUMBER_H_
