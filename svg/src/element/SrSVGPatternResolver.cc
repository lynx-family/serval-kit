#include "element/SrSVGPatternResolver.h"

#include <algorithm>
#include <optional>

#include "element/SrSVGContainer.h"
#include "element/SrSVGNode.h"
#include "element/SrSVGUse.h"

namespace serval {
namespace svg {
namespace element {

namespace {

struct PatternCascadeState {
  const SrSVGPattern* content_pattern{nullptr};
  std::optional<SrSVGLength> x;
  std::optional<SrSVGLength> y;
  std::optional<SrSVGLength> width;
  std::optional<SrSVGLength> height;
  SrSVGObjectBoundingBoxUnitType pattern_units{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType pattern_content_units{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  float pattern_transform[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  SrSVGBox view_box{0.f, 0.f, 0.f, 0.f};
  SrSVGPreserveAspectRatio preserve_aspect_ratio{
      make_default_preserve_aspect_radio()};
  bool has_pattern_units{false};
  bool has_pattern_content_units{false};
  bool has_pattern_transform{false};
  bool has_view_box{false};
  bool has_preserve_aspect_ratio{false};
};

bool PatternContainsUnsupportedNode(
    SrSVGNodeBase* node, IDMapper* mapper,
    std::unordered_set<std::string>* visited_use_ids) {
  if (!node) {
    return false;
  }

  if (node->Tag() == SrSVGTag::kImage) {
    return true;
  }

  if (node->Tag() == SrSVGTag::kUse) {
    auto* use = static_cast<SrSVGUse*>(node);
    if (use->href().empty() || !mapper || !visited_use_ids) {
      return false;
    }
    if (!visited_use_ids->insert(use->href()).second) {
      return false;
    }
    auto it = mapper->find(use->href());
    if (it == mapper->end() || !it->second) {
      return false;
    }
    return PatternContainsUnsupportedNode(it->second, mapper, visited_use_ids);
  }

  switch (node->Tag()) {
    case SrSVGTag::kG:
    case SrSVGTag::kPattern:
    case SrSVGTag::kSvg: {
      auto* container = static_cast<SrSVGContainer*>(node);
      for (auto* child : container->children()) {
        if (PatternContainsUnsupportedNode(child, mapper, visited_use_ids)) {
          return true;
        }
      }
      return false;
    }
    default:
      return false;
  }
}

bool ResolvePatternCascade(const SrSVGPattern* pattern, const std::string& id,
                           IDMapper* mapper,
                           const std::unordered_set<std::string>& active_ids,
                           std::unordered_set<std::string>* visited_ids,
                           PatternCascadeState* state) {
  if (!pattern || !visited_ids || !state) {
    return false;
  }
  if (active_ids.find(id) != active_ids.end() ||
      visited_ids->find(id) != visited_ids->end()) {
    return false;
  }
  visited_ids->insert(id);

  if (pattern->has_href()) {
    auto it = mapper->find(pattern->href());
    if (it != mapper->end() && it->second &&
        it->second->Tag() == SrSVGTag::kPattern) {
      if (!ResolvePatternCascade(static_cast<SrSVGPattern*>(it->second),
                                 pattern->href(), mapper, active_ids,
                                 visited_ids, state)) {
        return false;
      }
    }
  }

  if (!pattern->children().empty()) {
    state->content_pattern = pattern;
  }
  if (pattern->x()) {
    state->x = pattern->x();
  }
  if (pattern->y()) {
    state->y = pattern->y();
  }
  if (pattern->width()) {
    state->width = pattern->width();
  }
  if (pattern->height()) {
    state->height = pattern->height();
  }
  if (pattern->has_pattern_units()) {
    state->pattern_units = pattern->pattern_units();
    state->has_pattern_units = true;
  }
  if (pattern->has_pattern_content_units()) {
    state->pattern_content_units = pattern->pattern_content_units();
    state->has_pattern_content_units = true;
  }
  if (pattern->has_pattern_transform()) {
    const float* pattern_transform = pattern->pattern_transform();
    std::copy(pattern_transform, pattern_transform + 6,
              state->pattern_transform);
    state->has_pattern_transform = true;
  }
  if (pattern->has_view_box()) {
    state->view_box = pattern->view_box();
    state->has_view_box = true;
  }
  if (pattern->has_preserve_aspect_ratio()) {
    state->preserve_aspect_ratio = pattern->preserve_aspect_ratio();
    state->has_preserve_aspect_ratio = true;
  }
  return true;
}

}  // namespace

bool IsPatternIri(const char* iri, const SrSVGRenderContext& context) {
  if (!iri || iri[0] != '#') {
    return false;
  }
  auto* mapper = static_cast<IDMapper*>(context.id_mapper);
  if (!mapper) {
    return false;
  }
  auto it = mapper->find(std::string(iri + 1));
  return it != mapper->end() && it->second &&
         it->second->Tag() == SrSVGTag::kPattern;
}

bool ResolvePatternFromIri(const char* iri, const SrSVGRenderContext& context,
                           const SrSVGBox& object_bounding_box,
                           const std::unordered_set<std::string>& active_ids,
                           ResolvedPattern* resolved_pattern) {
  if (!resolved_pattern || !iri || iri[0] != '#') {
    return false;
  }
  auto* mapper = static_cast<IDMapper*>(context.id_mapper);
  if (!mapper) {
    return false;
  }
  std::string id(iri + 1);
  auto it = mapper->find(id);
  if (it == mapper->end() || !it->second ||
      it->second->Tag() != SrSVGTag::kPattern) {
    return false;
  }

  PatternCascadeState cascade;
  std::unordered_set<std::string> visited_ids;
  if (!ResolvePatternCascade(static_cast<SrSVGPattern*>(it->second), id, mapper,
                             active_ids, &visited_ids, &cascade)) {
    return false;
  }
  if (!cascade.content_pattern) {
    return false;
  }
  std::unordered_set<std::string> visited_use_ids;
  if (PatternContainsUnsupportedNode(
          const_cast<SrSVGPattern*>(cascade.content_pattern), mapper,
          &visited_use_ids)) {
    return false;
  }

  resolved_pattern->id = id;
  resolved_pattern->content_pattern = cascade.content_pattern;
  resolved_pattern->pattern_units = cascade.pattern_units;
  resolved_pattern->pattern_content_units = cascade.pattern_content_units;
  resolved_pattern->preserve_aspect_ratio = cascade.preserve_aspect_ratio;
  std::copy(std::begin(cascade.pattern_transform),
            std::end(cascade.pattern_transform),
            resolved_pattern->pattern_transform);

  SrSVGLength pattern_x = cascade.x.value_or(SrSVGLength{0});
  SrSVGLength pattern_y = cascade.y.value_or(SrSVGLength{0});
  SrSVGLength pattern_width = cascade.width.value_or(SrSVGLength{0});
  SrSVGLength pattern_height = cascade.height.value_or(SrSVGLength{0});

  SrSVGRenderContext mutable_context = context;
  if (cascade.pattern_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    mutable_context.view_port = SrSVGBox{0.f, 0.f, 1.f, 1.f};
    mutable_context.view_box = mutable_context.view_port;
  }
  resolved_pattern->x = convert_serval_length_to_float(
      &pattern_x, &mutable_context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  resolved_pattern->y = convert_serval_length_to_float(
      &pattern_y, &mutable_context, SR_SVG_LENGTH_TYPE_VERTICAL);
  resolved_pattern->width = convert_serval_length_to_float(
      &pattern_width, &mutable_context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  resolved_pattern->height = convert_serval_length_to_float(
      &pattern_height, &mutable_context, SR_SVG_LENGTH_TYPE_VERTICAL);

  if (resolved_pattern->pattern_units ==
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    resolved_pattern->x = object_bounding_box.left +
                          resolved_pattern->x * object_bounding_box.width;
    resolved_pattern->y = object_bounding_box.top +
                          resolved_pattern->y * object_bounding_box.height;
    resolved_pattern->width *= object_bounding_box.width;
    resolved_pattern->height *= object_bounding_box.height;
  }
  if (resolved_pattern->width <= 0.f || resolved_pattern->height <= 0.f) {
    return false;
  }

  resolved_pattern->view_port =
      SrSVGBox{0.f, 0.f, resolved_pattern->width, resolved_pattern->height};
  resolved_pattern->has_view_box = cascade.has_view_box;
  resolved_pattern->view_box =
      cascade.has_view_box ? cascade.view_box : resolved_pattern->view_port;

  xform_identity(resolved_pattern->content_transform);
  if (cascade.has_view_box && cascade.view_box.width > 0.f &&
      cascade.view_box.height > 0.f) {
    calculate_view_box_transform(
        &resolved_pattern->view_port, &resolved_pattern->view_box,
        cascade.preserve_aspect_ratio, resolved_pattern->content_transform);
  } else if (resolved_pattern->pattern_content_units ==
             SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    resolved_pattern->content_transform[0] = object_bounding_box.width;
    resolved_pattern->content_transform[3] = object_bounding_box.height;
    resolved_pattern->content_transform[4] =
        object_bounding_box.left - resolved_pattern->x;
    resolved_pattern->content_transform[5] =
        object_bounding_box.top - resolved_pattern->y;
  } else {
    xform_set_translation(resolved_pattern->content_transform,
                          -resolved_pattern->x, -resolved_pattern->y);
  }

  return true;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
