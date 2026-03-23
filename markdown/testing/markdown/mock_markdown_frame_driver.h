// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_FRAME_DRIVER_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_FRAME_DRIVER_H_

#include <cstdint>
#include <vector>

#include "rapidjson/document.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_platform_view.h"

namespace serval::markdown::testing {

struct MockMarkdownFrameStep {
  int64_t interval_ms_{16};
  RectF visible_rect_{};
  bool has_visible_rect_{false};
};

class MockMarkdownFrameDriver {
 public:
  MockMarkdownFrameDriver(MockMarkdownMainView* main_view,
                          MockMarkdownCanvas* canvas)
      : main_view_(main_view), canvas_(canvas) {}

  void SetMeasureSpec(MeasureSpec spec) { measure_spec_ = spec; }
  void SetDefaultVisibleRect(RectF rect) { default_visible_rect_ = rect; }
  void SetAlignPosition(PointF align_position) {
    align_position_ = align_position;
  }

  rapidjson::Document Run(const std::vector<MockMarkdownFrameStep>& steps);

 private:
  bool FlushView(MockMarkdownPlatformView* view) const;
  static rapidjson::Value MakeRectValue(
      RectF rect, rapidjson::Document::AllocatorType& alloc);

 private:
  MockMarkdownMainView* main_view_;
  MockMarkdownCanvas* canvas_;

  MeasureSpec measure_spec_{};
  RectF default_visible_rect_{};
  PointF align_position_{0, 0};
  int64_t current_timestamp_ms_{0};
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_FRAME_DRIVER_H_
