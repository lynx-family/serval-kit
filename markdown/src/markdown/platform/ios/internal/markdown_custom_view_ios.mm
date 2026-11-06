// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"

#include "markdown/element/markdown_drawable.h"

#import <Foundation/Foundation.h>

#import "markdown/platform/ios/MarkdownCustomDrawView.h"

@interface MarkdownCustomDrawView (AttachDrawable)
- (void)attachDrawable:(serval::markdown::MarkdownDrawable*)drawable;
@end

namespace serval::markdown {
MarkdownCustomViewIOS::MarkdownCustomViewIOS(
    id<IMarkdownPlatformViewHandle> view)
    : MarkdownPlatformViewIOS(view) {}
MarkdownCustomViewIOS::~MarkdownCustomViewIOS() {
  auto* view = (MarkdownCustomDrawView*)handle_;
  if (drawable_ != nullptr && view != nil) {
    [view attachDrawable:nullptr];
  }
}
void MarkdownCustomViewIOS::AttachDrawable(
    std::shared_ptr<MarkdownDrawable> drawable) {
  MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
  auto* view = (MarkdownCustomDrawView*)handle_;
  if (view != nil) {
    [view attachDrawable:drawable_.get()];
  }
}
MeasureResult MarkdownCustomViewIOS::OnMeasure(MeasureSpec spec) {
  if (drawable_ == nullptr) {
    return {};
  }
  return drawable_->Measure(spec);
}
void MarkdownCustomViewIOS::Align(float left, float top) {
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
}  // namespace serval::markdown
