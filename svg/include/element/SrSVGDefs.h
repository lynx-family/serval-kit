// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGDEFS_H_
#define SVG_INCLUDE_ELEMENT_SRSVGDEFS_H_

#include "SrSVGContainer.h"
#include "SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGDefs : public SrSVGContainer {
 public:
  static SrSVGDefs* Make() { return new SrSVGDefs(); }
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;

 private:
  SrSVGDefs() : SrSVGContainer(SrSVGTag::kDefs) {}
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGDEFS_H_
