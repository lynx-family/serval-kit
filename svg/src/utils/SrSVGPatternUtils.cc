#include "utils/SrSVGPatternUtils.h"

#include <algorithm>
#include <cmath>

namespace serval {
namespace svg {
namespace element {

bool IsIdentityTransform(const float* xform) {
  return std::fabs(xform[0] - 1.f) < 1e-6f && std::fabs(xform[1]) < 1e-6f &&
         std::fabs(xform[2]) < 1e-6f && std::fabs(xform[3] - 1.f) < 1e-6f &&
         std::fabs(xform[4]) < 1e-6f && std::fabs(xform[5]) < 1e-6f;
}

bool InvertAffineTransform(const float* xform, float* inverse) {
  float det = xform[0] * xform[3] - xform[1] * xform[2];
  if (std::fabs(det) < 1e-6f) {
    return false;
  }
  float inv_det = 1.f / det;
  inverse[0] = xform[3] * inv_det;
  inverse[1] = -xform[1] * inv_det;
  inverse[2] = -xform[2] * inv_det;
  inverse[3] = xform[0] * inv_det;
  inverse[4] = (xform[2] * xform[5] - xform[3] * xform[4]) * inv_det;
  inverse[5] = (xform[1] * xform[4] - xform[0] * xform[5]) * inv_det;
  return true;
}

void MapPoint(const float* xform, float x, float y, float* out_x,
              float* out_y) {
  *out_x = xform[0] * x + xform[2] * y + xform[4];
  *out_y = xform[1] * x + xform[3] * y + xform[5];
}

SrSVGBox MapBounds(const SrSVGBox& box, const float* xform) {
  float points[8];
  MapPoint(xform, box.left, box.top, &points[0], &points[1]);
  MapPoint(xform, box.left + box.width, box.top, &points[2], &points[3]);
  MapPoint(xform, box.left + box.width, box.top + box.height, &points[4],
           &points[5]);
  MapPoint(xform, box.left, box.top + box.height, &points[6], &points[7]);

  float min_x = points[0];
  float max_x = points[0];
  float min_y = points[1];
  float max_y = points[1];
  for (int i = 2; i < 8; i += 2) {
    min_x = std::min(min_x, points[i]);
    max_x = std::max(max_x, points[i]);
    min_y = std::min(min_y, points[i + 1]);
    max_y = std::max(max_y, points[i + 1]);
  }
  return {min_x, min_y, max_x - min_x, max_y - min_y};
}

}  // namespace element
}  // namespace svg
}  // namespace serval
