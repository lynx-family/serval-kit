// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_
#define SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_

#include <cstddef>

#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class PatternTileBudget {
 public:
  PatternTileBudget(const SrSVGRenderContext& context, float x, float y,
                    float right, float bottom, float width, float height);
  bool HasRemaining() const;
  void Consume();

 private:
  void* traversal_;
  size_t remaining_{0};
};

bool IsIdentityTransform(const float* xform);
bool ComposeAffineTransform(const float* left, const float* right, float* out);
bool InvertAffineTransform(const float* xform, float* inverse);
void MapPoint(const float* xform, float x, float y, float* out_x, float* out_y);
bool TryMapBounds(const SrSVGBox& box, const float* xform, SrSVGBox* out);
SrSVGBox MapBounds(const SrSVGBox& box, const float* xform);

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_
