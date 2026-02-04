// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout.h"

#include <algorithm>
#include <memory>

#include "markdown/element/markdown_run_delegates.h"
#include "markdown/element/markdown_table.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/embed/markdown_parser_embed.h"
#include "markdown/utils/markdown_platform.h"
namespace lynx {
namespace markdown {
MarkdownLayout::MarkdownLayout(MarkdownDocument* document)
    : document_(document) {}
std::pair<float, float> MarkdownLayout::Layout(float width, float height,
                                               int text_max_lines) {
  if (document_ == nullptr)
    return {0, 0};
  current_layout_bottom_ = paddings_.bottom_;
  max_width_ = width;
  max_height_ = height - paddings_.top_ - paddings_.bottom_;
  page_ = std::make_shared<MarkdownPage>();
  page_->max_width_ = width;
  page_->max_height_ = height;
  for (auto& para : document_->para_vec_) {
    if (page_->FullFilled() ||
        (text_max_lines > 0 && text_max_lines <= page_->GetLineCount())) {
      break;
    }
    Layout(para,
           text_max_lines > 0 ? (text_max_lines - page_->GetLineCount()) : -1,
           &para == &(document_->para_vec_.back()));
  }
  page_->layout_height_ += paddings_.bottom_;
  if (page_->FullFilled() && document_->event_ != nullptr &&
      !page_->regions_.empty()) {
    document_->event_->OnTextOverflow(
        page_->regions_.back()->element_->GetTextOverflow());
  }
  page_->SetElements(document_->para_vec_);
  auto& attachments = document_->border_attachments_;
  page_->SetBorderAttachments(std::move(attachments));
  if (document_->loader_ != nullptr &&
      !document_->GetStyle()
           .typewriter_cursor_.typewriter_cursor_.custom_cursor_.empty()) {
    auto custom_typewriter_cursor = document_->loader_->LoadInlineView(
        document_->style_.typewriter_cursor_.typewriter_cursor_.custom_cursor_
            .c_str(),
        width, height);
    if (custom_typewriter_cursor != nullptr) {
      page_->SetCustomTypewriterCursor(std::move(custom_typewriter_cursor));
    }
  }

  for (auto quote_range : document_->quote_range_) {
    if (quote_range.start_ < 0 ||
        quote_range.start_ >= static_cast<int>(page_->regions_.size()))
      continue;
    auto& start_para = page_->regions_[quote_range.start_];
    if (start_para->border_ == nullptr)
      continue;
    auto top = start_para->border_->rect_.GetTop();
    if (quote_range.end_ < 1 ||
        quote_range.end_ > static_cast<int>(page_->regions_.size()))
      continue;
    auto& end_para = page_->regions_[quote_range.end_ - 1];
    if (end_para->border_ == nullptr)
      continue;
    auto bottom = end_para->border_->rect_.GetBottom();
    auto border_left = document_->style_.quote_.block_.margin_left_ +
                       document_->style_.quote_.border_.border_width_ / 2;
    auto border_right = max_width_ -
                        document_->style_.quote_.block_.margin_right_ -
                        document_->style_.quote_.border_.border_width_ / 2;
    auto border = std::make_unique<MarkdownQuoteBorder>();
    border->rect_ =
        RectF(border_left, top, border_right - border_left, bottom - top);
    border->line_style_ = document_->style_.quote_border_line_;
    page_->quote_borders_.emplace_back(std::move(border));
  }
  if (!document_->inherited_scroll_state_.empty()) {
    page_->ApplyScrollState(document_->inherited_scroll_state_);
    document_->inherited_scroll_state_.clear();
  }
  document_->SetPage(page_);
  return std::make_pair(page_->GetLayoutWidth(), page_->GetLayoutHeight());
}

void MarkdownLayout::Layout(
    const std::shared_ptr<MarkdownElement>& paragraph_ptr, int max_lines,
    bool last) {
  const auto& paragraph = *paragraph_ptr;
  MarkdownBorder border = paragraph.GetBorderType();
  if (paragraph.GetBorderStyle().border_type_ == MarkdownBorderType::kNone) {
    border = MarkdownBorder::kNone;
  }
  float region_left = paddings_.left_ + paragraph.GetBlockStyle().margin_left_ +
                      paragraph.GetBlockStyle().padding_left_;
  float region_width = max_width_ - paragraph.GetBlockStyle().margin_left_ -
                       paragraph.GetBlockStyle().margin_right_ -
                       paragraph.GetBlockStyle().padding_left_ -
                       paragraph.GetBlockStyle().padding_right_ -
                       paddings_.left_ - paddings_.right_;
  if (paragraph_ptr->ScrollX()) {
    region_width = std::numeric_limits<float>::max();
  }
  float region_top =
      current_layout_bottom_ +
      std::max(current_margin_bottom_, paragraph.GetBlockStyle().margin_top_) +
      paragraph.GetBlockStyle().padding_top_;
  float region_max_height =
      max_height_ - region_top - paragraph.GetBlockStyle().padding_bottom_;
  float border_width = paragraph.GetBorderStyle().border_width_;
  float border_right_width = 0, border_bottom_width = 0, border_top_width = 0;
  if (border == MarkdownBorder::kLeft) {
    region_width -= border_width;
    region_left += border_width;
  } else if (border == MarkdownBorder::kRect) {
    region_width -= border_width * 2;
    region_left += border_width;
    region_top += border_width;
    region_max_height -= border_width * 2;
    border_right_width = border_width;
    border_bottom_width = border_width;
    border_top_width = border_width;
  } else if (border == MarkdownBorder::kTop) {
    region_top += border_width;
    region_max_height -= border_width;
    border_top_width = border_width;
  }
  if (paragraph.GetBlockStyle().max_width_ > 0) {
    region_width = std::min(region_width, paragraph.GetBlockStyle().max_width_);
  }
  auto page_region =
      LayoutElement(paragraph, max_lines, region_width, region_max_height,
                    region_left, region_top, last);
  if (page_region == nullptr) {
    if (page_->full_filled_ && !page_->regions_.empty()) {
      auto& region = page_->regions_.back();
      if (region->element_->GetTextOverflow() ==
          MarkdownTextOverflow::kEllipsis) {
        ForceAppendEllipsis(region.get());
        page_->layout_width_ = std::min(
            max_width_, std::max(page_->layout_width_,
                                 (region->border_ == nullptr
                                      ? region->rect_.GetRight()
                                      : region->border_->rect_.GetRight()) +
                                     paragraph.GetBlockStyle().margin_right_));
      }
    }
  } else {
    auto region_right = page_region->rect_.GetRight();
    auto region_bottom = page_region->rect_.GetBottom();

    float view_left = paragraph.GetBlockStyle().margin_left_;
    float view_right = max_width_ - paragraph.GetBlockStyle().margin_right_;
    float view_top = current_layout_bottom_;

    if (border != MarkdownBorder::kNone) {
      float border_left = paragraph.GetBlockStyle().margin_left_;
      float border_right = view_right;
      if (paragraph_ptr->ScrollX()) {
        float content_right = region_right +
                              paragraph.GetBlockStyle().padding_right_ +
                              border_right_width;
        border_right = std::max(content_right, view_right);
      }
      float border_top = region_top - paragraph.GetBlockStyle().padding_top_ -
                         border_top_width;
      float border_bottom = region_bottom +
                            paragraph.GetBlockStyle().padding_bottom_ +
                            border_bottom_width;
      page_region->border_ = std::make_unique<MarkdownPageRegionBorder>();
      page_region->border_->rect_ =
          RectF::MakeLTRB(border_left, border_top, border_right, border_bottom);
      page_region->border_->border_style_ = paragraph.GetBorderStyle();
      page_region->border_->border_ = border;
      region_right = border_right;
    }

    current_layout_bottom_ = region_bottom;
    if (!page_->full_filled_ ||
        paragraph.GetTextOverflow() != MarkdownTextOverflow::kClip) {
      current_layout_bottom_ +=
          paragraph.GetBlockStyle().padding_bottom_ + border_bottom_width;
    }
    current_margin_bottom_ = paragraph.GetBlockStyle().margin_bottom_;
    page_->layout_height_ = std::min(
        max_height_, std::max(page_->layout_height_, current_layout_bottom_));
    if (paragraph_ptr->ScrollX() && region_right > view_right) {
      page_region->scroll_x_ = true;
      page_region->scroll_x_offset_ = 0;
      page_region->scroll_x_view_rect_ = RectF::MakeLTRB(
          view_left, view_top, view_right, current_layout_bottom_);
      page_->layout_width_ = max_width_;
    } else {
      page_->layout_width_ = std::min(
          max_width_,
          std::max(page_->layout_width_,
                   region_right + paragraph.GetBlockStyle().margin_right_) +
              paddings_.right_);
    }
    current_layout_bottom_ += paragraph.GetSpaceAfter();
    page_region->element_ = paragraph_ptr;
    page_->regions_.emplace_back(std::move(page_region));
  }
}

std::unique_ptr<MarkdownPageRegion> MarkdownLayout::LayoutElement(
    const lynx::markdown::MarkdownElement& paragraph, int max_lines,
    float region_width, float region_max_height, float region_left,
    float region_top, bool last) {
  if (region_max_height <= 0 || max_lines == 0) {
    page_->full_filled_ = true;
    return nullptr;
  }
  float region_bottom = 0;
  float region_right = 0;
  std::unique_ptr<MarkdownPageRegion> page_region = nullptr;
  if (paragraph.GetType() == MarkdownElementType::kParagraph) {
    auto* para_element =
        reinterpret_cast<const MarkdownParagraphElement*>(&paragraph);
    auto width_mode = (para_element->GetParagraph()
                           ->GetParagraphStyle()
                           .GetHorizontalAlign() ==
                       tttext::ParagraphHorizontalAlignment::kLeft)
                          ? tttext::LayoutMode::kAtMost
                          : tttext::LayoutMode::kDefinite;
    auto region = LayoutParagraph(para_element->GetParagraph(), region_width,
                                  width_mode, region_max_height, max_lines,
                                  paragraph.GetTextOverflow(),
                                  &page_->full_filled_, last);
    if (region != nullptr) {
      if (para_element->GetLastLineAlign() != MarkdownTextAlign::kUndefined) {
        const auto align = MarkdownParserEmbed::ConvertTextAlign(
            para_element->GetLastLineAlign());
        for (uint32_t i = 0; i < region->GetLineCount(); i++) {
          auto* line = region->GetLine(i);
          if (line->IsLastLineOfParagraph()) {
            line->ModifyHorizontalAlignment(align);
          }
        }
      }
      region_bottom =
          region_top + MarkdownPlatform::GetMdLayoutRegionHeight(region.get());
      region_right =
          region_left + MarkdownPlatform::GetMdLayoutRegionWidth(region.get());
      page_->line_count_ += region->GetLineCount();
      if (max_lines > 0 &&
          (static_cast<int32_t>(region->GetLineCount()) > max_lines)) {
        page_->full_filled_ = true;
        if (paragraph.GetTextOverflow() == MarkdownTextOverflow::kClip) {
          region_bottom =
              region_top + region->GetLine(max_lines - 1)->GetLineBottom();
        }
      }
      auto para_region = std::make_unique<MarkdownPageParagraphRegion>();
      para_region->region_ = std::move(region);
      page_region = std::move(para_region);
    }
  } else if (paragraph.GetType() == MarkdownElementType::kTable) {
    auto table_element =
        reinterpret_cast<const MarkdownTableElement*>(&paragraph);
    auto table =
        LayoutTable(table_element->GetTable(), region_width, region_max_height,
                    table_element->GetBlockStyle().min_width_, max_lines,
                    paragraph.GetTextOverflow(), &page_->full_filled_);
    region_bottom = region_top + table->total_height_;
    region_right = region_left + table->total_width_;
    page_->line_count_ += table->GetRowCount();
    if (max_lines > 0 && table->GetRowCount() > max_lines) {
      page_->full_filled_ = true;
      region_bottom =
          region_top + table->GetCell(max_lines - 1, 0).cell_rect_.GetBottom();
    }
    auto table_region = std::make_unique<MarkdownPageTableRegion>();
    table_region->table_ = std::move(table);
    page_region = std::move(table_region);
  } else if (paragraph.GetType() == MarkdownElementType::kNone ||
             paragraph.GetType() == MarkdownElementType::kBlock) {
    page_region = std::make_unique<MarkdownPageRegion>();
    region_right = region_left + region_width;
    region_bottom = region_top;
  }
  if (page_region == nullptr) {
    return nullptr;
  }
  page_region->rect_ =
      RectF::MakeLTRB(region_left, region_top, region_right, region_bottom);
  return page_region;
}

std::unique_ptr<MarkdownTableRegion> MarkdownLayout::LayoutTable(
    lynx::markdown::MarkdownTable* table, float width, float height,
    float min_width, int max_lines, MarkdownTextOverflow overflow,
    bool* full_filled) {
  if (table->Empty()) {
    return nullptr;
  }
  int rows = table->GetRowCount(), columns = table->GetColumnCount();
  auto table_region = std::make_unique<MarkdownTableRegion>(rows, columns);
  float cell_max_width = table->cell_block_style_.max_width_;
  if (cell_max_width <= 0) {
    cell_max_width = std::numeric_limits<float>::max();
  }
  // pre-layout
  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      table_region->SetCell(
          row, column,
          MarkdownTableRegionCell{
              LayoutParagraph(table->GetCell(row, column).paragraph_.get(),
                              width, tttext::LayoutMode::kAtMost,
                              std::numeric_limits<float>::max(), -1, overflow,
                              nullptr, false),
              {},
              {}});
    }
  }

  // pre-calc column max width
  std::vector<float> column_max_width(table->GetColumnCount());
  for (int column = 0; column < columns; column++) {
    float max_width = 0;
    for (int row = 0; row < rows; row++) {
      auto& cell = table_region->GetCell(row, column);
      max_width = std::max(max_width, cell.GetRegionWidth());
    }
    column_max_width[column] = max_width;
  }
  auto column_max_width_copy = column_max_width;
  std::sort(column_max_width.begin(), column_max_width.end());

  // calc max region width
  uint32_t large_column_start_index = 0;
  auto& cell_block = table->cell_block_style_;
  float row_empty_space =
      (cell_block.padding_left_ + cell_block.padding_right_) * columns;
  float rest_width = width - row_empty_space;
  for (; large_column_start_index < column_max_width.size();
       large_column_start_index++) {
    if (column_max_width[large_column_start_index] >
        (rest_width / (column_max_width.size() - large_column_start_index))) {
      break;
    } else {
      rest_width -= column_max_width[large_column_start_index];
    }
  }
  float max_region_width = std::numeric_limits<float>::max();
  if (large_column_start_index < column_max_width.size()) {
    max_region_width =
        rest_width / (column_max_width.size() - large_column_start_index);
  }
  max_region_width = std::min(max_region_width, cell_max_width);

  // cal cell position and align region
  float total_width = 0;
  column_max_width.swap(column_max_width_copy);
  for (int column = 0; column < columns; column++) {
    column_max_width[column] =
        std::min(max_region_width, column_max_width[column]);
    total_width += column_max_width[column];
  }

  // apply min width
  min_width -= row_empty_space;
  min_width = std::min(min_width, width - row_empty_space);
  if (min_width > 0 && total_width < min_width) {
    const float extra = (min_width - total_width) / columns;
    for (int column = 0; column < columns; column++) {
      column_max_width[column] += extra;
    }
  }
  // re-layout regions which width larger than max region width
  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      float column_width = column_max_width[column];
      auto& cell = table_region->GetCell(row, column);
      cell.region_ = LayoutParagraph(
          table->GetCell(row, column).paragraph_.get(), std::ceil(column_width),
          tttext::LayoutMode::kDefinite, std::numeric_limits<float>::max(), -1,
          overflow, nullptr, false);
    }
  }

  float row_y_offset = 0;
  float column_x_offset = 0;
  for (int row = 0; row < rows; row++) {
    float row_height = 0;
    for (int column = 0; column < columns; column++) {
      auto& cell = table_region->GetCell(row, column);
      row_height = std::max(row_height, cell.GetRegionHeight());
    }
    column_x_offset = 0;
    for (int column = 0; column < columns; column++) {
      float column_width = column_max_width[column];
      auto& cell = table_region->GetCell(row, column);
      auto& content = table->GetCell(row, column);
      cell.cell_rect_ = RectF::MakeLTWH(
          column_x_offset, row_y_offset,
          column_width + cell_block.padding_left_ + cell_block.padding_right_,
          row_height + cell_block.padding_top_ + cell_block.padding_bottom_);
      float x_offset = 0;
      if (content.alignment_ == tttext::ParagraphHorizontalAlignment::kCenter) {
        x_offset = (column_width - cell.GetRegionWidth()) / 2;
      } else if (content.alignment_ ==
                 tttext::ParagraphHorizontalAlignment::kRight) {
        x_offset = (column_width - cell.GetRegionWidth());
      }
      float y_offset = 0;
      if (content.vertical_alignment_ == MarkdownVerticalAlign::kCenter) {
        y_offset = (row_height - cell.GetRegionHeight()) / 2;
      } else if (content.vertical_alignment_ ==
                 MarkdownVerticalAlign::kBottom) {
        y_offset = (row_height - cell.GetRegionHeight());
      }
      cell.region_offset_ = cell.region_ == nullptr
                                ? PointF{}
                                : PointF{cell_block.padding_left_ + x_offset,
                                         cell_block.padding_top_ + y_offset};
      column_x_offset +=
          column_width + cell_block.padding_left_ + cell_block.padding_right_;
    }
    row_y_offset +=
        row_height + cell_block.padding_top_ + cell_block.padding_bottom_;
  }
  table_region->total_width_ =
      table_region->GetCell(0, columns - 1).cell_rect_.GetRight();
  table_region->total_height_ =
      table_region->GetCell(rows - 1, 0).cell_rect_.GetBottom();
  if (table_region->total_height_ > height && full_filled != nullptr) {
    *full_filled = true;
  }
  return table_region;
}

std::unique_ptr<tttext::LayoutRegion> MarkdownLayout::LayoutParagraph(
    tttext::Paragraph* paragraph, float width, tttext::LayoutMode width_mode,
    float height, int max_lines, MarkdownTextOverflow overflow,
    bool* full_filled, bool last) {
  if (paragraph == nullptr || paragraph->GetCharCount() == 0) {
    return nullptr;
  }
  tttext::TextLayout& text_layout = *MarkdownPlatform::GetTextLayout();
  tttext::TTTextContext context;
  context.SetLastLineCanOverflow(overflow == MarkdownTextOverflow::kClip);
  auto region = std::make_unique<tttext::LayoutRegion>(
      width, height, width_mode, tttext::LayoutMode::kAtMost);
  uint32_t current_para_max_lines =
      static_cast<int>(paragraph->GetParagraphStyle().GetMaxLines());
  if (max_lines >= 0 &&
      static_cast<uint32_t>(max_lines) < current_para_max_lines) {
    paragraph->GetParagraphStyle().SetMaxLines(max_lines);
  }
  tttext::LayoutResult result =
      text_layout.LayoutEx(paragraph, region.get(), context);
  paragraph->GetParagraphStyle().SetMaxLines(current_para_max_lines);
  bool para_full_layout = true;
  if (result == tttext::LayoutResult::kBreakPage &&
      paragraph->GetCharCount() > 0 && !region->IsEmpty()) {
    auto last_line = region->GetLine(region->GetLineCount() - 1);
    if (last_line->GetParagraph() != paragraph ||
        last_line->GetEndCharPos() < paragraph->GetCharCount()) {
      para_full_layout = false;
    }
  }
  if (result == tttext::LayoutResult::kBreakPage &&
      region->GetLineCount() < current_para_max_lines) {
    if (!para_full_layout || !last) {
      if (full_filled != nullptr) {
        *full_filled = true;
      }
    }
    if (!last && region->GetLineCount() > 0 && para_full_layout) {
      auto* line = region->GetLine(region->GetLineCount() - 1);
      line->StripByEllipsis(nullptr);
      region->UpdateLayoutedSize(line, context);
    }
  }
  if (region->GetLineCount() == 0 ||
      (MarkdownPlatform::GetMdLayoutRegionHeight(region.get()) > height &&
       overflow == MarkdownTextOverflow::kEllipsis)) {
    if (full_filled != nullptr) {
      *full_filled = true;
    }
    return nullptr;
  } else {
    return region;
  }
}

std::pair<float, float> MarkdownLayout::MeasureParagraph(
    tttext::Paragraph* paragraph_ptr, float width, float height,
    int max_lines) {
  auto region =
      LayoutParagraph(paragraph_ptr, width, tttext::LayoutMode::kAtMost, height,
                      max_lines, MarkdownTextOverflow::kClip, nullptr, false);
  return {MarkdownPlatform::GetMdLayoutRegionWidth(region.get()),
          MarkdownPlatform::GetMdLayoutRegionHeight(region.get())};
}

void MarkdownLayout::ForceAppendEllipsis(MarkdownPageRegion* region) {
  if (region->element_->GetTextOverflow() == MarkdownTextOverflow::kEllipsis) {
    if (region->element_->GetType() == MarkdownElementType::kParagraph) {
      auto* para_region =
          reinterpret_cast<MarkdownPageParagraphRegion*>(region);
      if (para_region->region_->GetLineCount() != 0) {
        auto width_before = MarkdownPlatform::GetMdLayoutRegionWidth(
            para_region->region_.get());
        auto* last_line = para_region->region_->GetLine(
            para_region->region_->GetLineCount() - 1);
        last_line->StripByEllipsis(nullptr);
        para_region->region_->UpdateLayoutedSize(last_line,
                                                 tttext::TTTextContext());
        auto width_delta = MarkdownPlatform::GetMdLayoutRegionWidth(
                               para_region->region_.get()) -
                           width_before;
        if (width_delta > 0) {
          para_region->rect_.SetRight(para_region->rect_.GetRight() +
                                      width_delta);
          if (para_region->border_ != nullptr) {
            para_region->border_->rect_.SetRight(
                para_region->border_->rect_.GetRight() + width_delta);
          }
        }
      }
    }
  }
}

void MarkdownLayout::SetPaddings(Paddings paddings) {
  paddings_ = paddings;
}

}  // namespace markdown
}  // namespace lynx
