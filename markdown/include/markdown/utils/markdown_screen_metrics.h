// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
#include <cstdint>
namespace lynx::markdown {
class MarkdownScreenMetrics {
 public:
  static float DPToPx(float dp) { return GetDpi() * dp; }
  static float PxToDp(float px) { return px / GetDpi(); }
  static float GetDpi() { return Ins().dpi_; }
  static void SetDpi(float density) { Ins().dpi_ = density; }
  static int32_t GetScreenWidth() { return Ins().screen_width_; }
  static int32_t GetScreenHeight() { return Ins().screen_height_; }
  static void SetScreenWidth(int32_t width) { Ins().screen_width_ = width; }
  static void SetScreenHeight(int32_t height) { Ins().screen_height_ = height; }
  static float GetScaledScreenWidth() {
    return PxToDp(static_cast<float>(GetScreenWidth()));
  }
  static float GetScaledScreenHeight() {
    return PxToDp(static_cast<float>(GetScreenHeight()));
  }

 private:
  static MarkdownScreenMetrics& Ins();

  float dpi_{1};
  int32_t screen_width_{0};
  int32_t screen_height_{0};
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_SCREEN_METRICS_H_
