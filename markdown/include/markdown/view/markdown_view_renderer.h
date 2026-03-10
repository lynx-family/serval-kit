// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#include <memory>

#include "markdown/element/markdown_document.h"
#include "markdown_view_measurer.h"
namespace lynx::markdown {
class MarkdownViewRenderer {
 public:
  MarkdownViewRenderer() = default;
  ~MarkdownViewRenderer() = default;

  void Draw(tttext::ICanvasHelper* canvas, float left, float top, float right,
            float bottom) const;
  void SetDocument(std::shared_ptr<MarkdownDocument> document);
  void SetMarkdownAnimationType(MarkdownAnimationType type) {
    animation_type_ = type;
  }
  void SetMarkdownAnimationStep(int32_t step) { animation_step_ = step; }
  void SetTypewriterCursor(MarkdownPlatformView* cursor) { cursor_ = cursor; }

 private:
  std::shared_ptr<MarkdownDocument> document_;
  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  int32_t animation_step_{0};
  MarkdownPlatformView* cursor_{nullptr};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
