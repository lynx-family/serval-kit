// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
#import <CoreFoundation/CoreFoundation.h>

#include "markdown/view/markdown_platform_view.h"
namespace lynx::markdown {
class MarkdownPlatformViewIOS : public MarkdownPlatformView {
 public:
  MarkdownPlatformViewIOS(UIView* view);
  ~MarkdownPlatformViewIOS() override = default;
  void RequestDraw() override;
  PointF GetAlignedPosition() override;
  SizeF GetMeasuredSize() override;

  void SetMeasuredSize(SizeF size) override;
  void SetAlignPosition(PointF position) override;
  void SetVisibility(bool visible) override;

  UIView* GetUIView() { return view_; }

 protected:
  __weak UIView* view_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
