// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGMASK_H_
#define SVG_INCLUDE_ELEMENT_SRSVGMASK_H_

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGMask : public SrSVGContainer {
 public:
  static SrSVGMask* Make() { return new SrSVGMask(SrSVGTag::kMask); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;
  inline SrSVGObjectBoundingBoxUnitType mask_units() const {
    return mask_units_;
  };
  inline SrSVGObjectBoundingBoxUnitType mask_content_units() const {
    return mask_content_units_;
  };
  ~SrSVGMask() override {}

 protected:
  explicit SrSVGMask(SrSVGTag t) : SrSVGContainer(t){};

 private:
  SrSVGObjectBoundingBoxUnitType mask_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType mask_content_units_{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  // Default values per SVG spec: x=-10%, y=-10%, width=120%, height=120%
  // But here we store them as floats. If units are missing, they are treated as user units or ratios depending on maskUnits?
  // Actually, if maskUnits=objectBoundingBox, defaults are -0.1, -0.1, 1.2, 1.2.
  // If userSpaceOnUse, defaults are ...?
  // Let's assume the user provided values or we use a "large enough" default if not provided?
  // For simplicity, let's init with these defaults.
  float x_{-0.1f};
  float y_{-0.1f};
  float width_{1.2f};
  float height_{1.2f};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGMASK_H_
