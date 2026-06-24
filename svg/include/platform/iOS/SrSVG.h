// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import "SrSVGRenderResult.h"

NS_ASSUME_NONNULL_BEGIN

typedef UIImage* _Nullable (^SrSvgImageCallback)(NSString* _Nullable href);

@interface SrSVG : NSObject

- (instancetype)initWithString:(NSString*)svgDoc;
- (instancetype)initWithData:(NSData*)data;
- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                          andCallback:(SrSvgImageCallback)imageCb;
- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(nullable NSString*)color
                          andCallback:(SrSvgImageCallback)imageCb;
// Preferred entry for callers that need both the rendered image and diagnostics.
- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(nullable NSString*)color
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result;
// Renders an animated SVG at a deterministic SMIL timestamp.
- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(nullable NSString*)color
                               atTime:(NSTimeInterval)seconds
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result;
- (BOOL)hasAnimations;

@end

NS_ASSUME_NONNULL_END
