// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#import <CoreFoundation/CoreFoundation.h>
#import "ServalMarkdown/serval_markdown_props.h"
typedef enum : NSUInteger {
  kServalMarkdownLayoutModeIndefinite,
  kServalMarkdownLayoutModeDefinite,
  kServalMarkdownLayoutModeAtMost,
} ServalMarkdownLayoutMode;
typedef enum : NSUInteger {
  kServalMarkdownAnimationTypeNone,
  kServalMarkdownAnimationTypeTypewriter,
} ServalMarkdownAnimationType;
@interface MarkdownCustomViewImpl : UIView
- (CGSize)measureByWidth:(CGFloat)width
               WidthMode:(ServalMarkdownLayoutMode)widthMode
                  Height:(CGFloat)height
              HeightMode:(ServalMarkdownLayoutMode)heightMode;
- (void)requestMeasure;
@end

@interface ServalMarkdownView : MarkdownCustomViewImpl
@property(nonatomic, strong) NSString* content;
@property(nonatomic, strong) NSDictionary* style;
@property(nonatomic, assign) ServalMarkdownAnimationType animationType;
@property(nonatomic, assign) float animationVelocity;
@property(nonatomic, assign) int initialAnimationStep;

- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value;
- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value;
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value;
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color;
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array;
- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict;
@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
