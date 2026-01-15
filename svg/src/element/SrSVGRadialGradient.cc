// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGRadialGradient.h"

#include "element/SrSVGStop.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGRadialGradient::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "gradientTransform") == 0) {
    ParseTransform(value, gradient_transform_);
    return true;
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
    cx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "cy") == 0) {
    cy_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "r") == 0) {
    r_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "fx") == 0) {
    fx_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "fy") == 0) {
    fy_ = make_serval_length(value);
    return true;
  }
  return SrSVGNode::ParseAndSetAttribute(name, value);
}

void SrSVGRadialGradient::OnRender(canvas::SrCanvas* canvas,
                                   SrSVGRenderContext& context) {
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
          stops_.push_back(stop);
        }
      }
    }
  }
  // If not set fx or fy, use cx or cy as default.
  if (fx_.unit == SR_SVG_UNITS_UNKNOWN) {
    fx_ = cx_;
  }
  if (fy_.unit == SR_SVG_UNITS_UNKNOWN) {
    fy_ = cy_;
  }
  float cx = convert_serval_length_to_float(&cx_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float cy = convert_serval_length_to_float(&cy_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float r =
      convert_serval_length_to_float(&r_, &context, SR_SVG_LENGTH_TYPE_NUMERIC);
  float fx = convert_serval_length_to_float(&fx_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  float fy = convert_serval_length_to_float(&fy_, &context,
                                            SR_SVG_LENGTH_TYPE_NUMERIC);
  canvas->UpdateRadialGradient(id_.c_str(), gradient_transform_, spread_method_,
                               cx, cy, r, fx, fy, stops_, gradient_units_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
