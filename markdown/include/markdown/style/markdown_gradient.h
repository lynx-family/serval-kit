// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_

#include <memory>
#include <string_view>

#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_value.h"

namespace serval::markdown {

enum class MarkdownGradientType : int32_t {
  kLinear = 2,
  kRadial = 3,
  kConic = 4,
};

enum class MarkdownLinearGradientDirection : int32_t {
  kNone = 0,
  kToTop = 1,
  kToBottom = 2,
  kToLeft = 3,
  kToRight = 4,
  kToTopRight = 5,
  kToTopLeft = 6,
  kToBottomRight = 7,
  kToBottomLeft = 8,
  kAngle = 9,
};

enum class MarkdownBackgroundPositionType : int32_t {
  kTop = -(1 << 5),
  kRight = -(1 << 5) - 1,
  kBottom = -(1 << 5) - 2,
  kLeft = -(1 << 5) - 3,
  kCenter = -(1 << 5) - 4,
};

enum class MarkdownRadialGradientShapeType : int32_t {
  kEllipse = 0,
  kCircle = 1,
};

enum class MarkdownRadialGradientSizeType : int32_t {
  kFarthestCorner = 0,
  kFarthestSide = 1,
  kClosestCorner = 2,
  kClosestSide = 3,
  kLength = 4,
};

enum class MarkdownPlatformLengthUnit : int32_t {
  kNumber = 0,
  kPercentage = 1,
  kCalc = 2,
};

bool IsGradientValue(std::string_view value);

// Parses CSS gradient text into the same nested array shape consumed by
// LynxGradient setters: [type, gradient_args].
std::unique_ptr<Value> ParseGradientValue(
    std::string_view value, const MarkdownLengthContext& context);

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_
