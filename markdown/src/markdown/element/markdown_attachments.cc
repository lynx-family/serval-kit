// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/element/markdown_attachments.h"

#include "markdown/draw/markdown_canvas.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/utils/markdown_float_comparison.h"
#include "markdown/utils/markdown_platform.h"
namespace lynx::markdown {

void MarkdownTextAttachment::DrawOnMultiLines(
    tttext::ICanvasHelper* canvas, const std::vector<RectF>& lines_rect,
    float total_length) const {
  float total_width = total_length;
  if (total_width <= 0) {
    for (auto& r : lines_rect) {
      total_width += r.GetWidth();
    }
  }
  float offset_width = 0;
  for (auto& r : lines_rect) {
    canvas->Save();
    const float left = offset_width == 0 ? 0 : r.GetLeft();
    const float right = FloatsEqual(offset_width + r.GetWidth(), total_width)
                            ? 1e6
                            : r.GetRight();
    canvas->ClipRect(left, 0, right, 1e6, false);
    DrawOnRect(canvas, RectF::MakeLTWH(r.GetLeft() - offset_width, r.GetTop(),
                                       total_width, r.GetHeight()));
    canvas->Restore();
    offset_width += r.GetWidth();
  }
}

float CalculateLength(const MarkdownLengthContext& context,
                      const MarkdownStyleValue* value,
                      const float default_value) {
  if (value == nullptr || !value->IsValid()) {
    return default_value;
  }
  return value->CalculateLengthValue(context);
}

void MarkdownTextAttachment::DrawOnRect(tttext::ICanvasHelper* canvas,
                                        RectF rect) const {
  MarkdownLengthContext context{.base_length_ = rect.GetWidth()};
  const float left =
      rect.GetLeft() + CalculateLength(context, rect_.left_.get(), 0);
  const float right =
      rect.GetLeft() +
      CalculateLength(context, rect_.right_.get(), rect.GetWidth());
  context.base_length_ = rect.GetHeight();
  const float top =
      rect.GetTop() + CalculateLength(context, rect_.top_.get(), 0);
  const float bottom =
      rect.GetTop() +
      CalculateLength(context, rect_.bottom_.get(), rect.GetHeight());

  DrawRect(canvas, RectF::MakeLTRB(left, top, right, bottom), rect_);
  const float border_left =
      CalculateLength(context, border_left_.width_.get(), 0);
  DrawLine(canvas, {left + border_left / 2, top},
           {left + border_left / 2, bottom}, context, border_left_,
           border_left);
  const float border_right =
      CalculateLength(context, border_right_.width_.get(), 0);
  DrawLine(canvas, {right - border_right / 2, top},
           {right - border_right / 2, bottom}, context, border_right_,
           border_right);
  context.base_length_ = rect.GetWidth();
  const float border_top =
      CalculateLength(context, border_top_.width_.get(), 0);
  DrawLine(canvas, {left, top + border_top / 2}, {right, top + border_top / 2},
           context, border_top_, border_top);
  const float border_bottom =
      CalculateLength(context, border_bottom_.width_.get(), 0);
  DrawLine(canvas, {left, bottom - border_bottom / 2},
           {right, bottom - border_bottom / 2}, context, border_bottom_,
           border_bottom);
}

void MarkdownTextAttachment::DrawRect(
    tttext::ICanvasHelper* canvas, RectF rect,
    const MarkdownAttachmentRectStyle& style) {
  if (style.color_ == 0 && style.gradient_ == nullptr)
    return;
  auto painter = canvas->CreatePainter();
  painter->SetFillColor(style.color_);
  painter->SetStrokeColor(style.stroke_color_);
  MarkdownLengthContext context;
  painter->SetStrokeWidth(
      CalculateLength(context, style.stroke_width_.get(), 0));
  float radius = CalculateLength(context, style.radius_.get(), 0);
  if (style.gradient_ != nullptr) {
    auto canvas_extend = MarkdownPlatform::GetMarkdownCanvasExtend(canvas);
    if (canvas_extend == nullptr) {
      return;
    }
    painter->SetFillColor(tttext::TTColor::WHITE);
    MarkdownPath path;
    if (radius == 0) {
      path.AddRect(rect);
    } else {
      path.AddRoundRect(
          {.rect_ = rect, .radius_x_ = radius, .radius_y_ = radius});
    }
    canvas_extend->DrawDelegateOnPath(style.gradient_.get(), &path,
                                      painter.get());
  } else {
    if (radius == 0) {
      canvas->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                       rect.GetBottom(), painter.get());
    } else {
      canvas->DrawRoundRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                            rect.GetBottom(), radius, painter.get());
    }
  }
}

MarkdownPath CreatePath(
    lynx::markdown::PointF start, lynx::markdown::PointF end,
    const MarkdownLengthContext& context,
    const lynx::markdown::MarkdownAttachmentLineStyle& style) {
  MarkdownPath path;
  if (style.line_type_ == MarkdownLineType::kSolid) {
    path.MoveTo(start);
    path.LineTo(end);
  } else if (style.line_type_ == MarkdownLineType::kDashed) {
    auto diff = end - start;
    float length = diff.LengthToZero();
    constexpr float kDefaultElementLength = 2.5f;
    constexpr float kDefaultEmptyLength = 1.5f;
    float empty_len =
        CalculateLength(context, style.empty_size_.get(), kDefaultEmptyLength);
    float element_len = CalculateLength(context, style.element_size_.get(),
                                        kDefaultElementLength);
    auto min_element_num =
        static_cast<int>(std::floor(length / (empty_len + element_len)));
    const int element_num = std::max(min_element_num, 1);
    if (element_num > 1) {
      const int num_empty = element_num - 1;
      empty_len = (length - element_len * min_element_num) / num_empty;
    }
    auto empty_rate = (empty_len / length);
    auto element_rate = (element_len / length);
    for (int i = 0; i < element_num; i++) {
      auto dash_start = start + (diff * ((empty_rate + element_rate) * i));
      auto dash_end =
          start +
          (diff *
           std::min((empty_rate + element_rate) * i + element_rate, 1.f));
      path.MoveTo(dash_start);
      path.LineTo(dash_end);
    }
  }
  return path;
}

void MarkdownTextAttachment::DrawLine(tttext::ICanvasHelper* canvas,
                                      PointF start, PointF end,
                                      const MarkdownLengthContext& context,
                                      const MarkdownAttachmentLineStyle& style,
                                      float width) {
  if (style.line_type_ == MarkdownLineType::kNone)
    return;
  auto painter = canvas->CreatePainter();
  painter->SetStrokeColor(style.color_);
  painter->SetStrokeWidth(width);
  if (style.line_type_ == MarkdownLineType::kSolid &&
      style.gradient_ == nullptr) {
    canvas->DrawLine(start.x_, start.y_, end.x_, end.y_, painter.get());
  } else {
    auto path = CreatePath(start, end, context, style);
    const auto canvas_extend =
        MarkdownPlatform::GetMarkdownCanvasExtend(canvas);
    if (canvas_extend == nullptr) {
      return;
    }
    if (style.gradient_ != nullptr) {
      painter->SetStrokeColor(tttext::TTColor::WHITE);
      canvas_extend->DrawDelegateOnPath(style.gradient_.get(), &path,
                                        painter.get());
    } else {
      canvas_extend->DrawMarkdownPath(&path, painter.get());
    }
  }
}

}  // namespace lynx::markdown
