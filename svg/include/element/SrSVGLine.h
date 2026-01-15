// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGLINE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGLINE_H_

#include <memory>

#include "SrSVGNode.h"
#include "SrSVGShape.h"
#include "SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGLine : public SrSVGShape {
 public:
  static SrSVGLine* Make() { return new SrSVGLine(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;

 private:
  SrSVGLine() : SrSVGShape(SrSVGTag::kLine) {}
  SrSVGLength x1_{}, y1_{}, x2_{}, y2_{};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGLINE_H_
