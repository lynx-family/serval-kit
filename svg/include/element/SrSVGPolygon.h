// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGPOLYGON_H_
#define SVG_INCLUDE_ELEMENT_SRSVGPOLYGON_H_

#include <memory>

#include "SrSVGNode.h"
#include "SrSVGShape.h"
#include "SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGPolygon : public SrSVGShape {
 public:
  static SrSVGPolygon* Make() { return new SrSVGPolygon(); }

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  ~SrSVGPolygon() override;

 private:
  SrPolygon* polygon_{nullptr};
  SrSVGPolygon() : SrSVGShape(SrSVGTag::kPolygon){};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGPOLYGON_H_
