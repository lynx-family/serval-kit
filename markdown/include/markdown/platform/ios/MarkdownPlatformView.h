// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWNPLATFORMVIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWNPLATFORMVIEW_H_

#import <UIKit/UIKit.h>

#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>

@interface MarkdownPlatformView : UIView <IMarkdownPlatformViewHandle>
@end

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_MARKDOWNPLATFORMVIEW_H_
