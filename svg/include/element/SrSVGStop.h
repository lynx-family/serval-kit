// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_
#define SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGStop : public SrSVGNodeBase {
 public:
  static SrSVGStop* Make() { return new SrSVGStop(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void StoreAttribute(const char* name, const char* value) override;
  void AddAnimation(SrSVGAnimation* animation) override;
  bool HasAnimations() const override { return !animations_.empty(); }
  void ApplyAnimations(double seconds, const IDMapper* id_mapper) override;
  void RestoreAnimatedAttributes() override;
  float offset(SrSVGRenderContext& context) const;
  float opacity(SrSVGRenderContext& context) const;

 private:
  SrSVGStop() : SrSVGNodeBase(SrSVGTag::kStop) {
    // default opacity is 1.f
    stop_.stopOpacity =
        (SrSVGLength){.value = 1.0f, .unit = SR_SVG_UNITS_NUMBER};
    // default offset is 0.f
    stop_.offset = (SrSVGLength){.value = 0.f, .unit = SR_SVG_UNITS_NUMBER};
    stop_.stopColor = make_serval_color("black");
  }
  void RestoreAnimatedAttribute(const std::string& name,
                                const std::optional<std::string>& base_value);
  void ClearAnimatedAttribute(const std::string& name);

 public:
  SrStop stop_;

 private:
  std::unordered_map<std::string, std::string> base_attributes_;
  std::unordered_map<std::string, std::optional<std::string>>
      animated_attributes_;
  std::vector<SrSVGAnimation*> animations_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGSTOP_H_
