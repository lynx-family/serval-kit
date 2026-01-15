// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"

namespace lynx::markdown {

MarkdownPlatformViewIOS::MarkdownPlatformViewIOS(UIView* view) : view_(view) {}

void MarkdownPlatformViewIOS::RequestDraw() {
  [view_ setNeedsDisplay];
}
PointF MarkdownPlatformViewIOS::GetAlignedPosition() {
  return {static_cast<float>(view_.frame.origin.x),
          static_cast<float>(view_.frame.origin.y)};
}
SizeF MarkdownPlatformViewIOS::GetMeasuredSize() {
  return {static_cast<float>(view_.frame.size.width),
          static_cast<float>(view_.frame.size.height)};
}

void MarkdownPlatformViewIOS::SetMeasuredSize(SizeF size) {
  CGRect new_frame = view_.frame;
  new_frame.size = CGSizeMake(size.width_, size.height_);
  view_.frame = new_frame;
}
void MarkdownPlatformViewIOS::SetAlignPosition(PointF position) {
  CGRect new_frame = view_.frame;
  new_frame.origin = CGPointMake(position.x_, position.y_);
  view_.frame = new_frame;
}
void MarkdownPlatformViewIOS::SetVisibility(bool visible) {
  view_.hidden = !visible;
}
}  // namespace lynx::markdown
