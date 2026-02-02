// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"
#include "markdown/platform/ios/serval_markdown_view.h"
@class MarkdownCustomViewImpl;

namespace lynx::markdown {
class MarkdownCustomViewIOS : public MarkdownPlatformViewIOS,
                              public MarkdownCustomViewHandle {
 public:
  MarkdownCustomViewIOS(MarkdownCustomViewImpl* view);
  ~MarkdownCustomViewIOS() override = default;
  void RequestMeasure() override;
  void RequestAlign() override;
  SizeF Measure(MeasureSpec spec) override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas) override;
  MarkdownCustomViewHandle* GetCustomViewHandle() override { return this; }
};
}  // namespace lynx::markdown

@interface MarkdownCustomViewImpl (CPP)
@property(nonatomic, assign)
    lynx::markdown::MarkdownCustomViewIOS* markdownViewHandle;
- (void)initHandle;
@end
#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
