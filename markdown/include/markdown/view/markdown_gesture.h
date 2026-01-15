// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_GESTURE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_GESTURE_H_
#include <cstdint>
#include <functional>
#include "markdown/utils/markdown_definition.h"
namespace lynx::markdown {
enum class GestureEventType : uint8_t {
  kUnknown = 0,
  kDown = 1,
  kMove = 2,
  kUp = 3,
  kCancel = 4,
};
using TapGestureListener =
    std::function<void(PointF position, GestureEventType event)>;
using LongPressGestureListener =
    std::function<void(PointF position, GestureEventType event)>;
using PanGestureListener =
    std::function<void(PointF position, PointF motion, GestureEventType event)>;
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_GESTURE_H_
