// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import "markdown/platform/ios/MarkdownPlatformView.h"

static const CGFloat kMarkdownMaxMeasureSize = 1e8;

@interface MarkdownPlatformView () {
  void* _nativePlatformView;
}

@end

@implementation MarkdownPlatformView

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    self.userInteractionEnabled = YES;
  }
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self != nil) {
    self.userInteractionEnabled = YES;
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  self = [super initWithCoder:coder];
  if (self != nil) {
    self.userInteractionEnabled = YES;
  }
  return self;
}

- (void)setNativePlatformView:(void*)platform_view {
  _nativePlatformView = platform_view;
}

- (void*)nativePlatformView {
  return _nativePlatformView;
}

- (void)requestMeasure {
  [self invalidateIntrinsicContentSize];
  [self setNeedsLayout];
  [self setNeedsDisplay];
}

- (void)requestAlign {
  [self setNeedsLayout];
}

- (void)requestDraw {
  [self setNeedsDisplay];
}

- (ServalMarkdownMeasureResult)
    measureByWidth:(CGFloat)width
         WidthMode:(ServalMarkdownLayoutMode)widthMode
            Height:(CGFloat)height
        HeightMode:(ServalMarkdownLayoutMode)heightMode {
  CGFloat maxWidth = kMarkdownMaxMeasureSize;
  CGFloat maxHeight = kMarkdownMaxMeasureSize;
  CGFloat targetWidth = width;
  CGFloat targetHeight = height;
  if (widthMode == kServalMarkdownLayoutModeIndefinite) {
    targetWidth = maxWidth;
  }
  if (heightMode == kServalMarkdownLayoutModeIndefinite) {
    targetHeight = maxHeight;
  }
  CGSize size = CGSizeMake(targetWidth, targetHeight);
  CGSize fit = [self sizeThatFits:size];
  if (widthMode == kServalMarkdownLayoutModeDefinite) {
    fit.width = width;
  } else if (widthMode == kServalMarkdownLayoutModeAtMost) {
    fit.width = MIN(fit.width, width);
  }
  if (heightMode == kServalMarkdownLayoutModeDefinite) {
    fit.height = height;
  } else if (heightMode == kServalMarkdownLayoutModeAtMost) {
    fit.height = MIN(fit.height, height);
  }
  return {static_cast<float>(fit.width), static_cast<float>(fit.height),
          static_cast<float>(fit.height)};
}

- (void)align:(CGFloat)left top:(CGFloat)top {
  CGRect newFrame = self.frame;
  newFrame.origin = CGPointMake(left, top);
  self.frame = newFrame;
}

- (CGSize)getSize {
  return self.frame.size;
}

- (CGPoint)getPosition {
  return self.frame.origin;
}

- (void)setSize:(CGFloat)width height:(CGFloat)height {
  CGRect newFrame = self.frame;
  newFrame.size = CGSizeMake(width, height);
  self.frame = newFrame;
}

- (void)setPosition:(CGFloat)left top:(CGFloat)top {
  CGRect newFrame = self.frame;
  newFrame.origin = CGPointMake(left, top);
  self.frame = newFrame;
}

- (void)setVisibility:(BOOL)visible {
  self.hidden = !visible;
}

- (ServalMarkdownVerticalAlign)getVerticalAlign {
  return kServalMarkdownVerticalAlignBaseline;
}

@end
