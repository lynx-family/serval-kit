// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGPATH_H_
#define SVG_INCLUDE_ELEMENT_SRSVGPATH_H_

#include <memory>

#include "SrSVGNode.h"
#include "SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGPath : public SrSVGShape {
 public:
  static SrSVGPath* Make() { return new SrSVGPath(); }

 private:
 protected:
 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  ~SrSVGPath() override;

 protected:
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override;

 private:
  SrPathData* path_{nullptr};
  SrSVGPath() : SrSVGShape(SrSVGTag::kPath){};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGPATH_H_
