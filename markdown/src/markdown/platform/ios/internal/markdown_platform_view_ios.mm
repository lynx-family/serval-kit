// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"

@protocol MarkdownNativePlatformViewBridge <NSObject>
- (void)setNativePlatformView:(void*)platform_view;
- (void*)nativePlatformView;
@end

namespace lynx::markdown {

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

void MarkdownPlatformViewIOS::RequestMeasure() {
  if (handle_ != nil) {
    [handle_ requestMeasure];
  }
}

void MarkdownPlatformViewIOS::RequestAlign() {
  if (handle_ != nil) {
    [handle_ requestAlign];
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

bool MarkdownPlatformViewIOS::HasGestureListener() const {
  return static_cast<bool>(tap_gesture_listener_) ||
         static_cast<bool>(long_press_gesture_listener_) ||
         static_cast<bool>(pan_gesture_listener_);
}

bool MarkdownPlatformViewIOS::DispatchTap(PointF position,
                                          GestureEventType event) {
  if (!tap_gesture_listener_) {
    return false;
  }
  return tap_gesture_listener_(position, event);
}

bool MarkdownPlatformViewIOS::DispatchLongPress(PointF position,
                                                GestureEventType event) {
  if (!long_press_gesture_listener_) {
    return false;
  }
  return long_press_gesture_listener_(position, event);
}

bool MarkdownPlatformViewIOS::DispatchPan(PointF position, PointF motion,
                                          GestureEventType event) {
  if (!pan_gesture_listener_) {
    return false;
  }
  return pan_gesture_listener_(position, motion, event);
}

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
PointF MarkdownPlatformViewIOS::GetAlignedPosition() {
  if (handle_ != nil) {
    const auto point = [handle_ getPosition];
    return {static_cast<float>(point.x), static_cast<float>(point.y)};
  }
  return {0, 0};
}
SizeF MarkdownPlatformViewIOS::GetMeasuredSize() {
  if (handle_ != nil) {
    const auto size = [handle_ getSize];
    return {static_cast<float>(size.width), static_cast<float>(size.height)};
  }
  return {0, 0};
}

void MarkdownPlatformViewIOS::SetMeasuredSize(SizeF size) {
  if (handle_ != nil) {
    [handle_ setSize:static_cast<CGFloat>(size.width_)
              height:static_cast<CGFloat>(size.height_)];
    return;
  }
}
void MarkdownPlatformViewIOS::SetAlignPosition(PointF position) {
  if (handle_ != nil) {
    [handle_ setPosition:static_cast<CGFloat>(position.x_)
                     top:static_cast<CGFloat>(position.y_)];
    return;
  }
}
void MarkdownPlatformViewIOS::SetVisibility(bool visible) {
  if (handle_ != nil) {
    [handle_ setVisibility:visible];
    return;
  }
}
}  // namespace lynx::markdown
