// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef UIImage* _Nullable (^SrSvgImageCallback)(NSString* _Nullable href);

@interface SrSVG : NSObject

- (instancetype)initWithString:(NSString*)svgDoc;
- (instancetype)initWithData:(NSData*)data;
- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                          andCallback:(SrSvgImageCallback)imageCb;

@end

NS_ASSUME_NONNULL_END
