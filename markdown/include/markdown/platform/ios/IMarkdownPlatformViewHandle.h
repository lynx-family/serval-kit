// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNPLATFORMVIEWHANDLE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNPLATFORMVIEWHANDLE_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>
typedef struct {
  float width;
  float height;
  float baseline;
} ServalMarkdownMeasureResult;
@protocol IMarkdownPlatformViewHandle <NSObject>
- (void)requestMeasure;
- (void)requestAlign;
- (void)requestDraw;

- (ServalMarkdownMeasureResult)
    measureByWidth:(CGFloat)width
         WidthMode:(ServalMarkdownLayoutMode)widthMode
            Height:(CGFloat)height
        HeightMode:(ServalMarkdownLayoutMode)heightMode;
- (void)align:(CGFloat)left top:(CGFloat)top;

- (CGSize)getSize;
- (CGPoint)getPosition;

- (void)setSize:(CGFloat)width height:(CGFloat)height;
- (void)setPosition:(CGFloat)left top:(CGFloat)top;
- (void)setVisibility:(BOOL)visible;

@optional
- (NSInteger)getVerticalAlign;
@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNPLATFORMVIEWHANDLE_H_
