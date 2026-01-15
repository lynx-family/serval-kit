// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGSVG_H_
#define SVG_INCLUDE_ELEMENT_SRSVGSVG_H_

#include "SrSVGContainer.h"
#include "SrSVGNode.h"
#include "SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGSVG : public SrSVGContainer {
 public:
  SrSVGSVG(SrSVGTag tag);
  ~SrSVGSVG();

  bool ParseAndSetAttribute(const char* name, const char* value) override;
  static SrSVGSVG* Make() { return new SrSVGSVG(SrSVGTag::kSvg); }
  inline SrSVGBox viewBox() { return this->view_box_; }

 protected:
  bool OnPrepareToRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const override;

 private:
  //  void calculateViewBoxTransform(const SrSVGBox& view_port, float* xform) const;
  SrSVGPreserveAspectRatio preserve_aspect_radio_;
  SrSVGBox view_box_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGSVG_H_
