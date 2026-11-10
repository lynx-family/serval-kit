// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion.
// See sheet/LICENSE.screens for the MIT license covering those portions.

#import "SRSheetContentShadowNode.h"

#import <Lynx/LynxCustomMeasureDelegate.h>
#import <Lynx/LynxNativeLayoutNode.h>

@implementation SRSheetContentShadowNode {
  BOOL _hasHostConstraints;
  BOOL _measureContentHeightToFit;
  CGFloat _hostWidth;
  LynxMeasureMode _hostWidthMode;
  CGFloat _hostHeight;
  LynxMeasureMode _hostHeightMode;
}

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  if (self = [super initWithSign:sign tagName:tagName]) {
    self.hasCustomLayout = YES;
    _hostWidthMode = LynxMeasureModeIndefinite;
    _hostHeightMode = LynxMeasureModeIndefinite;
  }
  return self;
}

- (void)updateHostConstraintsWithWidth:(CGFloat)width
                             widthMode:(LynxMeasureMode)widthMode
                                height:(CGFloat)height
                            heightMode:(LynxMeasureMode)heightMode
             measureContentHeightToFit:(BOOL)measureContentHeightToFit {
  if (_hasHostConstraints && fabs(_hostWidth - width) <= CGFLOAT_EPSILON &&
      _hostWidthMode == widthMode && fabs(_hostHeight - height) <= CGFLOAT_EPSILON &&
      _hostHeightMode == heightMode && _measureContentHeightToFit == measureContentHeightToFit) {
    return;
  }

  _hasHostConstraints = YES;
  _hostWidth = width;
  _hostWidthMode = widthMode;
  _hostHeight = height;
  _hostHeightMode = heightMode;
  _measureContentHeightToFit = measureContentHeightToFit;
  [self setNeedsLayout];
}

- (MeasureResult)customMeasureLayoutNode:(MeasureParam *)param
                          measureContext:(MeasureContext *)context {
  if (!_hasHostConstraints) {
    return (MeasureResult){CGSizeZero, 0.f};
  }

  // Fit content must grow beyond an initially empty panel.
  CGFloat childHeight =
      _measureContentHeightToFit
          ? 0
          : (_hostHeightMode == LynxMeasureModeIndefinite ? param.height : _hostHeight);
  LynxMeasureMode childHeightMode =
      _measureContentHeightToFit
          ? LynxMeasureModeIndefinite
          : (_hostHeightMode == LynxMeasureModeIndefinite ? param.heightMode : _hostHeightMode);
  MeasureParam *childParam = [[MeasureParam alloc]
      initWithWidth:_hostWidthMode == LynxMeasureModeIndefinite ? param.width : _hostWidth
          WidthMode:_hostWidthMode == LynxMeasureModeIndefinite ? param.widthMode : _hostWidthMode
             Height:childHeight
         HeightMode:childHeightMode];
  for (LynxShadowNode *child in self.children) {
    if ([child isKindOfClass:LynxNativeLayoutNode.class]) {
      [(LynxNativeLayoutNode *)child measureWithMeasureParam:childParam MeasureContext:context];
    } else if ([child isKindOfClass:LynxCustomMeasureShadowNode.class]) {
      [(id<LynxCustomMeasureDelegate>)child measureWithMeasureParam:childParam
                                                     MeasureContext:context];
    }
  }
  return (MeasureResult){CGSizeZero, 0.f};
}

- (void)customAlignLayoutNode:(AlignParam *)param alignContext:(AlignContext *)context {
  AlignParam *childParam = [AlignParam new];
  [childParam SetAlignOffsetWithLeft:0.f Top:0.f];
  for (LynxShadowNode *child in self.children) {
    if ([child isKindOfClass:LynxNativeLayoutNode.class]) {
      [(LynxNativeLayoutNode *)child alignWithAlignParam:childParam AlignContext:context];
    } else if ([child isKindOfClass:LynxCustomMeasureShadowNode.class]) {
      [(id<LynxCustomMeasureDelegate>)child alignWithAlignParam:childParam AlignContext:context];
    }
  }
}

@end
