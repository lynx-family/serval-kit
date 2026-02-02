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
bool IsUtf8StartByte(const char byte) {
  return (byte & 0b1100'0000) != 0b1000'0000;
}
char32_t GetUnicodeFromUtf8String(const char* string, int32_t* char_len) {
  *char_len = 0;
  int32_t cp = 0;
  uint8_t nxt = string[(*char_len)++];
  if ((nxt & 0x80u) == 0) {
    cp = nxt & 0x7Fu;
    *char_len = 1;
  } else if ((nxt & 0xE0u) == 0xC0) {
    cp = (nxt & 0x1Fu);
    *char_len = 2;
  } else if ((nxt & 0xF0u) == 0xE0) {
    cp = nxt & 0x0Fu;
    *char_len = 3;
  } else if ((nxt & 0xF8u) == 0xF0) {
    cp = nxt & 0x07u;
    *char_len = 4;
  } else {
    cp = 0;
    return -1;
  }
  for (int32_t j = 1; j < *char_len; ++j) {
    nxt = string[j];
    if ((nxt & 0xC0u) != 0x80) {
      return -1;
    }
    cp <<= 6u;
    cp |= nxt & 0x3fu;
  }
  return cp;
}
bool IsEmptyChar(const char32_t unicode) {
  return unicode <= 0x20 || unicode == 0x7f ||
         (unicode >= 0x2000 && unicode <= 0x200a) || unicode == 0x3000 ||
         unicode == 0x2028 || unicode == 0x2029 || unicode == 0x00a0 ||
         unicode == 0x1680 || unicode == 0x202f || unicode == 0x205f;
}
bool IsPunctuation(const char32_t unicode) {
  if ((unicode >= 0x21 && unicode <= 0x2f) ||
      (unicode >= 0x3a && unicode <= 0x40) ||
      (unicode >= 0x5b && unicode <= 0x60) ||
      (unicode >= 0x7b && unicode <= 0x7e) ||
      (unicode >= 0x2010 && unicode <= 0x2027) ||
      (unicode >= 0x2030 && unicode <= 0x205e) ||
      (unicode >= 0x2e00 && unicode <= 0x2e7f) ||
      (unicode > 0x3000 && unicode <= 0x303f) ||
      (unicode >= 0xfe10 && unicode <= 0xfe1f) ||
      (unicode >= 0xfe30 && unicode <= 0xfe4f) ||
      (unicode >= 0xfe50 && unicode <= 0xfe6f) ||
      (unicode > 0xff01 && unicode <= 0xff0f) ||
      (unicode > 0xff1a && unicode <= 0xff20) ||
      (unicode >= 0xff3b && unicode <= 0xff40) ||
      (unicode >= 0xff5b && unicode <= 0xff65)) {
    return true;
  }
  return false;
}
}  // namespace markdown
}  // namespace lynx
