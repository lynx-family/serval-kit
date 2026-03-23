// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
#include "markdown/view/markdown_platform_view.h"
#include "vector"
namespace serval::markdown {
enum class SelectionHandleType : uint8_t {
  kLeftHandle,
  kRightHandle,
};
class MarkdownSelectionHandle final : public MarkdownDrawable {
 public:
  MarkdownSelectionHandle(const float size, const float margin,
                          const SelectionHandleType type, const uint32_t color)
      : size_(size),
        margin_(margin),
        color_(color),
        handle_type_(type),
        text_height_(0) {}
  ~MarkdownSelectionHandle() override = default;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  void SetSize(float size) { size_ = size; }
  void SetColor(uint32_t color) { color_ = color; }
  void SetTextHeight(float height) { text_height_ = height; }
  void SetTouchMargin(float margin) { margin_ = margin; }
  void UpdateViewRect(PointF pivot, MarkdownPlatformView* view) const;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
  SizeF GetSize() const;
  float size_;
  float margin_;
  uint32_t color_;
  SelectionHandleType handle_type_;
  float text_height_;
};

class MarkdownSelectionHighlight final : public MarkdownDrawable {
 public:
  MarkdownSelectionHighlight() = default;
  ~MarkdownSelectionHighlight() override = default;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  void SetRects(const std::vector<RectF>& rects) {
    selection_rects_ = rects;
    CalculateBoundingBox();
  }
  void SetColor(const uint32_t color) { color_ = color; }
  RectF GetBoundingBox() const { return bounding_box_; }
  void UpdateViewRect(MarkdownPlatformView* view) const;

 private:
  MeasureResult OnMeasure(MeasureSpec spec) override;
  void CalculateBoundingBox();
  std::vector<RectF> selection_rects_;
  RectF bounding_box_;
  uint32_t color_{};
};

}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
