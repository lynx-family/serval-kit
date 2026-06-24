// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_RENDERER_SRSVGANIMATEDRENDERER_H_
#define SVG_INCLUDE_RENDERER_SRSVGANIMATEDRENDERER_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "renderer/SrSVGAnimationState.h"

namespace serval {
namespace svg {
namespace renderer {

class SrSVGAnimatedRenderer {
 public:
  bool SetContent(const char* content, size_t length,
                  std::vector<parser::SrSVGDiagnostic>* diagnostics) {
    if (content == nullptr || length == 0) {
      SetDOM(nullptr);
      return false;
    }
    std::unique_ptr<parser::SrSVGDOM> dom;
    dom = parser::SrSVGDOM::make(content, length, diagnostics);
    const bool success = dom != nullptr;
    SetDOM(std::move(dom));
    return success;
  }

  void SetDOM(std::unique_ptr<parser::SrSVGDOM> dom) {
    std::lock_guard<std::mutex> lock(mutex_);
    dom_ = std::move(dom);
    animation_state_.SetHasAnimations(dom_ && dom_->HasAnimations());
    animation_state_.SetAnimationTimelineEndSeconds(
        dom_ ? dom_->AnimationTimelineEndSeconds() : 0.0);
  }

  bool HasContent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dom_ != nullptr;
  }
  bool HasAnimations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.HasAnimations();
  }
  bool IsRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.IsRunning();
  }
  bool NeedsAnimationFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.NeedsAnimationFrame();
  }
  double AnimationTimelineEndSeconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.AnimationTimelineEndSeconds();
  }
  double CurrentAnimationSeconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.CurrentSeconds();
  }

  void StartAnimation() {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.Start();
  }
  void StartAnimationAtTimeNanos(int64_t frame_time_nanos) {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.StartAtTimeNanos(frame_time_nanos);
  }
  void StartAnimationAtTimeSeconds(double frame_time_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.StartAtTimeSeconds(frame_time_seconds);
  }
  void StopAnimation() {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.Stop();
  }
  void ResetAnimationClock() {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.ResetClock();
  }
  void SetAnimationSeconds(double seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    animation_state_.SetCurrentSeconds(seconds);
  }
  bool OnFrameTimeNanos(int64_t frame_time_nanos) {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.OnFrameTimeNanos(frame_time_nanos);
  }
  bool OnFrameTimeSeconds(double frame_time_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    return animation_state_.OnFrameTimeSeconds(frame_time_seconds);
  }

  void SetDefaultColor(uint32_t color) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_color_ = color;
  }
  void ResetDefaultColor() {
    std::lock_guard<std::mutex> lock(mutex_);
    default_color_.reset();
  }

  void Render(canvas::SrCanvas* canvas) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dom_ || !canvas) {
      return;
    }
    ApplyDefaultColor();
    RenderLocked(canvas);
  }

  void Render(canvas::SrCanvas* canvas, std::optional<uint32_t> default_color) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dom_ || !canvas) {
      return;
    }
    ApplyDefaultColor(default_color);
    RenderLocked(canvas);
  }

  void Render(canvas::SrCanvas* canvas, SrSVGBox view_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dom_ || !canvas) {
      return;
    }
    ApplyDefaultColor();
    RenderLocked(canvas, view_port);
  }

  void Render(canvas::SrCanvas* canvas, SrSVGBox view_port,
              std::optional<uint32_t> default_color) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dom_ || !canvas) {
      return;
    }
    ApplyDefaultColor(default_color);
    RenderLocked(canvas, view_port);
  }

 private:
  void RenderLocked(canvas::SrCanvas* canvas) {
    if (animation_state_.HasAnimations()) {
      dom_->RenderAtTime(canvas, animation_state_.CurrentSeconds());
    } else {
      dom_->Render(canvas);
    }
  }

  void RenderLocked(canvas::SrCanvas* canvas, SrSVGBox view_port) {
    if (animation_state_.HasAnimations()) {
      dom_->RenderAtTime(canvas, view_port, animation_state_.CurrentSeconds());
    } else {
      dom_->Render(canvas, view_port);
    }
  }

  void ApplyDefaultColor() { ApplyDefaultColor(default_color_); }

  void ApplyDefaultColor(std::optional<uint32_t> default_color) {
    if (!dom_) {
      return;
    }
    if (default_color.has_value()) {
      dom_->SetDefaultColor(*default_color);
    } else {
      dom_->ResetDefaultColor();
    }
  }

  mutable std::mutex mutex_;
  std::unique_ptr<parser::SrSVGDOM> dom_;
  SrSVGAnimationState animation_state_;
  std::optional<uint32_t> default_color_;
};

}  // namespace renderer
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_RENDERER_SRSVGANIMATEDRENDERER_H_
