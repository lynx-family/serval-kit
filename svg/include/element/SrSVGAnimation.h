// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
#define SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "element/SrSVGNode.h"

namespace serval {
namespace svg {
namespace element {

struct SrSVGMotionPathCache {
  std::string source_key;
  const SrPathData* source_path{nullptr};
  uint32_t n_ops{0};
  uint32_t n_args{0};
  uint64_t hash{0};
  std::vector<double> coordinates;
  std::vector<double> cumulative_lengths;
  double total_length{0.0};
};

struct SrSVGPathPairCache {
  std::string from;
  std::string to;
  SrPathData* from_path{nullptr};
  SrPathData* to_path{nullptr};
  bool compatible{false};
};

class SrSVGAnimation final : public SrSVGNodeBase {
 public:
  struct Effect {
    std::string attribute;
    std::string value;
    const SrPathData* path_data{nullptr};
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

  ~SrSVGAnimation() override;
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void AppendChild(SrSVGNodeBase* child) override;
  bool Evaluate(double seconds, const IDMapper* id_mapper,
                const std::string& underlying, Effect* effect) const;
  double LastChangeSeconds(const IDMapper* id_mapper) const;
  const std::string& TargetHref() const { return target_href_; }
  std::string TargetAttributeName() const {
    if (Tag() == SrSVGTag::kAnimateMotion) {
      return "transform";
    }
    if (Tag() == SrSVGTag::kAnimateTransform) {
      return attribute_name_.empty() ? "transform" : attribute_name_;
    }
    return attribute_name_;
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
  enum class Restart {
    kAlways,
    kWhenNotActive,
    kNever,
  };
  struct BeginSpec {
    BeginType type{BeginType::kStatic};
    std::string sync_id;
    double offset{0.0};
  };

  std::string MakeValue(const ActiveState& state,
                        const std::string& underlying) const;
  const SrPathData* MakePathDataValue(const ActiveState& state,
                                      const std::string& underlying,
                                      std::string* fallback_value) const;
  bool ActiveStateAt(double seconds, const IDMapper* id_mapper,
                     ActiveState* state) const;
  void ResetCaches() const;
  void ResetMotionPathCache() const;
  void ResetPathDataCaches() const;
  bool EnsureMotionPathCacheForPathString(const std::string& path) const;
  bool EnsureMotionPathCacheForPathData(const std::string& source_key,
                                        const SrPathData* path_data) const;
  const SrSVGPathPairCache* EnsurePathPairCache(const std::string& from,
                                                const std::string& to) const;
  const SrPathData* InterpolateCachedPathData(const std::string& from,
                                              const std::string& to,
                                              double progress) const;
  void ParseBegin(const char* value);
  void ParseEnd(const char* value);
  void ParseTimeSpecList(const char* value, std::vector<BeginSpec>* specs,
                         double* earliest_static) const;
  std::vector<double> ResolvedTimeSpecSeconds(
      const std::vector<BeginSpec>& specs, const IDMapper* id_mapper,
      int depth) const;
  std::vector<double> ResolvedBeginSecondsList(const IDMapper* id_mapper,
                                               int depth) const;
  std::vector<double> ResolvedEndSecondsList(const IDMapper* id_mapper,
                                             int depth) const;
  double ResolvedBeginSeconds(const IDMapper* id_mapper, int depth) const;
  double ResolvedActiveDurationForBegin(double begin, const IDMapper* id_mapper,
                                        int depth) const;
  double ResolvedActiveDuration(const IDMapper* id_mapper, int depth) const;
  double ResolvedLastChangeSeconds(const IDMapper* id_mapper, int depth) const;

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
  Restart restart_{Restart::kAlways};
  std::vector<BeginSpec> end_specs_;
  mutable SrSVGMotionPathCache motion_path_cache_;
  mutable std::vector<SrSVGPathPairCache> path_pair_caches_;
  mutable SrPathData* interpolated_path_cache_{nullptr};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGANIMATION_H_
