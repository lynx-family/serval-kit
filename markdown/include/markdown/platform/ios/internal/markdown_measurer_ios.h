// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MEASURER_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MEASURER_IOS_H_

#import <ServalMarkdown/MarkdownMeasurer.h>

namespace serval::markdown {
class MarkdownMainViewIOS;
class MarkdownView;
}  // namespace serval::markdown

NS_ASSUME_NONNULL_BEGIN

@interface MarkdownMeasurer (Internal)

- (serval::markdown::MarkdownView*)getMarkdownView;
- (BOOL)bindToView:(serval::markdown::MarkdownMainViewIOS*)view;
- (void)detachFromView:(serval::markdown::MarkdownMainViewIOS*)view;

@end

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_MEASURER_IOS_H_
