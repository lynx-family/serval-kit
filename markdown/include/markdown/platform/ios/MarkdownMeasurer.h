// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_

#import <UIKit/UIKit.h>

@interface MarkdownMeasureResult : NSObject

@property(nonatomic, assign, readonly) CGFloat width;
@property(nonatomic, assign, readonly) CGFloat height;
@property(nonatomic, assign, readonly) NSInteger lineCount;
@property(nonatomic, copy, readonly) NSArray<NSString*>* lines;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

@interface MarkdownMeasurer : NSObject

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

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWN_MEASURER_H_
