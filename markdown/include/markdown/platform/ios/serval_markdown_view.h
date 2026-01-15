// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
#import <CoreFoundation/CoreFoundation.h>

typedef enum : NSUInteger {
  kServalMarkdownLayoutModeIndefinite,
  kServalMarkdownLayoutModeDefinite,
  kServalMarkdownLayoutModeAtMost,
} ServalMarkdownLayoutMode;

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
@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_IOS_SERVAL_MARKDOWN_VIEW_H_
