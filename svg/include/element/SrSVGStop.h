// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_
#define SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_

#include "SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGStop : public SrSVGNodeBase {
 public:
  static SrSVGStop* Make() { return new SrSVGStop(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  float offset(SrSVGRenderContext& context) const;
  float opacity(SrSVGRenderContext& context) const;

 private:
  SrSVGStop() : SrSVGNodeBase(SrSVGTag::kStop) {
    // default opacity is 1.f
    stop_.stopOpacity =
        (SrSVGLength){.value = 1.0f, .unit = SR_SVG_UNITS_NUMBER};
    // default offset is 1.f
    stop_.offset = (SrSVGLength){.value = 0.f, .unit = SR_SVG_UNITS_NUMBER};
  }

 public:
  SrStop stop_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_
