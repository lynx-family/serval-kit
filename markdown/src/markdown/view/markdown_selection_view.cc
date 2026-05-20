// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_selection_view.h"

#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_context.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_platform_view.h"
namespace serval::markdown {
namespace {

float GetSelectionHandleLineWidth() {
  return MarkdownScreenMetrics::DPToPx(2);
}

PointF MakePoint(float x, float y) {
  return {x, y};
}

void AddArc(MarkdownPath* path, PointF center, float radius, float start_angle,
            float end_angle) {
  path->AddArc({.center_ = center,
                .radius_ = radius,
                .start_angle_ = start_angle,
                .end_angle_ = end_angle});
}

MarkdownPath MakeWaterDropPath(float size, SelectionHandleType handle_type) {
  MarkdownPath path;
  const float radius = size / 2;
  if (handle_type == SelectionHandleType::kLeftHandle) {
    path.MoveTo(MakePoint(size, 0));
    path.LineTo(MakePoint(radius, 0));
    AddArc(&path, MakePoint(radius, radius), radius, -90, -360);
    path.LineTo(MakePoint(size, 0));
    return path;
  }
  path.MoveTo(MakePoint(0, 0));
  path.LineTo(MakePoint(radius, 0));
  AddArc(&path, MakePoint(radius, radius), radius, -90, 180);
  path.LineTo(MakePoint(0, 0));
  return path;
}

}  // namespace

MeasureResult MarkdownSelectionHandle::OnMeasure(MeasureSpec spec) {
  const auto size = GetSize();
  return {.width_ = size.width_,
          .height_ = size.height_,
          .baseline_ = size.height_};
}
void MarkdownSelectionHandle::Draw(tttext::ICanvasHelper* canvas, float x,
                                   float y) {
  auto painter = canvas->CreatePainter();
  const auto r = size_ / 2;
  painter->SetFillColor(color_);
  if (handle_shape_ == SelectionHandleShape::kWaterDrop) {
    auto* canvas_extend =
        markdown_context_ == nullptr
            ? nullptr
            : markdown_context_->GetMarkdownCanvasExtend(canvas);
    if (canvas_extend == nullptr) {
      return;
    }
    auto path = MakeWaterDropPath(size_, handle_type_);
    canvas_extend->DrawMarkdownPath(&path, painter.get());
  } else {
    canvas->DrawCircle(r, r, r, painter.get());
  }
}
void MarkdownSelectionHandle::UpdateViewRect(PointF pivot,
                                             MarkdownPlatformView* view) const {
  const auto r = size_ / 2;
  float left = pivot.x_ - r;
  float top = pivot.y_;
  if (handle_shape_ == SelectionHandleShape::kWaterDrop) {
    if (handle_type_ == SelectionHandleType::kLeftHandle) {
      left = pivot.x_ - size_;
    } else {
      left = pivot.x_;
    }
  } else if (handle_type_ == SelectionHandleType::kLeftHandle) {
    top = pivot.y_ - text_height_ - size_;
  }
  view->Align(left, top);
  view->SetMeasuredSize(GetSize());
}
SizeF MarkdownSelectionHandle::GetSize() const {
  return {size_, size_};
}

MeasureResult MarkdownSelectionHighlight::OnMeasure(MeasureSpec spec) {
  const float w = bounding_box_.GetWidth();
  const float h = bounding_box_.GetHeight();
  return {.width_ = w, .height_ = h, .baseline_ = h};
}

void MarkdownSelectionHighlight::Draw(tttext::ICanvasHelper* canvas, float x,
                                      float y) {
  canvas->Translate(-bounding_box_.GetLeft(), -bounding_box_.GetTop());
  auto painter = canvas->CreatePainter();
  painter->SetFillColor(color_);
  for (const auto& rect : selection_rects_) {
    canvas->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                     rect.GetBottom(), painter.get());
  }
  if (draw_handle_lines_ && !selection_rects_.empty()) {
    auto handle_painter = canvas->CreatePainter();
    handle_painter->SetStrokeWidth(GetSelectionHandleLineWidth());
    handle_painter->SetStrokeColor(handle_color_);
    const auto& start_rect = selection_rects_.front();
    canvas->DrawLine(start_rect.GetLeft(), start_rect.GetTop(),
                     start_rect.GetLeft(), start_rect.GetBottom(),
                     handle_painter.get());
    const auto& end_rect = selection_rects_.back();
    canvas->DrawLine(end_rect.GetRight(), end_rect.GetTop(),
                     end_rect.GetRight(), end_rect.GetBottom(),
                     handle_painter.get());
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
  if (draw_handle_lines_) {
    const auto half_line_width = GetSelectionHandleLineWidth() / 2;
    bounding_box_ =
        RectF::MakeLTRB(bounding_box_.GetLeft() - half_line_width,
                        bounding_box_.GetTop() - half_line_width,
                        bounding_box_.GetRight() + half_line_width,
                        bounding_box_.GetBottom() + half_line_width);
  }
}

void MarkdownSelectionHighlight::UpdateViewRect(
    MarkdownPlatformView* view) const {
  view->SetMeasuredSize({bounding_box_.GetWidth(), bounding_box_.GetHeight()});
  view->Align(bounding_box_.GetLeft(), bounding_box_.GetTop());
}

}  // namespace serval::markdown
