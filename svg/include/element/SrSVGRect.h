// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGRECT_H_
#define SVG_INCLUDE_ELEMENT_SRSVGRECT_H_

#include <memory>

#include "SrSVGShape.h"
#include "SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGRect : public SrSVGShape {
 public:
  static SrSVGRect* Make() { return new SrSVGRect(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  void onDraw(canvas::SrCanvas* const canvas,
              SrSVGRenderContext& context) const override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 private:
  SrSVGRect() : SrSVGShape(SrSVGTag::kRect) {}
  SrSVGLength x_{}, y_{}, width_{}, height_{}, rx_{}, ry_{};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGRECT_H_
