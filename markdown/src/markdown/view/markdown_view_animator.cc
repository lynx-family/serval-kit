// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view_animator.h"

#include <algorithm>
#include <cmath>

#include "markdown/utils/markdown_float_comparison.h"

namespace serval::markdown {

void MarkdownViewAnimator::SetAnimationType(MarkdownAnimationType type) {
  animation_type_ = type;
  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    SyncLineExpandStateFromAnimationStep();
  }
}

void MarkdownViewAnimator::SetMaxAnimationStep(int32_t max_step) {
  max_animation_step_ = std::max(0, max_step);
  current_animation_step_ = ClampAnimationStep(current_animation_step_);
  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    SyncLineExpandStateFromAnimationStep();
  }
}

void MarkdownViewAnimator::SetLineExpandLineEndSteps(
    std::vector<int32_t> line_end_steps) {
  line_expand_line_end_steps_ = std::move(line_end_steps);
  max_animation_line_ =
      static_cast<int32_t>(line_expand_line_end_steps_.size());
  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    SyncLineExpandStateFromAnimationStep();
  }
}

bool MarkdownViewAnimator::IsLineExpandComplete() const {
  return animation_type_ == MarkdownAnimationType::kLineExpand &&
         max_animation_line_ > 0 &&
         line_expand_shown_lines_ >= max_animation_line_;
}

void MarkdownViewAnimator::SetAnimationStep(int32_t step) {
  current_animation_step_ = ClampAnimationStep(step);
  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    SyncLineExpandStateFromAnimationStep();
  }
  current_animation_step_time_ms_ = 0;
}

void MarkdownViewAnimator::ResetAnimationRuntime() {
  current_frame_time_ms_ = 0;
  current_animation_step_time_ms_ = 0;
  transition_start_time_ms_ = 0;
  current_transition_time_ms_ = 0;
  transition_start_height_ = 0;
  transition_end_height_ = 0;
  current_transition_height_ = 0;
}

int32_t MarkdownViewAnimator::ClampAnimationStep(int32_t step) const {
  step = std::max(0, step);
  if (max_animation_step_ > 0) {
    step = std::min(step, max_animation_step_);
  }
  return step;
}

int32_t MarkdownViewAnimator::CalculateAnimationAdvanceCount() {
  if (animation_velocity_ <= 0 || current_frame_time_ms_ <= 0) {
    return 0;
  }

  const float interval_ms = 1000.0f / animation_velocity_;
  if (current_animation_step_time_ms_ == 0) {
    current_animation_step_time_ms_ = current_frame_time_ms_;
    return 1;
  }

  const auto duration =
      current_frame_time_ms_ - current_animation_step_time_ms_;
  if (duration <= 0) {
    current_animation_step_time_ms_ = current_frame_time_ms_;
    return 0;
  }

  const auto step_count =
      static_cast<int32_t>(static_cast<float>(duration) / interval_ms);
  current_animation_step_time_ms_ +=
      static_cast<int64_t>(static_cast<float>(step_count) * interval_ms);
  return step_count;
}

int32_t MarkdownViewAnimator::LineExpandShownLinesToCharStep(
    int32_t shown_lines) const {
  if (shown_lines <= 0 || line_expand_line_end_steps_.empty()) {
    return 0;
  }
  const int32_t line_index = std::min(shown_lines - 1, max_animation_line_ - 1);
  return ClampAnimationStep(line_expand_line_end_steps_[line_index]);
}

int32_t MarkdownViewAnimator::LineExpandCharStepToShownLines(
    int32_t char_step) const {
  if (char_step <= 0 || line_expand_line_end_steps_.empty()) {
    return 0;
  }
  const auto iter =
      std::lower_bound(line_expand_line_end_steps_.begin(),
                       line_expand_line_end_steps_.end(), char_step);
  if (iter == line_expand_line_end_steps_.end()) {
    return max_animation_line_;
  }
  return static_cast<int32_t>(iter - line_expand_line_end_steps_.begin()) + 1;
}

void MarkdownViewAnimator::SyncLineExpandStateFromAnimationStep() {
  line_expand_shown_lines_ =
      LineExpandCharStepToShownLines(current_animation_step_);
  if (!line_expand_line_end_steps_.empty()) {
    current_animation_step_ =
        LineExpandShownLinesToCharStep(line_expand_shown_lines_);
  }
}

int32_t MarkdownViewAnimator::UpdateAnimationStep() {
  if (animation_type_ == MarkdownAnimationType::kNone) {
    return 0;
  }
  if (animation_type_ == MarkdownAnimationType::kTypewriter) {
    if (current_animation_step_ >= max_animation_step_ ||
        max_animation_step_ <= 0) {
      return 0;
    }
  } else if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    if (line_expand_shown_lines_ >= max_animation_line_ ||
        max_animation_line_ <= 0) {
      return 0;
    }
  } else {
    return 0;
  }

  const int32_t step_count = CalculateAnimationAdvanceCount();
  if (step_count <= 0) {
    return 0;
  }

  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    line_expand_shown_lines_ =
        std::min(line_expand_shown_lines_ + step_count, max_animation_line_);
    current_animation_step_ =
        LineExpandShownLinesToCharStep(line_expand_shown_lines_);
    if (line_expand_shown_lines_ >= max_animation_line_) {
      current_animation_step_time_ms_ = 0;
    }
  } else {
    current_animation_step_ += step_count;
    if (current_animation_step_ >= max_animation_step_) {
      current_animation_step_ = max_animation_step_;
      current_animation_step_time_ms_ = 0;
    }
  }
  if (last_sent_step_ != current_animation_step_) {
    SendAnimationStep(current_animation_step_, max_animation_step_);
    last_sent_step_ = current_animation_step_;
  }
  return step_count;
}

int32_t MarkdownViewAnimator::UpdateTypewriterStep() {
  return UpdateAnimationStep();
}

int32_t MarkdownViewAnimator::GetAnimationStepAfter(float seconds) const {
  if (seconds <= 0 || animation_velocity_ <= 0) {
    return current_animation_step_;
  }
  const int32_t step_offset =
      static_cast<int32_t>(std::floor(animation_velocity_ * seconds + 1e-4f));
  if (animation_type_ == MarkdownAnimationType::kLineExpand) {
    const int32_t shown_lines = std::max(
        0,
        std::min(line_expand_shown_lines_ + step_offset, max_animation_line_));
    return LineExpandShownLinesToCharStep(shown_lines);
  }
  return ClampAnimationStep(current_animation_step_ + step_offset);
}

float MarkdownViewAnimator::UpdateHeightTransition(float target_height) {
  if (height_transition_duration_ <= 0) {
    transition_start_height_ = target_height;
    transition_end_height_ = target_height;
    current_transition_height_ = target_height;
    transition_start_time_ms_ = 0;
    current_transition_time_ms_ = 0;
    return target_height;
  }

  if (transition_start_time_ms_ == 0) {
    transition_start_height_ = target_height;
    transition_end_height_ = target_height;
    transition_start_time_ms_ = current_frame_time_ms_;
    current_transition_height_ = target_height;
    current_transition_time_ms_ = current_frame_time_ms_;
    return target_height;
  }

  if (FloatsNotEqual(transition_end_height_, target_height)) {
    if (FloatsEqual(transition_end_height_, current_transition_height_)) {
      transition_start_time_ms_ = current_frame_time_ms_;
    } else {
      transition_start_time_ms_ = current_transition_time_ms_;
    }
    transition_end_height_ = target_height;
    transition_start_height_ = current_transition_height_;
  }

  int64_t duration_ms = std::max(0.0f, height_transition_duration_) * 1000;
  duration_ms = std::max<int64_t>(1, duration_ms);
  if (current_frame_time_ms_ >= transition_start_time_ms_ + duration_ms) {
    current_transition_height_ = transition_end_height_;
  } else {
    current_transition_height_ =
        transition_start_height_ +
        (transition_end_height_ - transition_start_height_) *
            static_cast<float>(current_frame_time_ms_ -
                               transition_start_time_ms_) /
            duration_ms;
  }
  current_transition_time_ms_ = current_frame_time_ms_;
  return current_transition_height_;
}

bool MarkdownViewAnimator::IsHeightTransitionRunning() const {
  return height_transition_duration_ > 0 &&
         FloatsNotEqual(transition_end_height_, current_transition_height_);
}
void MarkdownViewAnimator::SendAnimationStep(int32_t animation_step,
                                             int32_t max_animation_step) const {
  if (event_listener_ == nullptr) {
    return;
  }
  event_listener_->OnAnimationStep(animation_step, max_animation_step);
}

}  // namespace serval::markdown
