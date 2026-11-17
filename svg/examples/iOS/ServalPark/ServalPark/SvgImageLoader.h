// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SvgImageLoader : NSObject

// For the demo's main-thread SrSVG bitmap preview, not background renders.
// Remote images return nil until ready, then redraw each requesting view.
- (nullable UIImage*)imageForHref:(nullable NSString*)href view:(UIView*)view;
- (void)clearCallbacks;

@end

NS_ASSUME_NONNULL_END
