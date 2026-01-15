// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGPOLYLINE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGPOLYLINE_H_

#include <memory>

#include "SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGPolyLine : public SrSVGShape {
 public:
  static SrSVGPolyLine* Make() { return new SrSVGPolyLine(); }

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  ~SrSVGPolyLine() override;

 private:
  SrPolygon* polygon_{nullptr};
  SrSVGPolyLine() : SrSVGShape(SrSVGTag::kPolyline){};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGPOLYLINE_H_
