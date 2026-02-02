// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
#include <memory>
#include <vector>

#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx::markdown {
class MarkdownDrawable;
class MarkdownPath;
class MarkdownCanvasExtend {
 public:
  virtual ~MarkdownCanvasExtend() = default;
  virtual void ClipPath(MarkdownPath* path) = 0;
  virtual void DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                                  MarkdownPath* path,
                                  tttext::Painter* painter) = 0;
  virtual void DrawMarkdownPath(MarkdownPath* path,
                                tttext::Painter* painter) = 0;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
