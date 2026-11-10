// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion.
// See sheet/LICENSE.screens for the MIT license covering those portions.

#import "SRSheetContentUI.h"
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxEventHandler.h>
#import "SRSheetContentShadowNode.h"

#import <Lynx/LynxBackgroundManager.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>

@interface SRSheetAnchorView : UIView
@property(nonatomic, copy) void (^onWindowChange)(void);
@end
@implementation SRSheetAnchorView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return nil;
}
- (void)didMoveToWindow {
  [super didMoveToWindow];
  if (self.onWindowChange) self.onWindowChange();
}
@end

@interface SRSheetContentView ()
@property(nonatomic, weak) LynxEventHandler *eventHandler;
@end

@interface SRSheetContentUI ()

- (void)syncHostConstraintsFromReceiver:(SRSheetContentView *)receiver;

@end

@implementation SRSheetContentView

- (instancetype)initWithTransfer:(SRSheetContentUI *)transfer {
  if (self = [super initWithFrame:CGRectZero]) {
    _transfer = transfer;
    self.translatesAutoresizingMaskIntoConstraints = YES;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return self;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  id<LynxEventTarget> target = [self.eventHandler hitTest:point withEvent:event];
  UIView *view = [super hitTest:point withEvent:event];
  [self.eventHandler handleFocus:target
                          onView:view
                   withContainer:self
                        andPoint:point
                        andEvent:event];
  return view;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self.transfer syncHostConstraintsFromReceiver:self];
}

@end

@implementation SRSheetContentUI {
  SRSheetContentView *_transferReceiver;
  LynxEventHandler *_contentEventHandler;
  NSInteger _arenaIndex;
}

- (instancetype)init {
  if ((self = [super init])) _arenaIndex = -1;
  return self;
}

- (UIView *)createView {
  SRSheetAnchorView *view = [SRSheetAnchorView new];
  __weak __typeof__(self) weakSelf = self;
  view.onWindowChange = ^{
    if (weakSelf.onHostChanged) weakSelf.onHostChanged();
  };
  return view;
}

- (void)setContext:(LynxUIContext *)context {
  [super setContext:context];
  if (!_contentEventHandler && context.eventHandler) {
    _contentEventHandler = [[LynxEventHandler alloc] initWithRootView:self.transferReceiver
                                                           withRootUI:self];
    self.transferReceiver.eventHandler = _contentEventHandler;
    [_contentEventHandler updateUiOwner:context.uiOwner eventEmitter:context.eventEmitter];
    _arenaIndex = [_contentEventHandler
        setGestureArenaManagerAndGetIndex:context.eventHandler.gestureArenaManager];
  }
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(UIEvent *)event {
  // Page hit testing skips the anchor; the receiver's handler enters hitTest directly.
  return NO;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  for (LynxUI *child in self.children.reverseObjectEnumerator) {
    CGPoint converted = [self.transferReceiver.layer convertPoint:point toLayer:child.view.layer];
    if ([child shouldHitTest:point withEvent:event] && [child containsPoint:converted])
      return [child hitTest:converted withEvent:event];
  }
  return self;
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
  if (self.onContentLayout) self.onContentLayout();
}

- (CGFloat)naturalContentHeight {
  CGFloat height = 0;
  for (UIView *child in self.transferReceiver.subviews)
    height = MAX(height, CGRectGetMaxY(child.frame));
  return height;
}

- (void)dealloc {
  if (_arenaIndex >= 0) [_contentEventHandler removeGestureArenaManager:_arenaIndex];
  [_contentEventHandler removeEventGestures];
}

- (SRSheetContentView *)transferReceiver {
  if (!_transferReceiver) {
    _transferReceiver = [self createTransferReceiver];
  }
  return _transferReceiver;
}

- (SRSheetContentView *)createTransferReceiver {
  return [[SRSheetContentView alloc] initWithTransfer:self];
}

- (UIView *)childrenContainerView {
  return self.transferReceiver;
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  UIView *containerView = self.transferReceiver;
  CALayer *mainLayer = child.view.layer;
  CALayer *containerLayer = containerView.layer;
  LynxBackgroundManager *backgroundManager = child.backgroundManager;

  // Lynx versions before childrenContainerView support always insert into self.view.
  [self didInsertChild:child atIndex:index];
  [containerView insertSubview:child.view atIndex:index];

  if (index > 0) {
    LynxUI *previousSibling = self.children[index - 1];
    if ([containerLayer.sublayers indexOfObject:previousSibling.topLayer] >
        [containerLayer.sublayers indexOfObject:mainLayer]) {
      [child.view removeFromSuperview];
      [containerView insertSubview:child.view aboveSubview:previousSibling.view];
      [mainLayer removeFromSuperlayer];
      [containerLayer insertSublayer:mainLayer above:previousSibling.topLayer];
    }
  }

  if ((NSUInteger)index < self.children.count - 1) {
    LynxUI *nextSibling = self.children[index + 1];
    if ([containerLayer.sublayers indexOfObject:nextSibling.bottomLayer] <
        [containerLayer.sublayers indexOfObject:mainLayer]) {
      [child.view removeFromSuperview];
      [containerView insertSubview:child.view belowSubview:nextSibling.view];
      [mainLayer removeFromSuperlayer];
      [containerLayer insertSublayer:mainLayer below:nextSibling.bottomLayer];
    }
  }

  if (backgroundManager.borderLayer) {
    [backgroundManager.borderLayer removeFromSuperlayer];
    if (child.overflow == OVERFLOW_HIDDEN_VAL) {
      [containerLayer insertSublayer:backgroundManager.borderLayer above:mainLayer];
    } else {
      [containerLayer insertSublayer:backgroundManager.borderLayer below:mainLayer];
    }
  }

  if (backgroundManager.backgroundLayer) {
    [backgroundManager.backgroundLayer removeFromSuperlayer];
    if (child.overflow != OVERFLOW_HIDDEN_VAL && backgroundManager.borderLayer) {
      [containerLayer insertSublayer:backgroundManager.backgroundLayer
                               below:backgroundManager.borderLayer];
    } else {
      [containerLayer insertSublayer:backgroundManager.backgroundLayer below:mainLayer];
    }
  }
}

- (BOOL)shouldMeasureContentHeightToFit {
  return self.fitContent;
}

- (void)syncHostConstraintsFromReceiver:(SRSheetContentView *)receiver {
  BOOL hasExternalHost = receiver.superview && receiver.superview != self.view;
  CGFloat width = CGRectGetWidth(receiver.bounds);
  BOOL measureContentHeightToFit = [self shouldMeasureContentHeightToFit];
  // Natural height must not depend on the previous panel height.
  CGFloat height = measureContentHeightToFit ? 0 : CGRectGetHeight(receiver.bounds);
  LynxMeasureMode widthMode = hasExternalHost ? LynxMeasureModeDefinite : LynxMeasureModeIndefinite;
  LynxMeasureMode heightMode =
      hasExternalHost ? LynxMeasureModeDefinite : LynxMeasureModeIndefinite;
  [self.context
      findShadowNodeAndRunTask:self.sign
                          task:^(LynxShadowNode *node) {
                            if ([node isKindOfClass:SRSheetContentShadowNode.class]) {
                              [(SRSheetContentShadowNode *)node
                                  updateHostConstraintsWithWidth:width
                                                       widthMode:widthMode
                                                          height:height
                                                      heightMode:heightMode
                                       measureContentHeightToFit:measureContentHeightToFit];
                            }
                          }];
}

@end
