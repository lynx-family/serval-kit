// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGCLIPPATH_H_
#define SVG_INCLUDE_ELEMENT_SRSVGCLIPPATH_H_

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGClipPath : public SrSVGContainer {
 public:
  static SrSVGClipPath* Make() {
    return new SrSVGClipPath(SrSVGTag::kClipPath);
  }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  inline SrSVGObjectBoundingBoxUnitType clip_path_units() const {
    return clip_path_units_;
  };
  SrSVGFillRule clip_rule() const { return clip_rule_; }

 protected:
  explicit SrSVGClipPath(SrSVGTag t) : SrSVGContainer(t){};

 private:
  SrSVGObjectBoundingBoxUnitType clip_path_units_{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  // default to nonzero
  SrSVGFillRule clip_rule_ = SR_SVG_FILL;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGCLIPPATH_H_
