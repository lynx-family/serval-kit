// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/element/markdown_run_delegates.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "markdown/parser/markdown_parser.h"
#include "markdown/utils/markdown_platform.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {

MarkdownOrderedListMarkDelegate::MarkdownOrderedListMarkDelegate(
    const std::string& content, const tttext::Style& base_style,
    const tttext::ParagraphStyle& paragraph_style,
    const MarkdownOrderedListNumberStyle& style)
    : style_(style) {
  para_ = tttext::Paragraph::Create();
  auto new_style = base_style;
  MarkdownParser::SetTTStyleByMarkdownBaseStyle(style.base_, &new_style);
  para_->SetParagraphStyle(&paragraph_style);
  para_->AddTextRun(&new_style, content.c_str());
}

void MarkdownOrderedListMarkDelegate::Layout() {
  if (layout_) {
    return;
  }
  auto* layout = MarkdownPlatform::GetTextLayout();
  page_ = std::make_unique<tttext::LayoutRegion>(
      std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
      tttext::LayoutMode::kAtMost, tttext::LayoutMode::kAtMost);
  tttext::TTTextContext context;
  context.SetHarmonyShaperForceLowAPI(true);
  layout->Layout(para_.get(), page_.get(), context);
  const float text_width =
      MarkdownPlatform::GetMdLayoutRegionWidth(page_.get());
  const float text_height =
      MarkdownPlatform::GetMdLayoutRegionHeight(page_.get());
  const float text_base_line = page_->GetLine(0)->GetLineBaseLine();
  const float left = style_.block_.margin_left_;
  const float top = style_.block_.margin_top_;
  const float right = style_.block_.margin_right_;
  const float bottom = style_.block_.margin_bottom_;
  advance_ = text_width + left + right;
  ascent_ = -(text_base_line + top);
  descent_ = (text_height - text_base_line) + bottom;
  layout_ = true;
}

void MarkdownOrderedListMarkDelegate::Draw(tttext::ICanvasHelper* canvas,
                                           float x, float y) {
  const float left = style_.block_.margin_left_;
  const float top = style_.block_.margin_top_;
  auto painter = canvas->CreatePainter();
  canvas->Save();
  canvas->Translate(x + left, y + top);
  tttext::LayoutDrawer drawer(canvas);
  drawer.DrawLayoutPage(page_.get());
  canvas->Restore();
}

void MarkdownRefDelegate::Layout() {
  if (layout_) return;
  paragraph_->GetParagraphStyle().SetHorizontalAlign(
      tttext::ParagraphHorizontalAlignment::kLeft);
  auto* layout = MarkdownPlatform::GetTextLayout();
  page_ = std::make_unique<tttext::LayoutRegion>(
      std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
      tttext::LayoutMode::kAtMost, tttext::LayoutMode::kAtMost);
  tttext::TTTextContext context;
  context.SetHarmonyShaperForceLowAPI(true);
  layout->Layout(paragraph_.get(), page_.get(), context);
  float text_width = MarkdownPlatform::GetMdLayoutRegionWidth(page_.get());
  float text_height = MarkdownPlatform::GetMdLayoutRegionHeight(page_.get());
  float text_base_line = page_->GetLine(0)->GetLineBaseLine();
  if (style_.ref_.background_type_ == MarkdownBackgroundType::kCapsule) {
    constexpr float sqrt5_2 = 2.2360679775 * 0.5;
    float diameter = text_height * sqrt5_2;
    width_ = diameter + std::max(0.f, text_width - text_height * 0.5f);
    height_ = diameter;
    base_line_ = text_base_line + (height_ - text_height) * 0.5;
  } else {
    width_ = text_width;
    height_ = text_height;
    base_line_ = text_base_line;
  }
  layout_ = true;
}

void lynx::markdown::MarkdownRefDelegate::Draw(tttext::ICanvasHelper* canvas,
                                               float x, float y) {
  canvas->Save();
  canvas->Translate(style_.block_.margin_left_ + x, y);
  auto drawer_painter = canvas->CreatePainter();
  tttext::LayoutDrawer drawer(canvas);
  if (style_.ref_.background_type_ == MarkdownBackgroundType::kCapsule) {
    drawer_painter->SetFillColor(style_.base_.background_color_);
    float r = height_ * 0.5;
    if (width_ == height_) {
      canvas->DrawCircle(r, r, r, drawer_painter.get());
    } else {
      canvas->DrawCircle(r, r, r, drawer_painter.get());
      canvas->DrawCircle(width_ - r, r, r, drawer_painter.get());
      canvas->DrawRect(r, 0, width_ - r, height_, drawer_painter.get());
    }
    float offset_x =
        (width_ - MarkdownPlatform::GetMdLayoutRegionWidth(page_.get())) * 0.5;
    float offset_y =
        (height_ - MarkdownPlatform::GetMdLayoutRegionHeight(page_.get())) *
        0.5;
    canvas->Translate(offset_x, offset_y);
    drawer.DrawLayoutPage(page_.get());
  } else {
    drawer.DrawLayoutPage(page_.get());
  }
  canvas->Restore();
}

void MarkdownTextDelegate::Layout() {
  auto* layout = MarkdownPlatform::GetTextLayout();
  page_ = std::make_unique<tttext::LayoutRegion>(
      width_ > 0 ? width_ : std::numeric_limits<float>::max(),
      height_ > 0 ? height_ : std::numeric_limits<float>::max(),
      tttext::LayoutMode::kAtMost, tttext::LayoutMode::kAtMost);
  tttext::TTTextContext context;
  context.SetHarmonyShaperForceLowAPI(true);
  context.SetLastLineCanOverflow(false);
  layout->LayoutEx(text_.get(), page_.get(), context);
  context.Reset();
  auto content_width = MarkdownPlatform::GetMdLayoutRegionWidth(page_.get());
  auto content_height = MarkdownPlatform::GetMdLayoutRegionHeight(page_.get());
  page_ = std::make_unique<tttext::LayoutRegion>(content_width, content_height);
  tttext::TTTextContext context2;
  layout->LayoutEx(text_.get(), page_.get(), context2);
}

void MarkdownTextDelegate::Draw(tttext::ICanvasHelper* canvas, float x,
                                float y) {
  auto painter = canvas->CreatePainter();
  canvas->Translate(x, y);
  tttext::LayoutDrawer drawer(canvas);
  drawer.DrawLayoutPage(page_.get());
  canvas->Translate(-x, -y);
}

MarkdownInlineBorderDelegate::MarkdownInlineBorderDelegate(
    lynx::markdown::InlineBorderDirection direction,
    lynx::markdown::MarkdownBorderStylePart border_style,
    lynx::markdown::MarkdownBlockStylePart block_style,
    uint32_t background_color, uint32_t char_offset)
    : direction_(direction),
      border_style_(border_style),
      block_style_(block_style),
      background_color_(background_color),
      char_offset_(char_offset) {
  border_rect_type_ = MarkdownSelection::RectType::kCharBounding;
}

void MarkdownInlineBorderDelegate::UpdateDrawRect(std::vector<RectF>&& rect) {
  draw_rect_ = std::move(rect);
}

void MarkdownInlineBorderDelegate::Draw(tttext::ICanvasHelper* canvas, float x,
                                        float y) {
  canvas->Save();
  if (enable_ && !draw_rect_.empty()) {
    DrawOnRects(canvas, x, y, draw_rect_);
  }
  canvas->Restore();
}

void MarkdownInlineBorderDelegate::DrawOnRects(
    tttext::ICanvasHelper* canvas, float x, float y,
    const std::vector<RectF>& rects) {
  auto painter_ = canvas->CreatePainter();
  canvas->Save();
  for (auto& rect : rects) {
    // background
    painter_->SetFillColor(background_color_);
    float left_offset =
        (&rect == &(rects.front())) ? -block_style_.padding_left_ : 0;
    float right_offset =
        (&rect == &(rects.back())) ? block_style_.padding_right_ : 0;
    canvas->DrawRoundRect(rect.GetLeft() + left_offset, rect.GetTop(),
                          rect.GetRight() + right_offset, rect.GetBottom(),
                          border_style_.border_radius_, painter_.get());
    if (background_drawable_ != nullptr) {
      canvas->Save();
      background_drawable_->Draw(canvas, rect.GetLeft(), rect.GetTop(),
                                 rect.GetRight(), rect.GetBottom());
      canvas->Restore();
    }
    // border
    if (border_style_.border_width_ > 0) {
      painter_->SetStrokeWidth(border_style_.border_width_);
      painter_->SetStrokeColor(border_style_.border_color_);
      canvas->DrawRoundRect(
          rect.GetLeft() + left_offset - border_style_.border_width_ / 2,
          rect.GetTop() - border_style_.border_width_ / 2,
          rect.GetRight() + right_offset + border_style_.border_width_ / 2,
          rect.GetBottom() + border_style_.border_width_ / 2,
          border_style_.border_radius_, painter_.get());
    }
  }
  canvas->Restore();
}

void CircleDelegate::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  auto painter = canvas->CreatePainter();
  painter->SetFillColor(color_);
  canvas->DrawCircle(x + radius_, y + radius_, radius_, painter.get());
}

}  // namespace markdown
}  // namespace lynx
