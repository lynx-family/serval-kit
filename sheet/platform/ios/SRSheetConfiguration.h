// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/** Native configuration, independent of the element's input protocol. */
@interface SRSheetConfiguration : NSObject <NSCopying>
@property(nonatomic, copy) NSArray<NSNumber *> *detents;
@property(nonatomic) NSInteger initialDetentIndex;
@property(nonatomic) NSInteger largestUndimmedDetentIndex;
@property(nonatomic) CGFloat preferredCornerRadius;
@property(nonatomic) BOOL prefersGrabberVisible;
@property(nonatomic) BOOL prefersScrollingExpandsWhenScrolledToEdge;
@property(nonatomic) BOOL gestureEnabled;
@property(nonatomic) BOOL animated;
@property(nonatomic, copy) NSArray<NSString *> *preventNativeDismissChannels;
@property(nonatomic) BOOL preventNativeDismissDragFeedback;
@property(nonatomic, strong) UIColor *backgroundColor;
@property(nonatomic, strong) UIColor *backdropColor;
@property(nonatomic) CGFloat backdropOpacity;
- (BOOL)isDismissPreventedForChannel:(NSString *)channel;
@end

NS_ASSUME_NONNULL_END
