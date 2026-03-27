// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
#include <cstdint>
namespace serval::markdown {
class MarkdownScreenMetrics {
 public:
  float DPToPx(float dp) const { return density_ * dp; }
  float PxToDp(float px) const {
    return density_ == 0 ? px : px / density_;
  }
  float GetDensity() const { return density_; }
  void SetDensity(float density) { density_ = density > 0 ? density : 1; }
  int32_t GetScreenWidth() const { return screen_width_; }
  int32_t GetScreenHeight() const { return screen_height_; }
  void SetScreenWidth(int32_t width) { screen_width_ = width; }
  void SetScreenHeight(int32_t height) { screen_height_ = height; }
  float GetScaledScreenWidth() const {
    return PxToDp(static_cast<float>(screen_width_));
  }
  float GetScaledScreenHeight() const {
    return PxToDp(static_cast<float>(screen_height_));
  }

 private:
  float density_{1};
  int32_t screen_width_{0};
  int32_t screen_height_{0};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
