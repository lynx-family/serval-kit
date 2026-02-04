// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "testing/markdown/mock_run_delegate.h"

#include "markdown/draw/markdown_path.h"
#include "markdown/utils/markdown_platform.h"
#include "testing/markdown/mock_markdown_canvas.h"
namespace lynx::markdown::testing {
void MockImage::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  if (radius_ > 0) {
    canvas->Save();
    auto extend = MarkdownPlatform::GetMarkdownCanvasExtend(canvas);
    if (extend != nullptr) {
      MarkdownPath path;
      path.AddRoundRect({.rect_ = RectF::MakeLTWH(x, y, width_, height_),
                         .radius_x_ = radius_,
                         .radius_y_ = radius_});
      extend->ClipPath(&path);
    }
  }
  static_cast<MockMarkdownCanvas*>(canvas)->DrawImage(
      src_.c_str(), x, y, x + width_, y + height_, nullptr);
  if (radius_ > 0) {
    canvas->Restore();
  }
}
void MockInlineView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawView(id_.c_str(), x, y,
                                                     x + width_, y + height_);
}

}  // namespace lynx::markdown::testing
