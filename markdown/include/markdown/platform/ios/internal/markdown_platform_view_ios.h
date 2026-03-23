// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
#import <Foundation/Foundation.h>

#import "markdown/platform/ios/IMarkdownPlatformViewHandle.h"
#include "markdown/view/markdown_platform_view.h"
namespace lynx::markdown {
class MarkdownPlatformViewIOS : public MarkdownPlatformView {
 public:
  explicit MarkdownPlatformViewIOS(id<IMarkdownPlatformViewHandle> handle);
  ~MarkdownPlatformViewIOS() override;
  void RequestMeasure() override;
  void RequestAlign() override;
  void RequestDraw() override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  bool HasGestureListener() const;
  bool DispatchTap(PointF position, GestureEventType event);
  bool DispatchLongPress(PointF position, GestureEventType event);
  bool DispatchPan(PointF position, PointF motion, GestureEventType event);
  PointF GetAlignedPosition() override;
  SizeF GetMeasuredSize() override;

  void SetMeasuredSize(SizeF size) override;
  void SetAlignPosition(PointF position) override;
  void SetVisibility(bool visible) override;

  id<IMarkdownPlatformViewHandle> GetHandle() { return handle_; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

  __weak id<IMarkdownPlatformViewHandle> handle_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_INTERNAL_MARKDOWN_PLATFORM_VIEW_IOS_H_
