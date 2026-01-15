// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGLinearGradient.h"

#include "element/SrSVGStop.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGLinearGradient::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "gradientTransform") == 0) {
    ParseTransform(value, gradient_transform_);
    return true;
  } else if (strcmp(name, "spreadMethod") == 0) {
    spread_method_ = make_serval_spread_method(value);
    return true;
  } else if (strcmp(name, "x1") == 0) {
    x1_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "x2") == 0) {
    x2_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y1") == 0) {
    y1_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y2") == 0) {
    y2_ = make_serval_length(value);
    return true;
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
  for (SrSVGNodeBase* child : children_) {
    if (child && child->Tag() == SrSVGTag::kStop) {
      SrSVGStop* stopNode = static_cast<SrSVGStop*>(child);
      if (stopNode) {
        SrStop stop = stopNode->stop_;
        stop.offset = {.value = stopNode->offset(context),
                       .unit = SR_SVG_UNITS_NUMBER};
        stop.stopOpacity = {.value = stopNode->opacity(context),
                            .unit = SR_SVG_UNITS_NUMBER};
        stops_.push_back(stop);
      }
    }
  }
  float x1 = convert_serval_length_to_float(&x1_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float y1 = convert_serval_length_to_float(&y1_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float x2 = convert_serval_length_to_float(&x2_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float y2 = convert_serval_length_to_float(&y2_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  canvas->UpdateLinearGradient(id_.c_str(), gradient_transform_, spread_method_,
                               x1, x2, y1, y2, stops_, gradient_units_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
