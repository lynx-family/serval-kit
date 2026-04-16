#ifndef SVG_INCLUDE_ELEMENT_SRSVGPATTERNRESOLVER_H_
#define SVG_INCLUDE_ELEMENT_SRSVGPATTERNRESOLVER_H_

#include <string>
#include <unordered_set>

#include "element/SrSVGPattern.h"

namespace serval {
namespace svg {
namespace element {

struct ResolvedPattern {
  const SrSVGPattern* content_pattern{nullptr};
  std::string id;
  float x{0.f};
  float y{0.f};
  float width{0.f};
  float height{0.f};
  SrSVGObjectBoundingBoxUnitType pattern_units{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType pattern_content_units{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  float pattern_transform[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  SrSVGBox view_port{0.f, 0.f, 0.f, 0.f};
  SrSVGBox view_box{0.f, 0.f, 0.f, 0.f};
  bool has_view_box{false};
  SrSVGPreserveAspectRatio preserve_aspect_ratio{
      make_default_preserve_aspect_radio()};
  float content_transform[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
};

bool IsPatternIri(const char* iri, const SrSVGRenderContext& context);
bool ResolvePatternFromIri(const char* iri, const SrSVGRenderContext& context,
                           const SrSVGBox& object_bounding_box,
                           const std::unordered_set<std::string>& active_ids,
                           ResolvedPattern* resolved_pattern);

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGPATTERNRESOLVER_H_
