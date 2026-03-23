// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEXPOSUREDELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEXPOSUREDELEGATE_H_

#import <Foundation/Foundation.h>

@protocol IMarkdownExposureDelegate <NSObject>
- (void)onLinkAppear:(nonnull NSString*)url Content:(nonnull NSString*)content;
- (void)onLinkDisappear:(nonnull NSString*)url
                Content:(nonnull NSString*)content;
- (void)onImageAppear:(nonnull NSString*)url;
- (void)onImageDisappear:(nonnull NSString*)url;
@end

#endif  //MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_IMARKDOWNEXPOSUREDELEGATE_H_
