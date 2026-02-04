// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_platform.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_shaper.h"
namespace lynx {
namespace markdown {
tttext::TextLayout* MarkdownPlatform::GetTextLayout() {
  thread_local tttext::TextLayout text_layout(
      std::make_unique<testing::MockMarkdownShaper>());
  return &text_layout;
}
MarkdownCanvasExtend* MarkdownPlatform::GetMarkdownCanvasExtend(
    tttext::ICanvasHelper* canvas) {
  return static_cast<testing::MockMarkdownCanvas*>(canvas);
}
}  // namespace markdown
}  // namespace lynx
