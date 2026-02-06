// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/draw/markdown_typewriter_drawer.h"

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_selection.h"
namespace lynx {
namespace markdown {

PointF MarkdownCharTypewriterDrawer::CalculateCursorPosition(
    tttext::TextLine* cursor_line, lynx::markdown::PointF cursor_position,
    PointF region_offset, tttext::RunDelegate* cursor, float page_width,
    MarkdownVerticalAlign align) {
  cursor->Layout();
  auto cursor_width = cursor->GetAdvance();
  auto cursor_ascent = -cursor->GetAscent();
  auto cursor_descent = cursor->GetDescent();
  if (cursor_line != nullptr) {
    if (cursor_width + cursor_position.x_ + region_offset.x_ > page_width) {
      // cursor need move to next line
      cursor_position.x_ = 0;
      cursor_position.y_ = cursor_line->GetLineBottom() + cursor_ascent;
    } else {
      // apply vertical align
      switch (align) {
        case MarkdownVerticalAlign::kTop:
          cursor_position.y_ = cursor_line->GetLineTop() + cursor_ascent;
          break;
        case MarkdownVerticalAlign::kBottom:
          cursor_position.y_ = cursor_line->GetLineBottom() - cursor_descent;
          break;
        case MarkdownVerticalAlign::kCenter:
          cursor_position.y_ =
              (cursor_line->GetLineTop() + cursor_line->GetLineBottom() -
               cursor_descent + cursor_ascent) /
              2;
          break;
        case MarkdownVerticalAlign::kBaseline:
        default:
          cursor_position.y_ = cursor_line->GetLineBaseLine();
          break;
      }
    }
  }
  cursor_position.y_ -= cursor_ascent;
  return cursor_position;
}

std::unique_ptr<tttext::RunDelegate>
MarkdownCharTypewriterDrawer::CreateEllipsis(float text_size, uint32_t color) {
  auto paragraph = tttext::Paragraph::Create();
  tttext::Style style;
  style.SetFontDescriptor(
      tttext::FontDescriptor{{}, tttext::FontStyle::Normal(), 0});
  style.SetTextSize(text_size);
  style.SetForegroundColor(tttext::TTColor(color));
  paragraph->AddTextRun(&style, "…");
  return std::make_unique<MarkdownTextDelegate>(std::move(paragraph), -1, -1);
}

MarkdownCharTypewriterDrawer::MarkdownCharTypewriterDrawer(
    tttext::ICanvasHelper* canvas, int32_t max_char_count,
    MarkdownResourceLoader* loader, const MarkdownTypewriterCursorStyle& style,
    bool draw_cursor_if_complete, tttext::RunDelegate* custom_typewriter_cursor)
    : MarkdownDrawer(canvas),
      style_(&style),
      max_char_count_(max_char_count),
      draw_cursor_if_complete_(draw_cursor_if_complete),
      custom_typewriter_cursor_(custom_typewriter_cursor) {}

void MarkdownCharTypewriterDrawer::DrawPage(const MarkdownPage& page) {
  page_ = &page;
  MarkdownDrawer::DrawPage(page);
  DrawTypewriterCursor();
}

void MarkdownCharTypewriterDrawer::DrawTypewriterCursor() {
  cursor_position_ = CalculateCursorPosition(page_);
  bool typewriter_complete =
      max_char_count_ >= MarkdownSelection::GetPageCharCount(page_);
  if (typewriter_cursor_ != nullptr &&
      (!typewriter_complete || draw_cursor_if_complete_)) {
    canvas_->Save();
    typewriter_cursor_->Draw(canvas_, cursor_position_.x_, cursor_position_.y_);
    canvas_->Restore();
  }
}

void MarkdownCharTypewriterDrawer::DrawTextRegion(
    tttext::LayoutRegion* page, tttext::LayoutDrawer* drawer) {
  if (page == nullptr || terminated_)
    return;
  for (uint32_t i = 0; i < page->GetLineCount(); i++) {
    auto* line = page->GetLine(i);
    const auto char_count =
        std::min(max_char_count_ - draw_char_count_,
                 static_cast<int32_t>(line->GetCharCount()));
    drawer->DrawTextLine(line, 0, char_count);
    draw_char_count_ += char_count;
    if (draw_char_count_ == max_char_count_) {
      terminated_ = true;
      return;
    }
  }
}

void MarkdownCharTypewriterDrawer::DrawRegion(const MarkdownPage& page,
                                              uint32_t region_index) {
  if (region_index >= page.GetRegionCount())
    return;
  page_ = &page;
  const auto* region = page.GetRegion(region_index);
  const int32_t char_start = region->element_->GetCharStart();
  const int32_t char_end = char_start + region->element_->GetCharCount();
  if (char_start >= max_char_count_)
    return;
  draw_char_count_ = char_start;
  MarkdownDrawer::DrawRegion(page, region_index);
  if (char_end >= max_char_count_) {
    canvas_->Save();
    DrawTypewriterCursor();
    canvas_->Restore();
  }
}

PointF MarkdownCharTypewriterDrawer::CalculateCursorPosition(
    const MarkdownPage* page) {
  page_ = page;
  auto max_char_count = max_char_count_;
  auto page_char_count = MarkdownSelection::GetPageCharCount(page);
  max_char_count = std::min(page_char_count, max_char_count) - 1;
  auto region_infos = MarkdownSelection::GetSelectionRegionsByCharRange(
      page, max_char_count, max_char_count + 1);
  if (region_infos.empty()) {
    return {0, 0};
  }
  auto& region_info = region_infos.back();
  auto* region = region_info.region_;
  if (region->GetLineCount() == 0) {
    max_draw_height_ = region_info.offset_.y_;
    return region_info.offset_;
  }
  auto char_index_in_region = max_char_count - region_info.char_pos_offset_;
  tttext::TextLine* line = nullptr;
  for (uint32_t i = 0; i < region->GetLineCount(); i++) {
    line = region->GetLine(i);
    if (static_cast<int32_t>(line->GetEndCharPos()) > char_index_in_region) {
      break;
    }
  }
  max_draw_height_ = region_info.offset_.y_ + line->GetLineBottom();
  float bounding_rect[4];
  line->GetCharBoundingRect(bounding_rect, char_index_in_region);
  auto [left, top, width, height] = bounding_rect;
  typewriter_cursor_ =
      LoadTypewriterCursor((height > 0 ? height : line->GetLineHeight()) / 1.2,
                           tttext::TTColor::BLACK);
  auto cursor_x = left + width;
  auto cursor_y = line->GetLineBaseLine();
  auto cursor_position = CalculateCursorPosition(
      line, {cursor_x, cursor_y}, region_info.offset_, typewriter_cursor_,
      page_->GetMaxWidth(),
      style_ != nullptr ? style_->typewriter_cursor_.vertical_align_
                        : MarkdownVerticalAlign::kBaseline);
  cursor_position += region_info.offset_;
  max_draw_height_ = std::max(
      max_draw_height_, cursor_position.y_ + typewriter_cursor_->GetDescent() -
                            typewriter_cursor_->GetAscent());
  return cursor_position;
}

tttext::RunDelegate* MarkdownCharTypewriterDrawer::LoadTypewriterCursor(
    float size, uint32_t color) {
  if (custom_typewriter_cursor_ != nullptr) {
    return custom_typewriter_cursor_;
  }

  if (!style_->typewriter_cursor_.custom_cursor_.empty()) {
    default_typewriter_cursor_ =
        std::make_unique<MarkdownEmptySpaceDelegate>(0);
    return default_typewriter_cursor_.get();
  }
  default_typewriter_cursor_ = CreateEllipsis(size, color);
  return default_typewriter_cursor_.get();
}

void MarkdownCharTypewriterDrawer::DrawAttachment(
    const MarkdownPage& page, MarkdownTextAttachment* attachment) {
  DrawAttachmentOnRegion(page, attachment, 0,
                         std::numeric_limits<int32_t>::max());
}

void MarkdownCharTypewriterDrawer::DrawAttachmentOnRegion(
    const MarkdownPage& page, MarkdownTextAttachment* attachment,
    int32_t region_char_start, int32_t region_char_end) {
  int32_t start_index = attachment->start_index_;
  int32_t end_index = attachment->end_index_;
  if (start_index < 0) {
    start_index = max_char_count_ + 1 + start_index;
    start_index = std::max(0, start_index);
  }
  if (end_index < 0) {
    end_index = max_char_count_ + 1 + end_index;
    end_index = std::max(0, end_index);
  }
  if (start_index >= end_index) {
    return;
  }
  if (region_char_start > end_index || region_char_end <= start_index) {
    return;
  }
  if (max_char_count_ <= start_index) {
    return;
  }
  const auto rects_origin = MarkdownSelection::GetSelectionRectByCharPos(
      &page, start_index, end_index,
      MarkdownSelection::RectType::kLineBounding);
  float total_width = 0;
  for (auto& r : rects_origin) {
    total_width += r.GetWidth();
  }
  if (max_char_count_ < end_index) {
    end_index = max_char_count_;
    const auto rects = MarkdownSelection::GetSelectionRectByCharPos(
        &page, start_index, end_index,
        MarkdownSelection::RectType::kLineBounding);
    attachment->DrawOnMultiLines(canvas_, rects, total_width);
  } else {
    attachment->DrawOnMultiLines(canvas_, rects_origin, total_width);
  }
}

}  // namespace markdown
}  // namespace lynx
