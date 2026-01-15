// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGRADIALGRADIENT_H_
#define SVG_INCLUDE_ELEMENT_SRSVGRADIALGRADIENT_H_

#include <vector>

#include "element/SrSVGContainer.h"
#include "element/SrSVGNode.h"
#include "element/SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGRadialGradient : public SrSVGContainer {
 public:
  static SrSVGRadialGradient* Make() { return new SrSVGRadialGradient(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  SrSVGObjectBoundingBoxUnitType gradient_units() const {
    return gradient_units_;
  }

 private:
  SrSVGRadialGradient() : SrSVGContainer(SrSVGTag::kRadialGradient) {}
  float gradient_transform_[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  SrSVGLength r_{0.5, SR_SVG_UNITS_NUMBER};
  SrSVGLength cx_{0.5, SR_SVG_UNITS_NUMBER};
  SrSVGLength cy_{0.5, SR_SVG_UNITS_NUMBER};
  SrSVGLength fx_{0., SR_SVG_UNITS_UNKNOWN};
  SrSVGLength fy_{0., SR_SVG_UNITS_UNKNOWN};
  GradientSpread spread_method_{pad};
  mutable std::vector<SrStop> stops_;
  SrSVGObjectBoundingBoxUnitType gradient_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGRADIALGRADIENT_H_
