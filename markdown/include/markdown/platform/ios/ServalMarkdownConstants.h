// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_SERVAL_MARKDOWN_PROPS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_SERVAL_MARKDOWN_PROPS_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, ServalMarkdownProps) {
  kServalMarkdownPropsAnimationType,
  kServalMarkdownPropsAnimationVelocity,
  kServalMarkdownPropsTextMaxline,
  kServalMarkdownPropsContentComplete,
  kServalMarkdownPropsTypewriterDynamicHeight,
  kServalMarkdownPropsInitialAnimationStep,
  kServalMarkdownPropsMarkdownMaxHeight,
  kServalMarkdownPropsContentRangeStart,
  kServalMarkdownPropsContentRangeEnd,
  kServalMarkdownPropsExposureTags,
  kServalMarkdownPropsAnimationFrameRate,
  kServalMarkdownPropsTypewriterHeightTransitionDuration,
  kServalMarkdownPropsAllowBreakAroundPunctuation,
  kServalMarkdownPropsEnableTextSelection,
  kServalMarkdownPropsSelectionHighlightColor,
  kServalMarkdownPropsSelectionHandleColor,
  kServalMarkdownPropsSelectionHandleSize,
  kServalMarkdownPropsSelectionHandleTouchMargin,
  kServalMarkdownPropsMarkdownEffect,
  kServalMarkdownPropsTextMarkAttachments,
  kServalMarkdownPropsTypewriterHeightTransitionPrefetch,
};

typedef NS_ENUM(NSUInteger, ServalMarkdownTextOverflow) {
  kServalMarkdownTextOverflowClip = 0,
  kServalMarkdownTextOverflowEllipsis = 1,
};

typedef NS_ENUM(NSUInteger, ServalMarkdownSelectionHandleType) {
  kServalMarkdownSelectionHandleTypeLeft = 0,
  kServalMarkdownSelectionHandleTypeRight = 1,
};

typedef NS_ENUM(NSUInteger, ServalMarkdownSelectionState) {
  kServalMarkdownSelectionStateEnter = 0,
  kServalMarkdownSelectionStateMove = 1,
  kServalMarkdownSelectionStateStop = 2,
  kServalMarkdownSelectionStateExit = 3,
};

typedef NS_ENUM(NSUInteger, ServalMarkdownLayoutMode) {
  kServalMarkdownLayoutModeIndefinite,
  kServalMarkdownLayoutModeDefinite,
  kServalMarkdownLayoutModeAtMost,
};

typedef enum : NSUInteger {
  kServalMarkdownAnimationTypeNone,
  kServalMarkdownAnimationTypeTypewriter,
} ServalMarkdownAnimationType;

#endif  //MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_SERVAL_MARKDOWN_PROPS_H_
