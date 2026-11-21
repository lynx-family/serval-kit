// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SRSheetController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SRSheetHost : NSObject
@property(nonatomic, strong) UIViewController *controller;
/** Nil selects modal presentation; a view selects child-controller containment. */
@property(nonatomic, strong, nullable) UIView *container;
@end

/** Owns one native presentation, its operation queue, and native results. */
@interface SRSheetSession : NSObject
@property(nonatomic, readonly) SRSheetController *controller;
@property(nonatomic, readonly, getter=isOpen) BOOL open;
@property(nonatomic, copy, nullable) SRSheetHost *_Nullable (^resolveHost)(void);
@property(nonatomic, copy, nullable) void (^onEvent)(NSString *name, NSDictionary *detail);
- (instancetype)initWithContentView:(UIView *)contentView anchorView:(UIView *)anchorView;
- (void)configure:(SRSheetConfiguration *)configuration;
- (NSDictionary *)presentAtIndex:(NSInteger)index requestId:(nullable NSString *)requestId;
- (NSDictionary *)presentAtIndex:(NSInteger)index
                        animated:(BOOL)animated
                       requestId:(nullable NSString *)requestId;
- (NSDictionary *)dismissAnimated:(BOOL)animated requestId:(nullable NSString *)requestId;
- (NSDictionary *)dismissWithRequestId:(nullable NSString *)requestId;
- (NSDictionary *)snapToIndex:(NSInteger)index
                     animated:(BOOL)animated
                    requestId:(nullable NSString *)requestId;
- (void)hostDidChange;
- (void)contentDidChange;
- (void)invalidate;
@end

NS_ASSUME_NONNULL_END
