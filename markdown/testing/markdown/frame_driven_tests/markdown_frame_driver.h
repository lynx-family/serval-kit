// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_FRAME_DRIVER_H_
#define MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_FRAME_DRIVER_H_

#include <cstdint>
#include <vector>

#include "rapidjson/document.h"
#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"
#include "testing/markdown/mock_platform/mock_markdown_canvas.h"
#include "testing/markdown/mock_platform/mock_markdown_platform_view.h"

namespace serval::markdown::testing {

class MarkdownFrameDriver {
  static constexpr int64_t FRAME_INTERVAL = 100;

 public:
  MarkdownFrameDriver(MockMarkdownMainView* main_view,
                      MockMarkdownCanvas* canvas)
      : main_view_(main_view), canvas_(canvas) {}

  void SetMeasureSpec(MeasureSpec spec);
  MeasureSpec GetMeasureSpec() const { return spec_; }

  void FlushFrame();
  int64_t CurrentTimestamp() const;

  const rapidjson::Document& RunSteps(
      const std::vector<MarkdownFrameStep>& steps);
  void Act(const MarkdownAction& action);

 private:
  MockMarkdownMainView* main_view_;
  MockMarkdownCanvas* canvas_;

  MeasureSpec spec_;
  bool spec_changed_{};
  int64_t current_timestamp_ms_{0};

  rapidjson::Document result_;
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_FRAME_DRIVEN_TESTS_MARKDOWN_FRAME_DRIVER_H_
