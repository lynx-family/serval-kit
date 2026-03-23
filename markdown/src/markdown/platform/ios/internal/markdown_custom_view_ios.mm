// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"

#include "markdown/element/markdown_drawable.h"

#import <Foundation/Foundation.h>

#import "markdown/platform/ios/MarkdownCustomDrawView.h"

@interface MarkdownCustomDrawView (AttachDrawable)
- (void)attachDrawable:(lynx::markdown::MarkdownDrawable*)drawable;
@end

namespace lynx::markdown {
MarkdownCustomViewIOS::MarkdownCustomViewIOS(
    id<IMarkdownPlatformViewHandle> view)
    : MarkdownPlatformViewIOS(view) {}

void MarkdownCustomViewIOS::AttachDrawable(
    std::unique_ptr<MarkdownDrawable> drawable) {
  MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
  auto* view = static_cast<MarkdownCustomDrawView*>(handle_);
  if (view != nil) {
    [view attachDrawable:drawable_.get()];
  }
}
void MarkdownCustomViewIOS::RequestMeasure() {
  MarkdownPlatformViewIOS::RequestMeasure();
}
void MarkdownCustomViewIOS::RequestAlign() {
  MarkdownPlatformViewIOS::RequestAlign();
}
MeasureResult MarkdownCustomViewIOS::OnMeasure(MeasureSpec spec) {
  if (drawable_ == nullptr) {
    return {};
  }
  const auto result = drawable_->Measure(spec);
  SetMeasuredSize({.width_ = result.width_, .height_ = result.height_});
  return result;
}
void MarkdownCustomViewIOS::Align(float left, float top) {
  SetAlignPosition({left, top});
  if (drawable_ != nullptr) {
    drawable_->Align(left, top);
  }
}
void MarkdownCustomViewIOS::Draw(tttext::ICanvasHelper* canvas, float x,
                                 float y) {
  if (drawable_ == nullptr) {
    return;
  }
  drawable_->Draw(canvas, x, y);
}
}  // namespace lynx::markdown
