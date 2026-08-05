// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_

#import <UIKit/UIKit.h>
#include <stdint.h>

#import <ServalMarkdown/IMarkdownEventDelegate.h>
#import <ServalMarkdown/IMarkdownExposureDelegate.h>
#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <ServalMarkdown/IMarkdownResourceDelegate.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

typedef void (^MarkdownRequestMeasureCallback)(void);

@interface MarkdownMeasureResult : NSObject

@property(nonatomic, assign, readonly) CGFloat width;
@property(nonatomic, assign, readonly) CGFloat height;
@property(nonatomic, assign, readonly) NSInteger lineCount;
@property(nonatomic, copy, readonly) NSArray<NSString*>* lines;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

@interface MarkdownMeasurer : NSObject

@property(nonatomic, strong) NSString* content;
@property(nonatomic, strong) NSDictionary* style;
@property(nonatomic, assign) ServalMarkdownAnimationType animationType;
@property(nonatomic, assign) float animationVelocity;
@property(nonatomic, assign) int initialAnimationStep;
@property(nonatomic, weak) id<IMarkdownResourceDelegate> resourceDelegate;
@property(nonatomic, weak) id<IMarkdownEventDelegate> eventDelegate;
@property(nonatomic, weak) id<IMarkdownExposureDelegate> exposureDelegate;
@property(nonatomic, copy)
    MarkdownRequestMeasureCallback requestMeasureCallback;

- (ServalMarkdownMeasureResult)
    measureWithWidth:(CGFloat)width
           widthMode:(ServalMarkdownLayoutMode)widthMode
              height:(CGFloat)height
          heightMode:(ServalMarkdownLayoutMode)heightMode;

- (void)align:(CGFloat)left top:(CGFloat)top;
- (void)markDirty;
- (void)setTextSelection:(int)start end:(int)end;
- (int)getAnimationStep;
- (void)setAnimationStep:(int)animationStep;
- (void)pauseAnimation;
- (void)resumeAnimation;
- (void)resumeAnimation:(int)animationStep;
- (void)onLayoutFrame:(int64_t)frameTimeNanos;
- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value;
- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value;
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value;
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color;
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array;
- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict;
- (void)onImageLoaded:(NSString*)url;
- (void)onFontLoaded:(NSString*)family Weight:(int)weight Style:(int)style;

/**
 * Measures Markdown without creating a view.
 *
 * @param markdown Markdown source text; nil is treated as empty content.
 * @param style The same style dictionary accepted by ServalMarkdownView; nil
 *     uses the default style.
 * @param maxWidth Maximum layout width in points. Must be finite and
 *     non-negative.
 * @param maxLines Maximum number of lines; values less than or equal to zero
 *     mean unlimited.
 */
+ (MarkdownMeasureResult*)measure:(NSString*)markdown
                            style:(NSDictionary*)style
                         maxWidth:(CGFloat)maxWidth
                         maxLines:(NSInteger)maxLines;

@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_
