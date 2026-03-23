// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#import <UIKit/UIKit.h>

#import <ServalMarkdown/IMarkdownEventDelegate.h>
#import <ServalMarkdown/IMarkdownExposureDelegate.h>
#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <ServalMarkdown/IMarkdownResourceDelegate.h>
#import <ServalMarkdown/MarkdownCustomDrawView.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>
@interface ServalMarkdownView : MarkdownCustomDrawView
@property(nonatomic, strong) NSString* content;
@property(nonatomic, strong) NSDictionary* style;
@property(nonatomic, assign) ServalMarkdownAnimationType animationType;
@property(nonatomic, assign) float animationVelocity;
@property(nonatomic, assign) int initialAnimationStep;
@property(nonatomic, weak) id<IMarkdownResourceDelegate> resourceDelegate;
@property(nonatomic, weak) id<IMarkdownEventDelegate> eventDelegate;
@property(nonatomic, weak) id<IMarkdownExposureDelegate> exposureDelegate;

- (void)setNumberProp:(ServalMarkdownProps)prop Value:(double)value;
- (void)setStringProp:(ServalMarkdownProps)prop Value:(NSString*)value;
- (void)setBooleanProp:(ServalMarkdownProps)prop Value:(BOOL)value;
- (void)setColorProp:(ServalMarkdownProps)prop Value:(uint32_t)color;
- (void)setArrayProp:(ServalMarkdownProps)prop Value:(NSArray*)array;
- (void)setMapProp:(ServalMarkdownProps)prop Value:(NSDictionary*)dict;
@end
#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
