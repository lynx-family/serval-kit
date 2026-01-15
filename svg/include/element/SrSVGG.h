// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGG_H_
#define SVG_INCLUDE_ELEMENT_SRSVGG_H_

#include "element/SrSVGContainer.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGG : public SrSVGContainer {
 public:
  static SrSVGG* Make() { return new SrSVGG(); }
  //    bool parseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGG() : SrSVGContainer(SrSVGTag::kG) {}
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGG_H_
