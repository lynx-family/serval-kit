// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SrSVGRenderResult : NSObject

// Indicates whether this call produced any parse/render diagnostics.
@property(nonatomic, assign, readonly, getter=hasError) BOOL error;
// A short summary derived from the first diagnostic message, if any.
@property(nonatomic, copy, readonly, nullable) NSString* errorMessage;

- (instancetype)initWithError:(BOOL)error
                 errorMessage:(nullable NSString*)errorMessage;

@end

NS_ASSUME_NONNULL_END
