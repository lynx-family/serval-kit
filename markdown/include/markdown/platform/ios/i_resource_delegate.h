// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
#define THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_

@interface LynxMarkdownViewInfo : NSObject
@property(nonatomic) CGSize size;
@property(nonatomic) float baseline;
@property(nonatomic, nullable) NSObject* style;
@property(nonatomic) float font_size;
@end

@protocol IResourceDelegate <NSObject>
- (nullable UIImage*)loadImageByURL:(nonnull NSString*)url;
- (nullable UIFont*)loadFontByFamilyName:(nonnull NSString*)family;
- (nullable LynxMarkdownViewInfo*)measureInlineView:
                                      (nonnull NSString*)idSelector
                                           MaxWidth:(float)width
                                          MaxHeight:(float)height;
- (nullable NSArray*)generateBackgroundByImage:(nonnull NSString*)image
                                      FontSize:(float)font_size
                                  RootFontSize:(float)root_font_size;
@end

@protocol IEventDelegate <NSObject>
- (BOOL)isBindEvent:(nonnull NSString*)name;
- (void)dispatchCustomEvent:(nonnull NSString*)name
                     Detail:(nullable NSDictionary*)detail;
@end
#endif  // THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
