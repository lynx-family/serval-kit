// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
#define THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_

#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <UIKit/UIKit.h>
@protocol IMarkdownResourceDelegate <NSObject>
- (nullable UIImage*)loadImageByURL:(nonnull NSString*)url;
- (nullable UIFont*)loadFontByFamilyName:(nonnull NSString*)family
                                  Weight:(int)weight
                                   Style:(int)style;
- (nullable id<IMarkdownPlatformViewHandle>)loadInlineView:
    (nonnull NSString*)idSelector;
@end
#endif  // THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
