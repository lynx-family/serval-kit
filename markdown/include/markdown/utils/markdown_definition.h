// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_DEFINITION_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_DEFINITION_H_
#include <algorithm>
#include <cmath>

#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {
class RectF {
 public:
  RectF() : RectF(0, 0, 0, 0) {}
  RectF(float left, float top, float width, float height)
      : x_(left), y_(top), width_(width), height_(height) {}
  ~RectF() = default;

 public:
  static RectF MakeEmpty() { return {}; }
  static RectF MakeLTRB(float left, float top, float right, float bottom) {
    return RectF(left, top, right - left, bottom - top);
  }
  static RectF MakeLTWH(float left, float top, float width, float height) {
    return RectF(left, top, width, height);
  }
  static RectF MakeWH(float width, float height) {
    return RectF(0, 0, width, height);
  }

 public:
  void SetLeft(float left) { x_ = left; }
  void SetTop(float top) { y_ = top; }
  void SetRight(float right) { width_ = right - x_; }
  void SetBottom(float bottom) { height_ = bottom - y_; }
  void SetWidth(float width) { width_ = width; }
  void SetHeight(float height) { height_ = height; }
  float GetLeft() const { return x_; }
  float GetTop() const { return y_; }
  float GetRight() const { return x_ + width_; }
  float GetBottom() const { return y_ + height_; }
  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  void Offset(float x, float y) {
    x_ += x;
    y_ += y;
  }
  void OffsetTo(float abs_x, float abs_y) {
    x_ = abs_x;
    y_ = abs_y;
  }
  bool Contains(float x, float y) const;
  bool InterSects(float left, float top, float right, float bottom) const;
  bool InterSects(const RectF& rect) const;
  bool IsEmpty() const;
  bool operator==(const RectF& rect) const;
  bool operator!=(const RectF& rect) const { return !operator==(rect); }
  void operator=(const RectF& rect) {
    x_ = rect.x_;
    y_ = rect.y_;
    width_ = rect.width_;
    height_ = rect.height_;
  }
  void Union(const RectF& rect) {
    float x = std::min(x_, rect.GetLeft());
    float y = std::min(y_, rect.GetTop());
    float width = std::max(GetRight(), rect.GetRight()) - x;
    float height = std::max(GetBottom(), rect.GetBottom()) - y;
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
  }

 private:
  float x_;
  float y_;
  float width_;
  float height_;
};

class PointF {
 public:
  inline void SetX(float x) { x_ = x; }
  inline float GetX() const { return x_; }
  inline void SetY(float y) { y_ = y; }
  inline float GetY() const { return y_; }
  inline void Translate(float x, float y) {
    x_ += x;
    y_ += y;
  }
  inline void Translate(const PointF& point) {
    x_ += point.x_;
    y_ += point.y_;
  }

 public:
  PointF operator+(const PointF& other) const {
    return {x_ + other.x_, y_ + other.y_};
  }
  void operator+=(const PointF& other) {
    x_ += other.x_;
    y_ += other.y_;
  }
  PointF operator-(const PointF& other) const {
    return {x_ - other.x_, y_ - other.y_};
  }
  void operator-=(const PointF& other) {
    x_ -= other.x_;
    y_ -= other.y_;
  }
  PointF operator*(float n) const { return {x_ * n, y_ * n}; }
  void operator*=(float n) {
    x_ *= n;
    y_ *= n;
  }
  PointF operator/(float n) const { return {x_ / n, y_ / n}; }
  void operator/=(float n) {
    x_ /= n;
    y_ /= n;
  }
  float LengthToZero() const { return std::sqrt(x_ * x_ + y_ * y_); }

 public:
  bool operator==(const PointF& point) const;
  bool operator!=(const PointF& point) { return !operator==(point); }

 public:
  float x_{0};
  float y_{0};
};

class Range {
 public:
  int32_t start_{0};
  int32_t end_{0};
};

class SizeF {
 public:
  float width_{0};
  float height_{0};
};

struct MeasureSpec {
  static constexpr float LAYOUT_MAX_SIZE = 1e5;
  float width_{LAYOUT_MAX_SIZE};
  tttext::LayoutMode width_mode_{tttext::LayoutMode::kIndefinite};
  float height_{LAYOUT_MAX_SIZE};
  tttext::LayoutMode height_mode_{tttext::LayoutMode::kIndefinite};
};

struct MeasureResult {
  float width_{0};
  float height_{0};
  float baseline_{0};
};

struct Paddings {
  float left_{0};
  float top_{0};
  float right_{0};
  float bottom_{0};
};

struct Margins {
  float left_{0};
  float top_{0};
  float right_{0};
  float bottom_{0};
};
bool IsUtf8StartByte(char byte);
char32_t GetUnicodeFromUtf8String(const char* string, int32_t* char_len);
bool IsEmptyChar(char32_t unicode);
bool IsPunctuation(char32_t unicode);
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_DEFINITION_H_
