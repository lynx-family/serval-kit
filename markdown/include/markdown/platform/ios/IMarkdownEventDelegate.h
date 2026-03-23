// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEVENTDELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEVENTDELEGATE_H_

#import <Foundation/Foundation.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

@protocol IMarkdownEventDelegate <NSObject>
- (void)onParseEnd;
- (void)onTextOverflow:(ServalMarkdownTextOverflow)overflow;
- (void)onDrawStart;
- (void)onDrawEnd;
- (void)onAnimationStep:(NSInteger)animationStep
       MaxAnimationStep:(NSInteger)maxAnimationStep;
- (void)onLinkClicked:(nonnull NSString*)url Content:(nonnull NSString*)content;
- (void)onImageClicked:(nonnull NSString*)url;
- (void)onSelectionChanged:(NSInteger)startIndex
                  EndIndex:(NSInteger)endIndex
                    Handle:(ServalMarkdownSelectionHandleType)handle
                     State:(ServalMarkdownSelectionState)state;
@end

#endif  //MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEVENTDELEGATE_H_
