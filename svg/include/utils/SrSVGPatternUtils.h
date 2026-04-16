#ifndef SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_
#define SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_

#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool IsIdentityTransform(const float* xform);
bool InvertAffineTransform(const float* xform, float* inverse);
void MapPoint(const float* xform, float x, float y, float* out_x, float* out_y);
SrSVGBox MapBounds(const SrSVGBox& box, const float* xform);

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_UTILS_SRSVGPATTERNUTILS_H_
