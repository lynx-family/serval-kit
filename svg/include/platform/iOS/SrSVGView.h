// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import "SrSVGRenderResult.h"

NS_ASSUME_NONNULL_BEGIN

@interface SrSVGView : UIView

@property(nonatomic, copy, nullable) NSString* color;

- (instancetype)initWithString:(NSString*)svgDoc;
- (instancetype)initWithData:(NSData*)data;
// Preferred entry for callers that need both rendering and diagnostics.
- (SrSVGRenderResult*)parseContentWithResult:(NSString*)content;

@end

NS_ASSUME_NONNULL_END
