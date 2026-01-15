// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGELLIPSE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGELLIPSE_H_

#include <memory>

#include "SrSVGNode.h"
#include "SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGEllipse : public SrSVGShape {
 public:
  static SrSVGEllipse* Make() { return new SrSVGEllipse(); }

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGEllipse() : SrSVGShape(SrSVGTag::kEllipse) {}
  SrSVGLength cx_{0}, cy_{0}, rx_{0}, ry_{0};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGELLIPSE_H_
