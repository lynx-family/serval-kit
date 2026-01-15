// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/view/markdown_platform_view.h"
namespace lynx::markdown {
SizeF MarkdownViewDelegate::Measure(MeasureSpec spec) {
  if (max_width_ > 0) {
    spec.width_ = max_width_;
  }
  if (max_height_ > 0) {
    spec.height_ = max_height_;
  }
  view_->Measure(spec);
  size_ = view_->GetMeasuredSize();
  return size_;
}
float MarkdownViewDelegate::GetWidth() const {
  return size_.width_;
}
float MarkdownViewDelegate::GetHeight() const {
  return size_.height_;
}
float MarkdownViewDelegate::GetBaseLine() const {
  return (size_.height_ + 0.6f * font_size_) / 2;
}

void MarkdownViewDelegate::Draw(tttext::ICanvasHelper* canvas, float left,
                                float top, float right, float bottom) {
  view_->SetVisibility(true);
}

void MarkdownBlockViewDelegate::Draw(tttext::ICanvasHelper* canvas, float left,
                                     float top, float right, float bottom) {
  view_->SetVisibility(true);
}

}  // namespace lynx::markdown
