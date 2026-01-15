// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGIMAGE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGIMAGE_H_

#include <string>

#include "element/SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGImage : public SrSVGShape {
 public:
  static SrSVGImage* Make() { return new SrSVGImage(); }

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;

 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGImage()
      : SrSVGShape(SrSVGTag::kImage),
        preserve_aspect_radio_(make_default_preserve_aspect_radio()) {}

  std::string href_;
  SrSVGLength x_{0}, y_{0}, width_{0}, height_{0};
  SrSVGPreserveAspectRatio preserve_aspect_radio_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGIMAGE_H_
