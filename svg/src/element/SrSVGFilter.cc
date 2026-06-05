// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGFilter.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <utility>

#include "element/SrSVGFilterPrimitives.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

namespace {

float ResolveUnitLength(const SrSVGLength& length,
                        const SrSVGRenderContext& context,
                        SrSVGLengthType length_type,
                        SrSVGObjectBoundingBoxUnitType unit_type) {
  SrSVGRenderContext mutable_context = context;
  if (unit_type == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    mutable_context.view_port = SrSVGBox{0.f, 0.f, 1.f, 1.f};
    mutable_context.view_box = mutable_context.view_port;
  }
  return convert_serval_length_to_float(&length, &mutable_context, length_type);
}

SrSVGBox UnionBox(const SrSVGBox& a, const SrSVGBox& b) {
  const float left = std::min(a.left, b.left);
  const float top = std::min(a.top, b.top);
  const float right = std::max(a.left + a.width, b.left + b.width);
  const float bottom = std::max(a.top + a.height, b.top + b.height);
  return SrSVGBox{left, top, right - left, bottom - top};
}

bool IsStandardFilterInput(const std::string& input) {
  return input == "SourceGraphic" || input == "SourceAlpha" ||
         input == "BackgroundImage" || input == "BackgroundAlpha" ||
         input == "FillPaint" || input == "StrokePaint";
}

SrSVGBox ResolvePrimitiveRegion(
    const SrSVGFilterPrimitive& primitive, const SrSVGBox& object_bounds,
    const SrSVGBox& default_region, const SrSVGRenderContext& context,
    SrSVGObjectBoundingBoxUnitType primitive_units) {
  if (!primitive.x() && !primitive.y() && !primitive.width() &&
      !primitive.height()) {
    return default_region;
  }

  SrSVGBox region = default_region;
  if (primitive.x()) {
    float x = ResolveUnitLength(*primitive.x(), context,
                                SR_SVG_LENGTH_TYPE_HORIZONTAL, primitive_units);
    region.left = primitive_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                      ? object_bounds.left + x * object_bounds.width
                      : x;
  }
  if (primitive.y()) {
    float y = ResolveUnitLength(*primitive.y(), context,
                                SR_SVG_LENGTH_TYPE_VERTICAL, primitive_units);
    region.top = primitive_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                     ? object_bounds.top + y * object_bounds.height
                     : y;
  }
  if (primitive.width()) {
    float width =
        ResolveUnitLength(*primitive.width(), context,
                          SR_SVG_LENGTH_TYPE_HORIZONTAL, primitive_units);
    region.width = primitive_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                       ? width * object_bounds.width
                       : width;
  }
  if (primitive.height()) {
    float height =
        ResolveUnitLength(*primitive.height(), context,
                          SR_SVG_LENGTH_TYPE_VERTICAL, primitive_units);
    region.height = primitive_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                        ? height * object_bounds.height
                        : height;
  }
  return region;
}

std::string ResolveInputName(const SrSVGFilterPrimitive& primitive,
                             const std::string& previous_result) {
  return primitive.input().empty() ? previous_result : primitive.input();
}

std::string ResolveResultName(const SrSVGFilterPrimitive& primitive,
                              size_t index) {
  if (!primitive.result().empty()) {
    return primitive.result();
  }
  return "__serval_filter_result_" + std::to_string(index);
}

}  // namespace

bool SrSVGFilter::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
  } else if (strcmp(name, "filterUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else if (strcmp(name, "primitiveUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else {
    return SrSVGContainer::ParseAndSetAttribute(name, value);
  }
  return true;
}

SrSVGBox SrSVGFilter::ResolveFilterRegion(
    const SrSVGBox& object_bounds, const SrSVGRenderContext& context) const {
  float x = ResolveUnitLength(x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                              filter_units_);
  float y = ResolveUnitLength(y_, context, SR_SVG_LENGTH_TYPE_VERTICAL,
                              filter_units_);
  float width = ResolveUnitLength(width_, context,
                                  SR_SVG_LENGTH_TYPE_HORIZONTAL, filter_units_);
  float height = ResolveUnitLength(height_, context,
                                   SR_SVG_LENGTH_TYPE_VERTICAL, filter_units_);

  if (filter_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    x = object_bounds.left + x * object_bounds.width;
    y = object_bounds.top + y * object_bounds.height;
    width *= object_bounds.width;
    height *= object_bounds.height;
  }

  return SrSVGBox{x, y, width, height};
}

bool SrSVGFilter::BuildFilterModel(const SrSVGBox& object_bounds,
                                   bool has_object_bounds,
                                   const SrSVGRenderContext& context,
                                   canvas::SrFilterModel* model) const {
  if (!model) {
    return false;
  }
  if (!has_object_bounds &&
      (filter_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX ||
       primitive_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX)) {
    return false;
  }

  model->region = ResolveFilterRegion(object_bounds, context);
  model->primitives.clear();
  if (model->region.width <= 0.f || model->region.height <= 0.f) {
    return true;
  }

  std::unordered_map<std::string, SrSVGBox> result_regions;
  result_regions["SourceGraphic"] = model->region;
  result_regions["SourceAlpha"] = model->region;
  result_regions["BackgroundImage"] = model->region;
  result_regions["BackgroundAlpha"] = model->region;
  result_regions["FillPaint"] = model->region;
  result_regions["StrokePaint"] = model->region;

  std::string previous_result = "SourceGraphic";
  size_t primitive_index = 0;
  for (auto* child : children()) {
    if (!child) {
      continue;
    }
    switch (child->Tag()) {
      case SrSVGTag::kFeGaussianBlur:
      case SrSVGTag::kFeOffset:
      case SrSVGTag::kFeColorMatrix:
      case SrSVGTag::kFeComposite:
      case SrSVGTag::kFeBlend:
      case SrSVGTag::kFeFlood:
        break;
      default:
        continue;
    }
    auto* primitive = static_cast<SrSVGFilterPrimitive*>(child);

    canvas::SrFilterPrimitiveModel primitive_model;
    primitive_model.input = ResolveInputName(*primitive, previous_result);
    primitive_model.result = ResolveResultName(*primitive, primitive_index);

    bool uses_standard_input =
        IsStandardFilterInput(primitive_model.input) ||
        (primitive_model.input2.empty()
             ? false
             : IsStandardFilterInput(primitive_model.input2));
    bool has_referenced_region = false;
    SrSVGBox default_region = model->region;
    auto input_region = result_regions.find(primitive_model.input);
    if (input_region != result_regions.end()) {
      default_region = input_region->second;
      has_referenced_region = true;
    }

    switch (child->Tag()) {
      case SrSVGTag::kFeGaussianBlur: {
        auto* blur = static_cast<SrSVGFeGaussianBlur*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kGaussianBlur;
        primitive_model.std_deviation_x =
            primitive_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                ? blur->std_deviation_x() * object_bounds.width
                : blur->std_deviation_x();
        primitive_model.std_deviation_y =
            primitive_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                ? blur->std_deviation_y() * object_bounds.height
                : blur->std_deviation_y();
        break;
      }
      case SrSVGTag::kFeOffset: {
        auto* offset = static_cast<SrSVGFeOffset*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kOffset;
        primitive_model.dx =
            primitive_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                ? offset->dx() * object_bounds.width
                : offset->dx();
        primitive_model.dy =
            primitive_units_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX
                ? offset->dy() * object_bounds.height
                : offset->dy();
        break;
      }
      case SrSVGTag::kFeColorMatrix: {
        auto* color_matrix = static_cast<SrSVGFeColorMatrix*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kColorMatrix;
        primitive_model.color_matrix_type = color_matrix->type();
        primitive_model.color_matrix_values = color_matrix->values();
        break;
      }
      case SrSVGTag::kFeComposite: {
        auto* composite = static_cast<SrSVGFeComposite*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kComposite;
        primitive_model.input2 = composite->input2();
        primitive_model.composite_operator = composite->composite_operator();
        primitive_model.k1 = composite->k1();
        primitive_model.k2 = composite->k2();
        primitive_model.k3 = composite->k3();
        primitive_model.k4 = composite->k4();
        auto input2_region = result_regions.find(primitive_model.input2);
        if (input2_region != result_regions.end()) {
          default_region = has_referenced_region
                               ? UnionBox(default_region, input2_region->second)
                               : input2_region->second;
          has_referenced_region = true;
        }
        uses_standard_input = uses_standard_input ||
                              IsStandardFilterInput(primitive_model.input2);
        break;
      }
      case SrSVGTag::kFeBlend: {
        auto* blend = static_cast<SrSVGFeBlend*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kBlend;
        primitive_model.input2 = blend->input2();
        primitive_model.blend_mode = blend->mode();
        auto input2_region = result_regions.find(primitive_model.input2);
        if (input2_region != result_regions.end()) {
          default_region = has_referenced_region
                               ? UnionBox(default_region, input2_region->second)
                               : input2_region->second;
          has_referenced_region = true;
        }
        uses_standard_input = uses_standard_input ||
                              IsStandardFilterInput(primitive_model.input2);
        break;
      }
      case SrSVGTag::kFeFlood: {
        auto* flood = static_cast<SrSVGFeFlood*>(child);
        primitive_model.type = canvas::SrFilterPrimitiveType::kFlood;
        primitive_model.flood_opacity = flood->flood_opacity();
        if (flood->flood_color() &&
            flood->flood_color()->type == SERVAL_PAINT_COLOR &&
            flood->flood_color()->content.color.type == SERVAL_COLOR) {
          primitive_model.flood_color =
              flood->flood_color()->content.color.color;
        }
        has_referenced_region = false;
        uses_standard_input = false;
        default_region = model->region;
        break;
      }
      default:
        continue;
    }

    if (!has_referenced_region || uses_standard_input) {
      default_region = model->region;
    }
    primitive_model.subregion = ResolvePrimitiveRegion(
        *primitive, object_bounds, default_region, context, primitive_units_);
    result_regions[primitive_model.result] = primitive_model.subregion;
    previous_result = primitive_model.result;
    model->primitives.push_back(std::move(primitive_model));
    primitive_index++;
  }
  return true;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
