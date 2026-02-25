// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PROPS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PROPS_H_
#include <cstdint>
namespace lynx::markdown {
enum class MarkdownProps {
  kAnimationType,
  kAnimationVelocity,
  kTextMaxline,
  kContentComplete,
  kTypewriterDynamicHeight,
  kInitialAnimationStep,
  kMarkdownMaxHeight,
  kContentRangeStart,
  kContentRangeEnd,
  kExposureTags,
  kAnimationFrameRate,
  kTypewriterHeightTransitionDuration,
  kAllowBreakAroundPunctuation,
  kEnableTextSelection,
  kSelectionHighlightColor,
  kSelectionHandleColor,
  kSelectionHandleSize,
  kSelectionHandleTouchMargin,
  kMarkdownEffect,
  kTextMarkAttachments,
};
}
#endif  //MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PROPS_H_
