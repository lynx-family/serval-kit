// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_RENDERER_SRSVGANIMATIONSTATE_H_
#define SVG_INCLUDE_RENDERER_SRSVGANIMATIONSTATE_H_

#include <cmath>
#include <cstdint>
#include <limits>

namespace serval {
namespace svg {
namespace renderer {

class SrSVGAnimationState {
 public:
  void SetHasAnimations(bool has_animations) {
    has_animations_ = has_animations;
    ResetClock();
    running_ = false;
  }

  void SetAnimationTimelineEndSeconds(double seconds) {
    animation_timeline_end_seconds_ =
        std::isfinite(seconds) && seconds >= 0.0
            ? seconds
            : std::numeric_limits<double>::infinity();
    ClampToTimelineEnd();
    if (IsCompleteAt(current_seconds_)) {
      running_ = false;
      has_start_time_ = false;
    }
  }

  bool HasAnimations() const { return has_animations_; }
  bool IsRunning() const { return running_; }
  bool NeedsAnimationFrame() const {
    return has_animations_ && running_ && !IsCompleteAt(current_seconds_);
  }
  double AnimationTimelineEndSeconds() const {
    return animation_timeline_end_seconds_;
  }
  double CurrentSeconds() const { return current_seconds_; }

  void Start() {
    if (!has_animations_ || IsCompleteAt(current_seconds_)) {
      running_ = false;
      return;
    }
    running_ = true;
    has_start_time_ = false;
  }

  void StartAtTimeNanos(int64_t frame_time_nanos) {
    Start();
    if (running_) {
      start_time_nanos_ = frame_time_nanos - SecondsToNanos(current_seconds_);
      has_start_time_ = true;
    }
  }

  void StartAtTimeSeconds(double frame_time_seconds) {
    StartAtTimeNanos(SecondsToNanos(frame_time_seconds));
  }

  void Stop() {
    running_ = false;
    has_start_time_ = false;
  }

  void ResetClock() {
    current_seconds_ = 0.0;
    start_time_nanos_ = 0;
    has_start_time_ = false;
  }

  bool OnFrameTimeNanos(int64_t frame_time_nanos) {
    if (!has_animations_) {
      return false;
    }
    if (!running_) {
      Start();
      if (!running_) {
        return false;
      }
    }
    if (!has_start_time_) {
      start_time_nanos_ = frame_time_nanos - SecondsToNanos(current_seconds_);
      has_start_time_ = true;
    }
    const int64_t elapsed_nanos = frame_time_nanos - start_time_nanos_;
    if (elapsed_nanos < 0) {
      current_seconds_ = 0.0;
      if (IsCompleteAt(current_seconds_)) {
        running_ = false;
        has_start_time_ = false;
        return false;
      }
      return true;
    }
    current_seconds_ = static_cast<double>(elapsed_nanos) / kNanosPerSecond;
    ClampToTimelineEnd();
    if (IsCompleteAt(current_seconds_)) {
      running_ = false;
      has_start_time_ = false;
      return false;
    }
    return true;
  }

  bool OnFrameTimeSeconds(double frame_time_seconds) {
    return OnFrameTimeNanos(SecondsToNanos(frame_time_seconds));
  }

  void SetCurrentSeconds(double seconds) {
    current_seconds_ = seconds > 0.0 ? seconds : 0.0;
    ClampToTimelineEnd();
    has_start_time_ = false;
  }

 private:
  static constexpr double kNanosPerSecond = 1000000000.0;

  static int64_t SecondsToNanos(double seconds) {
    if (seconds <= 0.0) {
      return 0;
    }
    return static_cast<int64_t>(seconds * kNanosPerSecond);
  }

  bool IsCompleteAt(double seconds) const {
    return std::isfinite(animation_timeline_end_seconds_) &&
           seconds >= animation_timeline_end_seconds_;
  }

  void ClampToTimelineEnd() {
    if (std::isfinite(animation_timeline_end_seconds_) &&
        current_seconds_ > animation_timeline_end_seconds_) {
      current_seconds_ = animation_timeline_end_seconds_;
    }
  }

  bool has_animations_{false};
  bool running_{false};
  bool has_start_time_{false};
  int64_t start_time_nanos_{0};
  double current_seconds_{0.0};
  double animation_timeline_end_seconds_{
      std::numeric_limits<double>::infinity()};
};

}  // namespace renderer
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_RENDERER_SRSVGANIMATIONSTATE_H_
