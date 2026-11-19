// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGLinearGradient.h"

#include <vector>

#include "element/SrSVGStop.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGLinearGradient::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "gradientTransform") == 0) {
    return TryParseTransform(value, gradient_transform_);
  } else if (strcmp(name, "spreadMethod") == 0) {
    spread_method_ = make_serval_spread_method(value);
    return true;
  } else if (strcmp(name, "x1") == 0) {
    return ParseLengthAttribute(value, x1_);
  } else if (strcmp(name, "x2") == 0) {
    return ParseLengthAttribute(value, x2_);
  } else if (strcmp(name, "y1") == 0) {
    return ParseLengthAttribute(value, y1_);
  } else if (strcmp(name, "y2") == 0) {
    return ParseLengthAttribute(value, y2_);
  } else if (strcmp(name, "gradientUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      gradient_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else {
      gradient_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
    return true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGLinearGradient::OnRender(canvas::SrCanvas* canvas,
                                   SrSVGRenderContext& context) {
  std::vector<SrStop> stops;
  stops.reserve(children_.size());
  for (SrSVGNodeBase* child : children_) {
    if (child && child->Tag() == SrSVGTag::kStop) {
      SrSVGStop* stopNode = static_cast<SrSVGStop*>(child);
      if (stopNode) {
        SrStop stop = stopNode->stop_;
        stop.offset = {.value = stopNode->offset(context),
                       .unit = SR_SVG_UNITS_NUMBER};
        stop.stopOpacity = {.value = stopNode->opacity(context),
                            .unit = SR_SVG_UNITS_NUMBER};
        stops.push_back(stop);
      }
    }
  }
  float x1, y1, x2, y2;
  if (!resolve_serval_length(&x1_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &x1) ||
      !resolve_serval_length(&y1_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &y1) ||
      !resolve_serval_length(&x2_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &x2) ||
      !resolve_serval_length(&y2_, &context, SR_SVG_LENGTH_TYPE_NUMERIC, &y2))
    return;
  canvas->UpdateLinearGradient(id_.c_str(), gradient_transform_, spread_method_,
                               x1, x2, y1, y2, stops, gradient_units_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
