// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
#include <vector>

#include "markdown/view/markdown_platform_view.h"
namespace serval::markdown {
class MarkdownContext;
enum class SelectionHandleType : uint8_t {
  kLeftHandle,
  kRightHandle,
};
enum class SelectionHandleShape : uint8_t {
  kCircle,
  kWaterDrop,
};
class MarkdownSelectionHandle final : public MarkdownDrawable {
 public:
  MarkdownSelectionHandle(
      const float size, const SelectionHandleType type, const uint32_t color,
      const SelectionHandleShape shape = SelectionHandleShape::kCircle)
      : size_(size),
        color_(color),
        handle_type_(type),
        handle_shape_(shape),
        text_height_(0) {}
  ~MarkdownSelectionHandle() override = default;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  void SetSize(float size) { size_ = size; }
  void SetColor(uint32_t color) { color_ = color; }
  void SetTextHeight(float height) { text_height_ = height; }
  void SetShape(SelectionHandleShape shape) { handle_shape_ = shape; }
  void SetMarkdownContext(MarkdownContext* context) {
    markdown_context_ = context;
  }
  void UpdateViewRect(PointF pivot, MarkdownPlatformView* view) const;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
  SizeF GetSize() const;
  float size_;
  uint32_t color_;
  SelectionHandleType handle_type_;
  SelectionHandleShape handle_shape_;
  MarkdownContext* markdown_context_{nullptr};
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
  void SetHandleColor(const uint32_t color) { handle_color_ = color; }
  void SetDrawHandleLines(const bool draw) {
    draw_handle_lines_ = draw;
    CalculateBoundingBox();
  }
  RectF GetBoundingBox() const { return bounding_box_; }
  void UpdateViewRect(MarkdownPlatformView* view) const;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
  void CalculateBoundingBox();
  std::vector<RectF> selection_rects_;
  RectF bounding_box_;
  uint32_t color_{};
  uint32_t handle_color_{};
  bool draw_handle_lines_{true};
};

}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_SELECTION_VIEW_H_
