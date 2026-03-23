// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"

namespace serval::markdown {
class MarkdownCustomViewIOS : public MarkdownPlatformViewIOS,
                              public MarkdownCustomViewHandle {
 public:
  explicit MarkdownCustomViewIOS(id<IMarkdownPlatformViewHandle> view);
  ~MarkdownCustomViewIOS() override = default;
  void AttachDrawable(std::unique_ptr<MarkdownDrawable> drawable) override;
  void RequestMeasure() override;
  void RequestAlign() override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  MarkdownCustomViewHandle* GetCustomViewHandle() override { return this; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_CUSTOM_VIEW_IOS_H_
