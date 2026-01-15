// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGCIRCLE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGCIRCLE_H_

#include <memory>

#include "SrSVGShape.h"
#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGCircle : public SrSVGShape {
 public:
  static SrSVGCircle* Make() { return new SrSVGCircle(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 private:
  SrSVGCircle() : SrSVGShape(SrSVGTag::kCircle) {}
  SrSVGLength cx_{0}, cy_{0}, r_{0};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGCIRCLE_H_
