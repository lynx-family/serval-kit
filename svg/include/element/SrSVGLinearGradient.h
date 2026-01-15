// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGLINEARGRADIENT_H_
#define SVG_INCLUDE_ELEMENT_SRSVGLINEARGRADIENT_H_

#include <vector>

#include "element/SrSVGContainer.h"
#include "element/SrSVGNode.h"
#include "element/SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGLinearGradient : public SrSVGContainer {
 public:
  static SrSVGLinearGradient* Make() { return new SrSVGLinearGradient(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  SrSVGObjectBoundingBoxUnitType gradient_units() const {
    return gradient_units_;
  }

 private:
  SrSVGLinearGradient() : SrSVGContainer(SrSVGTag::kLinearGradient) {}
  float gradient_transform_[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  SrSVGObjectBoundingBoxUnitType gradient_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  GradientSpread spread_method_{pad};
  SrSVGLength x1_{0};
  SrSVGLength x2_{1.0};
  SrSVGLength y1_{0};
  SrSVGLength y2_{0};
  mutable std::vector<SrStop> stops_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGLINEARGRADIENT_H_
