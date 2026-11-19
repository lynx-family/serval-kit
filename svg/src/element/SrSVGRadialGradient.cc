// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGRadialGradient.h"

#include <vector>

#include "element/SrSVGStop.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGRadialGradient::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "gradientTransform") == 0) {
    return TryParseTransform(value, gradient_transform_);
  } else if (strcmp(name, "gradientUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      gradient_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else {
      gradient_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
    return true;
  } else if (strcmp(name, "spreadMethod") == 0) {
    spread_method_ = make_serval_spread_method(value);
    return true;
  } else if (strcmp(name, "cx") == 0) {
    return ParseLengthAttribute(value, cx_);
  } else if (strcmp(name, "cy") == 0) {
    return ParseLengthAttribute(value, cy_);
  } else if (strcmp(name, "r") == 0) {
    return ParseLengthAttribute(value, r_, LengthRange::kNonNegative);
  } else if (strcmp(name, "fx") == 0) {
    return ParseLengthAttribute(value, fx_);
  } else if (strcmp(name, "fy") == 0) {
    return ParseLengthAttribute(value, fy_);
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGRadialGradient::OnRender(canvas::SrCanvas* canvas,
                                   SrSVGRenderContext& context) {
  std::vector<SrStop> stops;
  stops.reserve(children_.size());
  for (SrSVGNodeBase* child : children_) {
    if (child) {
      if (child->Tag() == SrSVGTag::kStop) {
        const SrSVGStop* stop_node = static_cast<const SrSVGStop*>(child);
        if (stop_node) {
          SrStop stop = stop_node->stop_;
          stop.offset = {.value = stop_node->offset(context),
                         .unit = SR_SVG_UNITS_NUMBER};
          stop.stopOpacity = {.value = stop_node->opacity(context),
                              .unit = SR_SVG_UNITS_NUMBER};
          stops.push_back(stop);
        }
      }
    }
  }
  const SrSVGLength& fx_length = fx_.unit == SR_SVG_UNITS_UNKNOWN ? cx_ : fx_;
  const SrSVGLength& fy_length = fy_.unit == SR_SVG_UNITS_UNKNOWN ? cy_ : fy_;
  float cx, cy, r, fx, fy;
  if (!resolve_serval_length(&cx_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &cx) ||
      !resolve_serval_length(&cy_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &cy) ||
      !resolve_serval_length(&r_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &r) ||
      !resolve_serval_length(&fx_length, &context, SR_SVG_LENGTH_TYPE_NUMERIC,
                             &fx) ||
      !resolve_serval_length(&fy_length, &context, SR_SVG_LENGTH_TYPE_NUMERIC,
                             &fy))
    return;
  canvas->UpdateRadialGradient(id_.c_str(), gradient_transform_, spread_method_,
                               cx, cy, r, fx, fy, stops, gradient_units_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
