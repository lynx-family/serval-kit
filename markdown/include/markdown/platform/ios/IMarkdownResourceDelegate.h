// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
#define THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_

#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

// Layout metrics are in points. Ascent is negative and descent is positive.
@protocol IMarkdownFormula <NSObject>
@property(nonatomic, readonly) CGFloat ascent;
@property(nonatomic, readonly) CGFloat descent;
@property(nonatomic, readonly) CGFloat advance;
- (void)drawInContext:(CGContextRef)context rect:(CGRect)rect;
@optional
// Implement both methods for asynchronous resources. Call each registered
// completion once, even when the result becomes ready before registration.
@property(nonatomic, readonly, getter=isLoaded) BOOL loaded;
- (void)setLoadCallback:(dispatch_block_t)callback;
@end

@protocol IMarkdownResourceDelegate <NSObject>
// Called during layout, which may run off the main thread.
- (nullable UIImage*)loadImageByURL:(nonnull NSString*)url;
- (nullable UIFont*)loadFontByFamilyName:(nonnull NSString*)family
                                  Weight:(int)weight
                                   Style:(int)style;
- (nullable id<IMarkdownPlatformViewHandle>)loadInlineView:
    (nonnull NSString*)idSelector;

@optional
// Called during layout. Return nil to display the literal Markdown source.
- (nullable id<IMarkdownFormula>)loadFormula:(NSString*)formula
                                    fontSize:(CGFloat)fontSize
                                       color:(uint32_t)color
                                    maxWidth:(CGFloat)maxWidth
                                 displayMode:(BOOL)displayMode;
// Called during layout, which may run off the main thread.
// Return YES if the image needs an animated image view.
- (BOOL)isAnimatedImage:(nonnull UIImage*)image;
// Called on the main thread when preparing to draw an animated image.
// Return a distinct image view for each call using the already loaded image.
- (nullable UIImageView*)loadAnimatedImage:(nonnull UIImage*)image;
@end

NS_ASSUME_NONNULL_END

#endif  // THIRD_PARTY_MARKDOWN_IOS_I_RESOURCE_DELEGATE_H_
