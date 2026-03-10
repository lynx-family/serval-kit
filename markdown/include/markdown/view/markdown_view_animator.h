// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_ANIMATOR_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_ANIMATOR_H_

#include <cstdint>

#include "markdown/view/markdown_view_measurer.h"

namespace lynx::markdown {

class MarkdownViewAnimator {
 public:
  MarkdownViewAnimator() = default;
  ~MarkdownViewAnimator() = default;
  void SetEventListener(MarkdownEventListener* event_listener) {
    event_listener_ = event_listener;
  }
  void SetAnimationType(MarkdownAnimationType type) { animation_type_ = type; }
  MarkdownAnimationType GetAnimationType() const { return animation_type_; }

  void SetAnimationVelocity(float velocity) { animation_velocity_ = velocity; }
  float GetAnimationVelocity() const { return animation_velocity_; }

  void SetTypewriterDynamicHeight(bool enable) {
    typewriter_dynamic_height_ = enable;
  }
  bool GetTypewriterDynamicHeight() const { return typewriter_dynamic_height_; }

  void SetInitialAnimationStep(int32_t step) {
    initial_animation_step_ = step;
    SetAnimationStep(step);
  }
  int32_t GetInitialAnimationStep() const { return initial_animation_step_; }

  void SetHeightTransitionDuration(float duration) {
    height_transition_duration_ = duration;
  }
  float GetHeightTransitionDuration() const {
    return height_transition_duration_;
  }

  void SetMaxAnimationStep(int32_t max_step) { max_animation_step_ = max_step; }
  int32_t GetMaxAnimationStep() const { return max_animation_step_; }

  void SetAnimationStep(int32_t step);
  int32_t GetAnimationStep() const { return current_animation_step_; }

  void UpdateCurrentTime(int64_t frame_time_ms) {
    current_frame_time_ms_ = frame_time_ms;
  }
  int64_t GetCurrentFrameTimeMs() const { return current_frame_time_ms_; }

  int32_t UpdateTypewriterStep();
  int32_t GetAnimationStepAfter(float seconds) const;

  float UpdateHeightTransition(float target_height);
  bool IsHeightTransitionRunning() const;

  void ResetAnimationRuntime();

 private:
  void SendAnimationStep(int32_t animation_step,
                         int32_t max_animation_step) const;

 private:
  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  float animation_velocity_{1};
  bool typewriter_dynamic_height_{true};
  int32_t initial_animation_step_{0};
  float height_transition_duration_{0};

  MarkdownEventListener* event_listener_{nullptr};

  int64_t current_frame_time_ms_{0};
  int32_t current_animation_step_{0};
  int32_t max_animation_step_{0};
  int64_t current_animation_step_time_ms_{0};
  int32_t last_sent_step_{-1};

  float transition_start_height_{0};
  float transition_end_height_{0};
  int64_t transition_start_time_ms_{0};
  float current_transition_height_{0};
  int64_t current_transition_time_ms_{0};
};

}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_ANIMATOR_H_
