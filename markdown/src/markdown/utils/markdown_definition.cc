// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_definition.h"

#include "markdown/utils/markdown_float_comparison.h"
namespace lynx {
namespace markdown {
bool RectF::Contains(float x, float y) const {
  return FloatsLargerOrEqual(x, GetLeft()) && FloatsLarger(GetRight(), x) &&
         FloatsLargerOrEqual(y, GetTop()) && FloatsLarger(GetBottom(), y);
}
bool RectF::InterSects(float left, float top, float right, float bottom) const {
  return !(FloatsLarger(GetLeft(), right) || FloatsLarger(GetTop(), bottom) ||
           FloatsLarger(left, GetRight()) || FloatsLarger(top, GetBottom()));
}
bool RectF::InterSects(const RectF& rect) const {
  return InterSects(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                    rect.GetBottom());
}
bool RectF::IsEmpty() const {
  return FloatsEqual(GetWidth(), 0) || FloatsEqual(GetHeight(), 0);
}
bool RectF::operator==(const RectF& rect) {
  return FloatsEqual(x_, rect.x_) && FloatsEqual(y_, rect.y_) &&
         FloatsEqual(width_, rect.width_) && FloatsEqual(height_, rect.height_);
}
bool PointF::operator==(const PointF& point) const {
  return FloatsEqual(x_, point.x_) && FloatsEqual(y_, point.y_);
}
}  // namespace markdown
}  // namespace lynx
