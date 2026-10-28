// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SRSheetConfiguration.h"

@implementation SRSheetConfiguration
- (instancetype)init {
  if ((self = [super init])) {
    _detents = @[];
    _initialDetentIndex = 0;
    _largestUndimmedDetentIndex = -1;
    _preferredCornerRadius = -1;
    _prefersScrollingExpandsWhenScrolledToEdge = YES;
    _gestureEnabled = YES;
    _animated = YES;
    _preventNativeDismissChannels = @[];
    _backgroundColor = UIColor.clearColor;
    _backdropColor = UIColor.blackColor;
    _backdropOpacity = 0.5;
  }
  return self;
}
- (id)copyWithZone:(NSZone *)zone {
  SRSheetConfiguration *copy = [[[self class] allocWithZone:zone] init];
  copy.detents = self.detents;
  copy.initialDetentIndex = self.initialDetentIndex;
  copy.largestUndimmedDetentIndex = self.largestUndimmedDetentIndex;
  copy.preferredCornerRadius = self.preferredCornerRadius;
  copy.prefersGrabberVisible = self.prefersGrabberVisible;
  copy.prefersScrollingExpandsWhenScrolledToEdge = self.prefersScrollingExpandsWhenScrolledToEdge;
  copy.gestureEnabled = self.gestureEnabled;
  copy.animated = self.animated;
  copy.preventNativeDismissChannels = self.preventNativeDismissChannels;
  copy.preventNativeDismissDragFeedback = self.preventNativeDismissDragFeedback;
  copy.backgroundColor = self.backgroundColor;
  copy.backdropColor = self.backdropColor;
  copy.backdropOpacity = self.backdropOpacity;
  return copy;
}
- (BOOL)isDismissPreventedForChannel:(NSString *)channel {
  return [self.preventNativeDismissChannels containsObject:channel];
}
@end
