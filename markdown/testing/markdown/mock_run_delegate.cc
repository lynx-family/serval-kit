// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "testing/markdown/mock_run_delegate.h"

#include "testing/markdown/mock_markdown_canvas.h"
namespace lynx::markdown::testing {
void MockImage::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawImage(
      src_.c_str(), x, y, x + width_, y + height_, nullptr);
}
void MockInlineView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawView(id_.c_str(), x, y,
                                                     x + width_, y + height_);
}
void MockBackgroundDrawable::Draw(tttext::ICanvasHelper* canvas, float left,
                                  float top, float right, float bottom) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawBackground(
      background_.background_image_.c_str(), left, top, right, bottom);
}

}  // namespace lynx::markdown::testing
