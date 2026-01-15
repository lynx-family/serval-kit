// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/draw/markdown_typewriter_drawer.h"

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_selection.h"
namespace lynx {
namespace markdown {

MarkdownTypewriterDrawer::MarkdownTypewriterDrawer(
    MarkdownCanvas* canvas, int32_t max_glyph_count,
    MarkdownResourceLoader* loader, const MarkdownTypewriterCursorStyle& style,
    bool draw_cursor_if_complete, tttext::RunDelegate* custom_typewriter_cursor)
    : MarkdownCanvas(nullptr),
      MarkdownDrawer(nullptr),
      origin_canvas_(canvas),
      loader_(loader),
      style_(&style),
      max_glyph_count_(max_glyph_count),
      draw_cursor_if_complete_(draw_cursor_if_complete),
      custom_typewriter_cursor_(custom_typewriter_cursor) {
  if (max_glyph_count >= 0 || canvas == nullptr) {
    canvas_ = this;
  } else {
    canvas_ = origin_canvas_;
  }
  painter_ = CreatePainter();
}

void MarkdownTypewriterDrawer::DrawPage(
    const lynx::markdown::MarkdownPage& page) {
  page_ = &page;
  std::vector<MarkdownInlineBorderDelegate*> split_borders;
  if (origin_canvas_ != nullptr) {
    // TODO(zhouchaoying): refactor these code, ensure typewriter step == char
    // count to avoid convert typewriter step to char count, char count  = step
    // + run delegates place holder(=1) - run delegates char count
    uint32_t typewriter_char_end = max_glyph_count_;
    for (auto [char_offset, step_offset] : page_->GetTypewriterStepOffset()) {
      if (typewriter_char_end >= char_offset) {
        typewriter_char_end += step_offset;
        if (typewriter_char_end <= char_offset) {
          typewriter_char_end = char_offset + 1;
          break;
        }
      } else {
        break;
      }
    }
    for (auto& inline_border : page_->GetInlineBorders()) {
      if (inline_border.left_->GetCharOffset() <= typewriter_char_end + 1 &&
          inline_border.right_->GetCharOffset() > typewriter_char_end) {
        // inline border split by typewriter
        if (typewriter_char_end > inline_border.left_->GetCharOffset()) {
          auto rect_vec = MarkdownSelection::GetSelectionRectByCharPos(
              page_, inline_border.left_->GetCharOffset(), typewriter_char_end,
              inline_border.left_->GetRectType(),
              MarkdownSelection::RectCoordinate::kAbsolute);
          inline_border.left_->DrawOnRects(origin_canvas_, 0, 0, rect_vec);
        }
        inline_border.left_->SetEnable(false);
        split_borders.emplace_back(inline_border.left_);
      }
    }
  }
  MarkdownDrawer::DrawPage(page);
  if (origin_canvas_ != nullptr) {
    for (auto& inline_border : split_borders) {
      inline_border->SetEnable(true);
    }
  }
  if (!terminated_) {
    if (typewriter_cursor_ == nullptr && last_draw_region_ != nullptr) {
      // page draw complete, draw cursor on last region
      if (last_draw_region_->GetLineCount() == 0) {
        cursor_position_ = {0, 0};
        typewriter_cursor_ = LoadTypewriterCursor(painter_.get());
      } else {
        auto* cursor_line =
            last_draw_region_->GetLine(last_draw_region_->GetLineCount() - 1);
        cursor_position_ = {cursor_line->GetLineRight(),
                            cursor_line->GetLineBaseLine()};
        typewriter_cursor_ = LoadTypewriterCursor(painter_.get());
        cursor_position_ = CalculateCursorPosition(
            cursor_line, cursor_position_, region_offset_, typewriter_cursor_);
      }
      cursor_position_ += region_offset_;
      if (draw_cursor_if_complete_) {
        if (origin_canvas_ != nullptr) {
          origin_canvas_->Save();
          origin_canvas_->Translate(cursor_position_.x_, cursor_position_.y_);
          typewriter_cursor_->Draw(
              origin_canvas_, cursor_position_.x_,
              cursor_position_.y_ + typewriter_cursor_->GetAscent());
          origin_canvas_->Restore();
        }
        max_draw_height_ = std::max(
            max_draw_height_,
            std::max(page.GetLayoutHeight(),
                     cursor_position_.y_ + typewriter_cursor_->GetDescent() -
                         typewriter_cursor_->GetAscent()));
      }
    }
    page_completed_ = true;
  }
}

void MarkdownTypewriterDrawer::DrawTextRegion(tttext::LayoutRegion* page,
                                              tttext::LayoutDrawer* drawer) {
  if (page != nullptr && (!terminated_ || typewriter_cursor_ == nullptr)) {
    // need draw next region or need draw cursor
    drawer->DrawLayoutPage(page);
    last_draw_region_ = page;
    region_offset_ = translate_offset_;
    if (terminated_ && typewriter_cursor_ != nullptr) {
      // adjust cursor position
      tttext::TextLine* cursor_line = nullptr;
      auto relative_position = cursor_position_ - region_offset_;
      for (uint32_t i = 0; i < page->GetLineCount(); i++) {
        auto* line = page->GetLine(i);
        if (line->GetLineTop() <= relative_position.y_ &&
            line->GetLineBottom() >= relative_position.y_) {
          cursor_line = line;
          break;
        }
      }
      relative_position = CalculateCursorPosition(
          cursor_line, relative_position, region_offset_, typewriter_cursor_);
      cursor_position_ = relative_position + region_offset_;
      if (origin_canvas_ != nullptr) {
        origin_canvas_->Save();
        typewriter_cursor_->Draw(origin_canvas_, relative_position.x_,
                                 relative_position.y_);
        origin_canvas_->Restore();
      }
      if (cursor_line != nullptr) {
        max_draw_height_ = std::max(
            max_draw_height_, cursor_line->GetLineBottom() + region_offset_.y_);
      }
      max_draw_height_ =
          std::max(max_draw_height_, cursor_position_.y_ +
                                         typewriter_cursor_->GetDescent() -
                                         typewriter_cursor_->GetAscent());
    } else {
      max_draw_height_ = std::max(
          max_draw_height_,
          region_offset_.y_ + MarkdownPlatform::GetMdLayoutRegionHeight(page));
    }
  }
}

PointF MarkdownTypewriterDrawer::CalculateCursorPosition(
    tttext::TextLine* cursor_line, lynx::markdown::PointF cursor_position,
    PointF region_offset, tttext::RunDelegate* cursor) {
  cursor->Layout();
  auto cursor_width = cursor->GetAdvance();
  auto cursor_ascent = -cursor->GetAscent();
  auto cursor_descent = cursor->GetDescent();
  if (cursor_line != nullptr) {
    if (cursor_width + cursor_position.x_ + region_offset.x_ >
        page_->GetMaxWidth()) {
      // cursor need move to next line
      cursor_position.x_ = 0;
      cursor_position.y_ = cursor_line->GetLineBottom() + cursor_ascent;
    } else {
      // apply vertical align
      MarkdownVerticalAlign align =
          style_ == nullptr ? MarkdownVerticalAlign::kBaseline
                            : style_->typewriter_cursor_.vertical_align_;
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

void MarkdownTypewriterDrawer::DrawGlyphs(const tttext::ITypefaceHelper* font,
                                          uint32_t glyph_count,
                                          const uint16_t* glyphs,
                                          const char* text, uint32_t text_bytes,
                                          float origin_x, float origin_y,
                                          float* x, float* y,
                                          tttext::Painter* painter) {
  if (!terminated_) {
    if (max_glyph_count_ < 0 ||
        max_glyph_count_ >=
            draw_glyph_count_ + static_cast<int32_t>(glyph_count)) {
      if (origin_canvas_ != nullptr) {
        origin_canvas_->DrawGlyphs(font, glyph_count, glyphs, text, text_bytes,
                                   origin_x, origin_y, x, y, painter);
      }
      draw_glyph_count_ += glyph_count;
    } else {
      uint32_t current_draw_max_count = max_glyph_count_ - draw_glyph_count_;
      if (origin_canvas_ != nullptr) {
        auto max_draw_glyph_count = std::min(
            base::UTF8IndexToCIndex(text, text_bytes, current_draw_max_count),
            (size_t)text_bytes);
        origin_canvas_->DrawGlyphs(font, current_draw_max_count, glyphs, text,
                                   max_draw_glyph_count, origin_x, origin_y, x,
                                   y, painter);
      }
      typewriter_cursor_ = LoadTypewriterCursor(painter);
      cursor_position_ =
          translate_offset_ +
          PointF{*(x + current_draw_max_count) + origin_x, origin_y};
      draw_glyph_count_ = max_glyph_count_;
      terminated_ = true;
    }
    last_draw_font_ = font;
  } else if (typewriter_cursor_ == nullptr) {
    typewriter_cursor_ = LoadTypewriterCursor(painter);
    cursor_position_ = translate_offset_ + PointF{*x + origin_x, origin_y};
  }
}

tttext::RunDelegate* MarkdownTypewriterDrawer::LoadTypewriterCursor(
    tttext::Painter* painter) {
  if (custom_typewriter_cursor_ != nullptr) {
    return custom_typewriter_cursor_;
  }

  if (!style_->typewriter_cursor_.custom_cursor_.empty() ||
      painter == nullptr) {
    default_typewriter_cursor_ =
        std::make_unique<MarkdownEmptySpaceDelegate>(0);
    return default_typewriter_cursor_.get();
  }

  auto paragraph = tttext::Paragraph::Create();
  tttext::Style style;
  style.SetFontDescriptor(
      tttext::FontDescriptor{{}, tttext::FontStyle::Normal(), 0});
  style.SetTextSize(painter->GetTextSize());
  style.SetForegroundColor(tttext::TTColor(painter->GetFillColor()));
  paragraph->AddTextRun(&style, "…");
  default_typewriter_cursor_ =
      std::make_unique<MarkdownTextDelegate>(std::move(paragraph), -1, -1);
  return default_typewriter_cursor_.get();
}

}  // namespace markdown
}  // namespace lynx
