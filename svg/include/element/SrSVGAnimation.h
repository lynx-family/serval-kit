// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
#define SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_

#include <cstdint>
#include <string>
#include <vector>

#include "element/SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGAnimation final : public SrSVGNodeBase {
 public:
  struct Effect {
    std::string attribute;
    std::string value;
    bool additive{false};
    bool transform{false};
    bool motion{false};
  };

  static SrSVGAnimation* MakeAnimate() {
    return new SrSVGAnimation(SrSVGTag::kAnimate);
  }
  static SrSVGAnimation* MakeAnimateColor() {
    return new SrSVGAnimation(SrSVGTag::kAnimateColor);
  }
  static SrSVGAnimation* MakeAnimateMotion() {
    return new SrSVGAnimation(SrSVGTag::kAnimateMotion);
  }
  static SrSVGAnimation* MakeAnimateTransform() {
    return new SrSVGAnimation(SrSVGTag::kAnimateTransform);
  }
  static SrSVGAnimation* MakeSet() {
    return new SrSVGAnimation(SrSVGTag::kSet);
  }
  static SrSVGAnimation* MakeMPath() {
    return new SrSVGAnimation(SrSVGTag::kMPath);
  }

  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void AppendChild(SrSVGNodeBase* child) override;
  bool Evaluate(double seconds, const IDMapper* id_mapper,
                const std::string& underlying, Effect* effect) const;
  const std::string& TargetHref() const { return target_href_; }
  std::string TargetAttributeName() const {
    return Tag() == SrSVGTag::kAnimateMotion ||
                   Tag() == SrSVGTag::kAnimateTransform
               ? "transform"
               : attribute_name_;
  }

 private:
  explicit SrSVGAnimation(SrSVGTag tag) : SrSVGNodeBase(tag) {}

  struct ActiveState {
    double progress{0.0};
    uint64_t repeat_index{0};
    bool frozen{false};
  };
  enum class BeginType {
    kStatic,
    kSyncBegin,
    kSyncEnd,
  };
  struct BeginSpec {
    BeginType type{BeginType::kStatic};
    std::string sync_id;
    double offset{0.0};
  };

  std::string MakeValue(const ActiveState& state,
                        const std::string& underlying) const;
  bool ActiveStateAt(double seconds, const IDMapper* id_mapper,
                     ActiveState* state) const;
  void ParseBegin(const char* value);
  double ResolvedBeginSeconds(const IDMapper* id_mapper, int depth) const;
  double ResolvedActiveDuration(const IDMapper* id_mapper, int depth) const;

  std::string attribute_name_;
  std::string target_href_;
  std::string from_;
  std::string to_;
  std::string by_;
  std::vector<std::string> values_;
  std::vector<double> key_times_;
  std::vector<double> key_points_;
  std::vector<double> key_splines_;
  std::string transform_type_;
  std::string path_href_;
  std::string path_;
  std::string rotate_;
  std::string calc_mode_;
  double begin_{0.0};
  double dur_{0.0};
  double end_{0.0};
  double repeat_dur_{0.0};
  double min_{0.0};
  double max_{0.0};
  std::vector<BeginSpec> begin_specs_;
  bool has_begin_{false};
  bool dur_indefinite_{false};
  bool repeat_indefinite_{false};
  bool repeat_dur_indefinite_{false};
  double repeat_count_{1.0};
  bool has_end_{false};
  bool has_repeat_dur_{false};
  bool has_min_{false};
  bool has_max_{false};
  bool freeze_{false};
  bool additive_sum_{false};
  bool accumulate_sum_{false};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
