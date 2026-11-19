// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SVG_INCLUDE_UTILS_SRSVGARC_H_
#define SVG_INCLUDE_UTILS_SRSVGARC_H_
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SR_SVG_ARC_MAX_CUBICS 16
// Fixed storage; no allocation, recursion, or platform drawing calls.
// line uses the original end point; count==0 with !line omits a coincident arc.
typedef struct SrSVGArc {
  bool line;
  size_t count;
  double cubics[SR_SVG_ARC_MAX_CUBICS][6];
} SrSVGArc;
// Arguments use the existing SrPathData A-command layout (nine floats).
// Invalid input leaves out unchanged. All successful coordinates fit float.
bool SrSVGArcToCubics(const float args[9], SrSVGArc* out);
#ifdef __cplusplus
}
#endif
#endif  // SVG_INCLUDE_UTILS_SRSVGARC_H_
