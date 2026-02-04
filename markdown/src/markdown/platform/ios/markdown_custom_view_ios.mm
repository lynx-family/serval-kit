// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_canvas_ios.h"

@interface MarkdownCustomViewImpl () {
  lynx::markdown::MarkdownCustomViewIOS* _markdownViewHandle;
}

@end
const float kServalMarkdownMaxSize = 1e8;
@implementation MarkdownCustomViewImpl
- (instancetype)init {
  self = [super init];
  if (self != nil) {
    [self initHandle];
  }
  return self;
}
- (void)initHandle {
  _markdownViewHandle = new lynx::markdown::MarkdownCustomViewIOS(self);
}

- (void)setMarkdownViewHandle:
    (lynx::markdown::MarkdownCustomViewIOS*)markdownViewHandle {
  _markdownViewHandle = markdownViewHandle;
}

- (lynx::markdown::MarkdownCustomViewIOS*)markdownViewHandle {
  return _markdownViewHandle;
}

- (void)dealloc {
  if (_markdownViewHandle != nullptr) {
    delete _markdownViewHandle;
  }
}

- (void)requestMeasure {
  [self invalidateIntrinsicContentSize];
  [self setNeedsLayout];
  [self setNeedsDisplay];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.markdownViewHandle->Align(self.frame.origin.x, self.frame.origin.y);
}

- (CGSize)measureByWidth:(CGFloat)width
               WidthMode:(ServalMarkdownLayoutMode)widthMode
                  Height:(CGFloat)height
              HeightMode:(ServalMarkdownLayoutMode)heightMode {
  lynx::markdown::MeasureSpec spec;
  spec.width_ = width;
  spec.height_ = height;
  spec.width_mode_ = static_cast<tttext::LayoutMode>(widthMode);
  spec.height_mode_ = static_cast<tttext::LayoutMode>(heightMode);
  auto size = self.markdownViewHandle->Measure(spec);
  return CGSizeMake(size.width_, size.height_);
}

- (CGSize)sizeThatFits:(CGSize)size {
  BOOL hasWidth = size.width > 0 && size.width < CGFLOAT_MAX;
  BOOL hasHeight = size.height > 0 && size.height < CGFLOAT_MAX;
  ServalMarkdownLayoutMode widthMode =
      hasWidth ? kServalMarkdownLayoutModeAtMost
               : kServalMarkdownLayoutModeIndefinite;
  ServalMarkdownLayoutMode heightMode =
      hasHeight ? kServalMarkdownLayoutModeAtMost
                : kServalMarkdownLayoutModeIndefinite;
  CGFloat width = hasWidth ? size.width : kServalMarkdownMaxSize;
  CGFloat height = hasHeight ? size.height : kServalMarkdownMaxSize;
  return [self measureByWidth:width
                    WidthMode:widthMode
                       Height:height
                   HeightMode:heightMode];
}

- (CGSize)intrinsicContentSize {
  CGSize size = self.bounds.size;
  BOOL hasWidth = size.width > 0 && size.width < CGFLOAT_MAX;
  ServalMarkdownLayoutMode widthMode = hasWidth
                                           ? kServalMarkdownLayoutModeDefinite
                                           : kServalMarkdownLayoutModeAtMost;
  CGFloat width = hasWidth ? size.width : kServalMarkdownMaxSize;
  return [self measureByWidth:width
                    WidthMode:widthMode
                       Height:kServalMarkdownMaxSize
                   HeightMode:kServalMarkdownLayoutModeAtMost];
}

- (CGSize)
      systemLayoutSizeFittingSize:(CGSize)targetSize
    withHorizontalFittingPriority:(UILayoutPriority)horizontalFittingPriority
          verticalFittingPriority:(UILayoutPriority)verticalFittingPriority {
  BOOL hasWidth = targetSize.width > 0 && targetSize.width < CGFLOAT_MAX;
  BOOL hasHeight = targetSize.height > 0 && targetSize.height < CGFLOAT_MAX;
  ServalMarkdownLayoutMode widthMode =
      (horizontalFittingPriority >= UILayoutPriorityRequired && hasWidth)
          ? kServalMarkdownLayoutModeDefinite
          : (hasWidth ? kServalMarkdownLayoutModeAtMost
                      : kServalMarkdownLayoutModeIndefinite);
  ServalMarkdownLayoutMode heightMode =
      (verticalFittingPriority >= UILayoutPriorityRequired && hasHeight)
          ? kServalMarkdownLayoutModeDefinite
          : (hasHeight ? kServalMarkdownLayoutModeAtMost
                       : kServalMarkdownLayoutModeIndefinite);
  CGFloat width = hasWidth ? targetSize.width : kServalMarkdownMaxSize;
  CGFloat height = hasHeight ? targetSize.height : kServalMarkdownMaxSize;
  return [self measureByWidth:width
                    WidthMode:widthMode
                       Height:height
                   HeightMode:heightMode];
}

- (void)drawRect:(CGRect)rect {
  CGContextRef context = UIGraphicsGetCurrentContext();
  MarkdownCanvasIOS canvas(context);
  self.markdownViewHandle->Draw(&canvas);
}
@end

namespace lynx::markdown {
MarkdownCustomViewIOS::MarkdownCustomViewIOS(MarkdownCustomViewImpl* view)
    : MarkdownPlatformViewIOS(view) {}
void MarkdownCustomViewIOS::RequestMeasure() {
  [((MarkdownCustomViewImpl*)view_) requestMeasure];
}
void MarkdownCustomViewIOS::RequestAlign() {
  [((MarkdownCustomViewImpl*)view_) setNeedsLayout];
}
SizeF MarkdownCustomViewIOS::Measure(MeasureSpec spec) {
  auto size = drawable_->Measure(spec);
  SetMeasuredSize(size);
  return size;
}
void MarkdownCustomViewIOS::Align(float left, float top) {
  SetAlignPosition({left, top});
  drawable_->Align(left, top);
}
void MarkdownCustomViewIOS::Draw(tttext::ICanvasHelper* cavas) {
  drawable_->Draw(cavas, 0, 0);
}
}  // namespace lynx::markdown
