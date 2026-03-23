// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import "markdown/platform/ios/MarkdownPlatformView.h"

#include "markdown/platform/ios/internal/markdown_platform_view_ios.h"

static const CGFloat kMarkdownMaxMeasureSize = 1e8;

namespace {
lynx::markdown::PointF ConvertPoint(CGPoint point) {
  return {static_cast<float>(point.x), static_cast<float>(point.y)};
}
}  // namespace

@interface MarkdownPlatformView () {
  void* _nativePlatformView;
  BOOL _didSetupGestures;
}

- (void)setupGestureRecognizers;
- (BOOL)dispatchTapAtPoint:(CGPoint)point
                     event:(lynx::markdown::GestureEventType)event;
- (BOOL)dispatchLongPressAtPoint:(CGPoint)point
                           event:(lynx::markdown::GestureEventType)event;
- (BOOL)dispatchPanAtPoint:(CGPoint)point
                    motion:(CGPoint)motion
                     event:(lynx::markdown::GestureEventType)event;
- (void)onTapGesture:(UITapGestureRecognizer*)recognizer;
- (void)onLongPressGesture:(UILongPressGestureRecognizer*)recognizer;
- (void)onPanGesture:(UIPanGestureRecognizer*)recognizer;
@end

@implementation MarkdownPlatformView

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    [self setupGestureRecognizers];
  }
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self != nil) {
    [self setupGestureRecognizers];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  self = [super initWithCoder:coder];
  if (self != nil) {
    [self setupGestureRecognizers];
  }
  return self;
}

- (void)setupGestureRecognizers {
  if (_didSetupGestures) {
    return;
  }
  _didSetupGestures = YES;
  self.userInteractionEnabled = YES;
  UITapGestureRecognizer* tap =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(onTapGesture:)];
  tap.cancelsTouchesInView = NO;
  [self addGestureRecognizer:tap];
  UILongPressGestureRecognizer* long_press =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(onLongPressGesture:)];
  long_press.cancelsTouchesInView = NO;
  [self addGestureRecognizer:long_press];
  UIPanGestureRecognizer* pan =
      [[UIPanGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(onPanGesture:)];
  pan.cancelsTouchesInView = NO;
  [self addGestureRecognizer:pan];
}

- (void)setNativePlatformView:(void*)platform_view {
  _nativePlatformView = platform_view;
}

- (void*)nativePlatformView {
  return _nativePlatformView;
}

- (BOOL)dispatchTapAtPoint:(CGPoint)point
                     event:(lynx::markdown::GestureEventType)event {
  auto* view = reinterpret_cast<lynx::markdown::MarkdownPlatformViewIOS*>(
      _nativePlatformView);
  if (view == nullptr) {
    return NO;
  }
  return view->DispatchTap(ConvertPoint(point), event);
}

- (BOOL)dispatchLongPressAtPoint:(CGPoint)point
                           event:(lynx::markdown::GestureEventType)event {
  auto* view = reinterpret_cast<lynx::markdown::MarkdownPlatformViewIOS*>(
      _nativePlatformView);
  if (view == nullptr) {
    return NO;
  }
  return view->DispatchLongPress(ConvertPoint(point), event);
}

- (BOOL)dispatchPanAtPoint:(CGPoint)point
                    motion:(CGPoint)motion
                     event:(lynx::markdown::GestureEventType)event {
  auto* view = reinterpret_cast<lynx::markdown::MarkdownPlatformViewIOS*>(
      _nativePlatformView);
  if (view == nullptr) {
    return NO;
  }
  return view->DispatchPan(ConvertPoint(point), ConvertPoint(motion), event);
}

- (void)onTapGesture:(UITapGestureRecognizer*)recognizer {
  if (recognizer.state != UIGestureRecognizerStateEnded) {
    return;
  }
  const CGPoint point = [recognizer locationInView:self];
  [self dispatchTapAtPoint:point event:lynx::markdown::GestureEventType::kDown];
}

- (void)onLongPressGesture:(UILongPressGestureRecognizer*)recognizer {
  if (recognizer.state != UIGestureRecognizerStateBegan) {
    return;
  }
  const CGPoint point = [recognizer locationInView:self];
  [self dispatchLongPressAtPoint:point
                           event:lynx::markdown::GestureEventType::kDown];
}

- (void)onPanGesture:(UIPanGestureRecognizer*)recognizer {
  const CGPoint point = [recognizer locationInView:self];
  const CGPoint motion = [recognizer translationInView:self];
  switch (recognizer.state) {
    case UIGestureRecognizerStateBegan:
      [self dispatchPanAtPoint:point
                        motion:CGPointZero
                         event:lynx::markdown::GestureEventType::kDown];
      break;
    case UIGestureRecognizerStateChanged:
      [self dispatchPanAtPoint:point
                        motion:motion
                         event:lynx::markdown::GestureEventType::kMove];
      break;
    case UIGestureRecognizerStateEnded:
      [self dispatchPanAtPoint:point
                        motion:motion
                         event:lynx::markdown::GestureEventType::kUp];
      break;
    case UIGestureRecognizerStateCancelled:
    case UIGestureRecognizerStateFailed:
      [self dispatchPanAtPoint:point
                        motion:motion
                         event:lynx::markdown::GestureEventType::kCancel];
      break;
    default:
      break;
  }
}

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  UIView* target = [super hitTest:point withEvent:event];
  if (target != self) {
    return target;
  }
  auto* view = reinterpret_cast<lynx::markdown::MarkdownPlatformViewIOS*>(
      _nativePlatformView);
  if (view == nullptr || !view->HasGestureListener()) {
    return nil;
  }
  return target;
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

- (NSInteger)getVerticalAlign {
  return 0;
}

@end
