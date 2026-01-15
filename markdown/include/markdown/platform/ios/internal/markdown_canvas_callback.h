// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_CALLBACK_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_CALLBACK_H_
#import <CoreText/CTFont.h>
@protocol MarkdownCanvasCallback <NSObject>
- (void)SetInlineViewVisible:(NSString*)idSelector;
@end

#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_CALLBACK_H_
