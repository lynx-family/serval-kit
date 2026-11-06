// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"

#include "markdown/platform/ios/internal/markdown_value_convert.h"

@protocol MarkdownNativePlatformViewBridge <NSObject>
- (void)setNativePlatformView:(void*)platform_view;
- (void*)nativePlatformView;
@end

namespace serval::markdown {
MarkdownPlatformViewIOS::MarkdownPlatformViewIOS(
    id<IMarkdownPlatformViewHandle> handle)
    : handle_(handle) {
  id view_obj = handle_;
  if (view_obj != nil &&
      [view_obj respondsToSelector:@selector(setNativePlatformView:)]) {
    auto* view = (id<MarkdownNativePlatformViewBridge>)view_obj;
    [view setNativePlatformView:this];
  }
}

MarkdownPlatformViewIOS::~MarkdownPlatformViewIOS() {
  id view_obj = handle_;
  if (view_obj != nil &&
      [view_obj respondsToSelector:@selector(nativePlatformView)] &&
      [view_obj respondsToSelector:@selector(setNativePlatformView:)]) {
    auto* view = (id<MarkdownNativePlatformViewBridge>)view_obj;
    if ([view nativePlatformView] == this) {
      [view setNativePlatformView:nullptr];
    }
  }
}

void MarkdownPlatformViewIOS::RequestDraw() {
  if (handle_ != nil) {
    [handle_ requestDraw];
  }
}

void MarkdownPlatformViewIOS::Align(float left, float top) {
  if (handle_ != nil) {
    [handle_ align:static_cast<CGFloat>(left) top:static_cast<CGFloat>(top)];
  }
}

void MarkdownPlatformViewIOS::Draw(tttext::ICanvasHelper* canvas, float x,
                                   float y) {}

MeasureResult MarkdownPlatformViewIOS::OnMeasure(MeasureSpec spec) {
  if (handle_ == nil) {
    return {};
  }
  const auto result = [handle_
      measureByWidth:static_cast<CGFloat>(spec.width_)
           WidthMode:static_cast<ServalMarkdownLayoutMode>(spec.width_mode_)
              Height:static_cast<CGFloat>(spec.height_)
          HeightMode:static_cast<ServalMarkdownLayoutMode>(spec.height_mode_)];
  MeasureResult measure_result{
      .width_ = result.width,
      .height_ = result.height,
      .baseline_ = result.baseline,
  };
  return measure_result;
}

void MarkdownPlatformViewIOS::SetVisibility(bool visible) {
  if (handle_ != nil) {
    [handle_ setVisibility:visible];
    return;
  }
}

void MarkdownPlatformViewIOS::OnSetBounds(RectF bounds) {
  [handle_ setRenderBounds:CGRectMake(bounds.GetLeft(), bounds.GetTop(),
                                      bounds.GetWidth(), bounds.GetHeight())];
}

MarkdownVerticalAlign MarkdownPlatformViewIOS::GetVerticalAlign() const {
  return [handle_ respondsToSelector:@selector(getVerticalAlign)]
             ? MarkdownValueConvert::ConvertVerticalAlign(
                   [handle_ getVerticalAlign])
             : MarkdownVerticalAlign::kBaseline;
}

float MarkdownPlatformViewIOS::GetVerticalAlignLength() const {
  return [handle_ respondsToSelector:@selector(getVerticalAlignLength)]
             ? [handle_ getVerticalAlignLength]
             : 0;
}
}  // namespace serval::markdown
