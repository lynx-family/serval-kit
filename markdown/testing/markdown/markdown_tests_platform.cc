// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/markdown/markdown_tests_platform.h"

#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_shaper.h"

namespace serval::markdown {
namespace {

class TestMarkdownPlatform final : public MarkdownPlatform {
 public:
  tttext::TextLayout* GetTextLayout() override {
    thread_local tttext::TextLayout text_layout(
        std::make_unique<testing::MockMarkdownShaper>());
    return &text_layout;
  }

  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) override {
    return static_cast<testing::MockMarkdownCanvas*>(canvas);
  }
};

}  // namespace

namespace testing {

std::unique_ptr<MarkdownPlatform> CreateTestMarkdownPlatform() {
  return std::make_unique<TestMarkdownPlatform>();
}

std::shared_ptr<MarkdownContext> CreateTestMarkdownSharedContext() {
  return std::make_shared<MarkdownContext>(CreateTestMarkdownPlatform());
}

}  // namespace testing
}  // namespace serval::markdown
