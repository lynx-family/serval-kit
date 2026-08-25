// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#import <UIKit/UIKit.h>
#include <stdint.h>

#import <ServalMarkdown/IMarkdownEventDelegate.h>
#import <ServalMarkdown/IMarkdownExposureDelegate.h>
#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <ServalMarkdown/IMarkdownResourceDelegate.h>
#import <ServalMarkdown/MarkdownCustomDrawView.h>
#import <ServalMarkdown/MarkdownMeasurer.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

NS_ASSUME_NONNULL_BEGIN

@interface ServalMarkdownView : MarkdownCustomDrawView
@property(nonatomic, strong, readonly, nullable)
    MarkdownMeasurer* markdownMeasurer;
@property(nonatomic, strong, nullable) NSString* content;
@property(nonatomic, strong, nullable) NSDictionary* style;
@property(nonatomic, assign) ServalMarkdownAnimationType animationType;
@property(nonatomic, assign) float animationVelocity;
@property(nonatomic, assign) int initialAnimationStep;
@property(nonatomic, weak, nullable) id<IMarkdownResourceDelegate>
    resourceDelegate;
@property(nonatomic, weak, nullable) id<IMarkdownEventDelegate> eventDelegate;
@property(nonatomic, weak, nullable) id<IMarkdownExposureDelegate>
    exposureDelegate;

- (instancetype)initWithCreateMeasurer:(BOOL)createMeasurer;
- (BOOL)attachMarkdownMeasurer:(nullable MarkdownMeasurer*)measurer;

- (NSString*)getContent:(int)start
                    end:(int)end
              indexType:(ServalMarkdownIndexType)indexType;
- (void)requestMeasure;
- (void)markDirty;
- (NSString*)getContent;
- (NSString*)getContentID;
- (NSString*)getSelectedText;
- (NSArray<NSString*>*)getAllImageUrl;
- (NSArray<NSString*>*)getLinkUrl;
- (NSArray<NSString*>*)getLinkContent;
- (NSArray<NSValue*>*)getLinkBoundingRect;
- (NSArray<NSValue*>*)getSyntaxSourceRanges:(nullable NSString*)tag;
- (NSRange)getSelectedRange;
- (NSArray<NSValue*>*)getSelectedLineBoundingRect;
- (CGPoint)getSelectionHandlePosition;
- (float)getSelectionHandleRadius;
- (NSArray<NSValue*>*)getTextBoundingRect:(int)start
                                      end:(int)end
                                indexType:(ServalMarkdownIndexType)indexType;
- (int)getCharIndexByPoint:(float)x
                         y:(float)y
                 indexType:(ServalMarkdownIndexType)indexType;
- (NSRange)getCharRangeByPoint:(float)x
                             y:(float)y
                     indexType:(ServalMarkdownIndexType)indexType
                     rangeType:(ServalMarkdownCharRangeType)rangeType;
- (void)setTextSelection:(int)start end:(int)end;
- (int)getAnimationStep;
- (int)getRenderedAnimationStep;
- (void)pauseRenderUpdate;
- (void)resumeRenderUpdate;
- (void)disableInternalVSync:(BOOL)disable;
- (void)onRendererFrame:(int64_t)frameTimeNanos;
- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value;
- (void)setStringProp:(ServalMarkdownProps)prop Value:(nullable NSString*)value;
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value;
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color;
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(nullable NSArray*)array;
- (void)setMapProp:(ServalMarkdownProps)prop Value:(nullable NSDictionary*)dict;
- (void)onImageLoaded:(nullable NSString*)url;
- (void)onFontLoaded:(nullable NSString*)family
              Weight:(int)weight
               Style:(int)style;
@end

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
