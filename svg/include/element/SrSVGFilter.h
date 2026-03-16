// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGFILTER_H_
#define SVG_INCLUDE_ELEMENT_SRSVGFILTER_H_

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGFilter : public SrSVGContainer {
 public:
  static SrSVGFilter* Make() { return new SrSVGFilter(SrSVGTag::kFilter); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  bool ShouldRenderSourceGraphicOnTop() const;

  SrSVGObjectBoundingBoxUnitType filter_units() const { return filter_units_; }
  SrSVGObjectBoundingBoxUnitType primitive_units() const {
    return primitive_units_;
  }
  
  const SrSVGLength& x() const { return x_; }
  const SrSVGLength& y() const { return y_; }
  const SrSVGLength& width() const { return width_; }
  const SrSVGLength& height() const { return height_; }

 protected:
  explicit SrSVGFilter(SrSVGTag t) : SrSVGContainer(t) {}

 private:
  SrSVGObjectBoundingBoxUnitType filter_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType primitive_units_{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
      
  SrSVGLength x_{.value = -10.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength y_{.value = -10.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength width_{.value = 120.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength height_{.value = 120.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGFILTER_H_
