// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion.
// See sheet/LICENSE.screens for the MIT license covering those portions.

#import <UIKit/UIKit.h>
#import "SRSheetConfiguration.h"

NS_ASSUME_NONNULL_BEGIN
@protocol SRSheetControllerDelegate <NSObject>
- (void)sheetLifecycle:(NSString *)phase;
- (void)sheetTargetIndexChanged:(NSInteger)index;
- (void)sheetSnapSettled:(NSInteger)index height:(CGFloat)height;
- (void)sheetDismissPrevented:(NSString *)channel;
- (void)sheetRequestDismiss:(NSString *)channel;
@end

/** The shared custom native presenter. All access is on the UI thread. */
@interface SRSheetController
    : UIViewController <UIGestureRecognizerDelegate, UIViewControllerTransitioningDelegate>
@property(nonatomic, weak, nullable) id<SRSheetControllerDelegate> delegate;
@property(nonatomic, weak, nullable) UIView *anchorView;
@property(nonatomic, copy) SRSheetConfiguration *configuration;
@property(nonatomic, copy, nullable) CGRect (^resolveArea)(UIWindow *window);
@property(nonatomic, copy, nullable) CGFloat (^measureContentHeight)(void);
@property(nonatomic, readonly) NSInteger currentIndex;
@property(nonatomic, readonly) CGFloat currentHeight;
- (instancetype)initWithContentView:(UIView *)contentView;
- (void)applyConfigurationApplyingInitialDetent:(BOOL)applyInitialDetent;
- (void)invalidateDetents;
- (void)hostAreaDidChange;
- (void)presentSheetInParentViewController:(UIViewController *)parent
                             containerView:(UIView *)containerView
                                  animated:(BOOL)animated
                                completion:(void (^)(void))completion;
- (void)dismissSheetWithCompletion:(void (^)(void))completion;
- (void)dismissSheetImmediatelyWithCompletion:(void (^)(void))completion;
- (void)snapToIndex:(NSInteger)index animated:(BOOL)animated;
@end
NS_ASSUME_NONNULL_END
