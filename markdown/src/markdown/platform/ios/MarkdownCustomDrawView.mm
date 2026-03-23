// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "markdown/platform/ios/MarkdownCustomDrawView.h"

#include "markdown/element/markdown_drawable.h"
#include "markdown/utils/markdown_textlayout_headers.h"

#include "markdown/platform/ios/internal/markdown_canvas_ios.h"

@interface MarkdownCustomDrawView () {
  serval::markdown::MarkdownDrawable* _drawable;
}
@end

const float kServalMarkdownMaxSize = 1e8;

@implementation MarkdownCustomDrawView

- (void)attachDrawable:(serval::markdown::MarkdownDrawable*)drawable {
  _drawable = drawable;
}

- (void)requestMeasure {
  [self invalidateIntrinsicContentSize];
  [self setNeedsLayout];
  [self setNeedsDisplay];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (_drawable != nullptr) {
    _drawable->Align(self.frame.origin.x, self.frame.origin.y);
  }
}

- (ServalMarkdownMeasureResult)
    measureByWidth:(CGFloat)width
         WidthMode:(ServalMarkdownLayoutMode)widthMode
            Height:(CGFloat)height
        HeightMode:(ServalMarkdownLayoutMode)heightMode {
  if (_drawable == nullptr) {
    return {0, 0, 0};
  }
  serval::markdown::MeasureSpec spec;
  spec.width_ = width;
  spec.height_ = height;
  spec.width_mode_ = static_cast<tttext::LayoutMode>(widthMode);
  spec.height_mode_ = static_cast<tttext::LayoutMode>(heightMode);
  const auto size = _drawable->Measure(spec);
  return {size.width_, size.height_, size.baseline_};
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
  const auto result = [self measureByWidth:width
                                 WidthMode:widthMode
                                    Height:height
                                HeightMode:heightMode];
  return CGSizeMake(result.width, result.height);
}

- (CGSize)intrinsicContentSize {
  CGSize size = self.bounds.size;
  BOOL hasWidth = size.width > 0 && size.width < CGFLOAT_MAX;
  ServalMarkdownLayoutMode widthMode = hasWidth
                                           ? kServalMarkdownLayoutModeDefinite
                                           : kServalMarkdownLayoutModeAtMost;
  CGFloat width = hasWidth ? size.width : kServalMarkdownMaxSize;
  const auto result = [self measureByWidth:width
                                 WidthMode:widthMode
                                    Height:kServalMarkdownMaxSize
                                HeightMode:kServalMarkdownLayoutModeAtMost];
  return CGSizeMake(result.width, result.height);
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
  const auto result = [self measureByWidth:width
                                 WidthMode:widthMode
                                    Height:height
                                HeightMode:heightMode];
  return CGSizeMake(result.width, result.height);
}

- (void)drawRect:(CGRect)rect {
  if (_drawable == nullptr) {
    return;
  }
  CGContextRef context = UIGraphicsGetCurrentContext();
  MarkdownCanvasIOS canvas(context);
  _drawable->Draw(&canvas, 0, 0);
}

@end
