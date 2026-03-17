// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGMASK_H_
#define SVG_INCLUDE_ELEMENT_SRSVGMASK_H_

#include <functional>
#include <memory>

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGMask : public SrSVGContainer {
 public:
  static SrSVGMask* Make() { return new SrSVGMask(SrSVGTag::kMask); }
  static bool ApplyToTargetIfPresent(canvas::SrCanvas* canvas,
                                     SrSVGRenderContext& context,
                                     const SrSVGNodeBase* target,
                                     const SrSVGPaint* mask_paint,
                                     const std::function<void()>& draw_content);
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
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
  void RenderMaskContent(canvas::SrCanvas* canvas, SrSVGRenderContext& context);

  canvas::MaskType mask_type_{canvas::MaskType::kLuminance};
  SrSVGObjectBoundingBoxUnitType mask_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType mask_content_units_{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  SrSVGLength x_{-10.f, SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength y_{-10.f, SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength width_{120.f, SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength height_{120.f, SR_SVG_UNITS_PERCENTAGE};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGMASK_H_
