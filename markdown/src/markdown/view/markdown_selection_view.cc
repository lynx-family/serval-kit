// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_selection_view.h"

#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_platform_view.h"
namespace lynx::markdown {
MeasureResult MarkdownSelectionHandle::OnMeasure(MeasureSpec spec) {
  const auto size = GetSize();
  return {.width_ = size.width_,
          .height_ = size.height_,
          .baseline_ = size.height_};
}
void MarkdownSelectionHandle::Draw(tttext::ICanvasHelper* canvas, float x,
                                   float y) {
  canvas->Translate(margin_, margin_);
  auto painter = canvas->CreatePainter();
  const auto r = size_ / 2;
  if (handle_type_ == SelectionHandleType::kLeftHandle) {
    painter->SetFillColor(color_);
    canvas->DrawCircle(r, r, r, painter.get());
    painter->SetStrokeWidth(MarkdownScreenMetrics::DPToPx(2));
    painter->SetStrokeColor(color_);
    canvas->DrawLine(r, size_, r, size_ + text_height_, painter.get());
  } else {
    painter->SetFillColor(color_);
    canvas->DrawCircle(r, text_height_ + r, r, painter.get());
    painter->SetStrokeWidth(MarkdownScreenMetrics::DPToPx(2));
    painter->SetStrokeColor(color_);
    canvas->DrawLine(r, 0, r, text_height_, painter.get());
  }
  canvas->Translate(-margin_, -margin_);
}
void MarkdownSelectionHandle::UpdateViewRect(PointF pivot,
                                             MarkdownPlatformView* view) const {
  const auto r = size_ / 2;
  if (handle_type_ == SelectionHandleType::kLeftHandle) {
    const float left = pivot.x_ - r - margin_;
    const float top = pivot.y_ - text_height_ - size_ - margin_;
    view->Align(left, top);
  } else {
    const float left = pivot.x_ - r - margin_;
    const float top = pivot.y_ - text_height_ - margin_;
    view->Align(left, top);
  }
  view->SetMeasuredSize(GetSize());
}
SizeF MarkdownSelectionHandle::GetSize() const {
  return {size_ + margin_ * 2, size_ + text_height_ + margin_ * 2};
}

MeasureResult MarkdownSelectionHighlight::OnMeasure(MeasureSpec spec) {
  const float w = bounding_box_.GetWidth();
  const float h = bounding_box_.GetHeight();
  return {.width_ = w, .height_ = h, .baseline_ = h};
}

void MarkdownSelectionHighlight::Draw(tttext::ICanvasHelper* canvas, float x,
                                      float y) {
  canvas->Translate(-bounding_box_.GetLeft(), -bounding_box_.GetTop());
  const auto painter = canvas->CreatePainter();
  painter->SetFillColor(color_);
  for (const auto& rect : selection_rects_) {
    canvas->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                     rect.GetBottom(), painter.get());
  }
  canvas->Translate(bounding_box_.GetLeft(), bounding_box_.GetTop());
}

void MarkdownSelectionHighlight::CalculateBoundingBox() {
  if (selection_rects_.empty()) {
    bounding_box_ = RectF::MakeEmpty();
    return;
  }
  bounding_box_ = selection_rects_.front();
  for (auto& rect : selection_rects_) {
    bounding_box_.Union(rect);
  }
}

void MarkdownSelectionHighlight::UpdateViewRect(
    MarkdownPlatformView* view) const {
  view->SetMeasuredSize({bounding_box_.GetWidth(), bounding_box_.GetHeight()});
  view->Align(bounding_box_.GetLeft(), bounding_box_.GetTop());
}

}  // namespace lynx::markdown
