// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
#define SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_

#include <string>
#include <vector>

#include "element/SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGAnimation final : public SrSVGNodeBase {
 public:
  static SrSVGAnimation* MakeAnimate() {
    return new SrSVGAnimation(SrSVGTag::kAnimate);
  }
  static SrSVGAnimation* MakeAnimateTransform() {
    return new SrSVGAnimation(SrSVGTag::kAnimateTransform);
  }

  bool ParseAndSetAttribute(const char* name, const char* value) override;
  bool Evaluate(double seconds, std::string* attribute,
                std::string* value) const;

 private:
  explicit SrSVGAnimation(SrSVGTag tag) : SrSVGNodeBase(tag) {}

  std::string MakeValue(double seconds) const;
  double ActiveProgress(double seconds) const;

  std::string attribute_name_;
  std::string from_;
  std::string to_;
  std::string by_;
  std::vector<std::string> values_;
  std::string transform_type_;
  double begin_{0.0};
  double dur_{0.0};
  bool repeat_indefinite_{false};
  double repeat_count_{1.0};
  bool freeze_{false};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
