// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_selection.h"

#include <limits>

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_table.h"
namespace lynx {
namespace markdown {
Range MarkdownSelection::GetCharRangeByPoint(
    const lynx::markdown::MarkdownPage* page, PointF point,
    CharRangeType type) {
  auto region_index = FindClosestRegionIndex(page, point.y_);
  auto& region = page->regions_[region_index];
  auto line_index =
      FindClosestMarkdownRegionLineIndexOrRowIndex(region.get(), point.y_);
  auto char_index = FindClosestMarkdownRegionCharIndexOrColumnIndex(
      region.get(), line_index, point.x_);
  Range range{0, 0};
  if (region->element_->GetType() == MarkdownElementType::kParagraph) {
    auto* content =
        reinterpret_cast<MarkdownParagraphElement*>(region->element_.get());
    auto* para_region =
        reinterpret_cast<MarkdownPageParagraphRegion*>(region.get());
    if (type == CharRangeType::kChar || type == CharRangeType::kWord) {
      auto char_pos =
          content->GetCharStart() +
          para_region->region_->GetLine(line_index)->GetStartCharPos() +
          char_index;
      range.start_ = char_pos;
      range.end_ = char_pos + 1;
    } else if (type == CharRangeType::kParagraph) {
      range.start_ = static_cast<int32_t>(content->GetCharStart());
      range.end_ = static_cast<int32_t>(content->GetCharStart() +
                                        content->GetCharCount());
    } else if (type == CharRangeType::kSentence) {
      auto pos_in_para =
          para_region->region_->GetLine(line_index)->GetStartCharPos() +
          char_index;
      range = GetSentenceOfChar(content->GetParagraph(), pos_in_para);
      range.start_ += static_cast<int32_t>(content->GetCharStart());
      range.end_ += static_cast<int32_t>(content->GetCharStart());
    }
  } else if (region->element_->GetType() == MarkdownElementType::kTable) {
    auto* table = reinterpret_cast<MarkdownPageTableRegion*>(region.get());
    auto* content =
        reinterpret_cast<MarkdownTableElement*>(region->element_.get());
    auto [cell_line_index, cell_char_index] =
        FindCellLineAndCharIndex(table, line_index, char_index, point);
    auto& cell = content->GetTable()->GetCell(line_index, char_index);
    auto& region_cell = table->table_->GetCell(line_index, char_index);
    if (cell.paragraph_ == nullptr || region_cell.region_ == nullptr) {
      range.start_ = static_cast<int32_t>(cell.char_start_);
      range.end_ = range.start_ + 1;
    } else {
      if (type == CharRangeType::kChar || type == CharRangeType::kWord) {
        auto char_pos =
            content->GetCharStart() + cell.char_start_ +
            region_cell.region_->GetLine(cell_line_index)->GetStartCharPos() +
            cell_char_index;
        range.start_ = static_cast<int32_t>(char_pos);
        range.end_ = static_cast<int32_t>(char_pos + 1);
      } else if (type == CharRangeType::kParagraph) {
        range.start_ =
            static_cast<int32_t>(content->GetCharStart() + cell.char_start_);
        range.end_ = static_cast<int32_t>(content->GetCharStart() +
                                          cell.char_start_ + cell.char_count_);
      } else if (type == CharRangeType::kSentence) {
        auto pos_in_para =
            region_cell.region_->GetLine(cell_line_index)->GetStartCharPos() +
            cell_char_index;
        range = GetSentenceOfChar(cell.paragraph_.get(), pos_in_para);
        range.start_ +=
            static_cast<int32_t>(content->GetCharStart() + cell.char_start_);
        range.end_ +=
            static_cast<int32_t>(content->GetCharStart() + cell.char_start_);
      }
    }
  }
  return range;
}

int MarkdownSelection::FindClosestRegionIndex(
    const lynx::markdown::MarkdownPage* page, float y) {
  auto lower_iter = std::lower_bound(
      page->regions_.begin(), page->regions_.end(), y,
      [](const std::unique_ptr<MarkdownPageRegion>& region, float y) -> bool {
        return region->rect_.GetTop() < y;
      });
  int index = lower_iter - page->regions_.begin();
  index = (index == 0 ? 0 : index - 1);
  while (index < (static_cast<int32_t>(page->regions_.size()) - 1) &&
         page->regions_[index]->element_->GetType() !=
             MarkdownElementType::kParagraph &&
         page->regions_[index]->element_->GetType() !=
             MarkdownElementType::kTable) {
    index++;
  }
  if (index == static_cast<int32_t>(page->regions_.size()) - 1) {
    return index;
  }
  auto& region = page->regions_[index];
  float region_bottom = region->rect_.GetBottom();
  if (region_bottom >= y) {
    return index;
  }
  auto& region_next = page->regions_[index + 1];
  if (region_next->element_->GetType() != MarkdownElementType::kParagraph &&
      region_next->element_->GetType() != MarkdownElementType::kTable) {
    return index;
  }
  float region_next_top = region_next->rect_.GetTop();
  if ((y - region_bottom) < (region_next_top - y)) {
    return index;
  } else {
    return index + 1;
  }
}

int MarkdownSelection::FindRegionLineIndex(tttext::LayoutRegion* region,
                                           float y) {
  int start = 0, len = region->GetLineCount(), mid = 0;
  while (len != 0) {
    int half_len = len / 2;
    mid = start + half_len;
    auto* line = region->GetLine(mid);
    float line_top = line->GetLineTop();
    float line_bottom = line->GetLineBottom();
    if (line_bottom < y) {
      start = ++mid;
      len -= half_len + 1;
    } else if (line_top > y) {
      len = half_len;
    } else {
      return mid;
    }
  }
  if (mid > 0) {
    mid -= 1;
  }
  if (mid == static_cast<int32_t>(region->GetLineCount()) - 1) {
    return mid;
  } else {
    auto line_bottom = region->GetLine(mid)->GetLineBottom();
    auto next_top = region->GetLine(mid + 1)->GetLineTop();
    if ((y - line_bottom) < (next_top - y)) {
      return mid;
    } else {
      return mid + 1;
    }
  }
}

int MarkdownSelection::FindTableRowIndex(
    lynx::markdown::MarkdownTableRegion* table, float y) {
  auto iter = std::lower_bound(
      table->cells_.begin(), table->cells_.end(), y,
      [](const std::vector<MarkdownTableRegionCell>& var, float y) -> bool {
        return var.front().cell_rect_.GetTop() < y;
      });
  auto index = iter - table->cells_.begin();
  return index == 0 ? 0 : index - 1;
}

int MarkdownSelection::FindTableColumnIndex(MarkdownTableRegion* table,
                                            float x) {
  auto iter = std::lower_bound(
      table->cells_.front().begin(), table->cells_.front().end(), x,
      [](const MarkdownTableRegionCell& var, float x) -> bool {
        return var.cell_rect_.GetLeft() < x;
      });
  auto index = iter - table->cells_.front().begin();
  return index == 0 ? 0 : index - 1;
}

std::vector<RectF> MarkdownSelection::GetSelectionRectByCharPos(
    const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end,
    RectType type, RectCoordinate coordinate) {
  std::vector<RectF> rect_vec;
  for (auto& region : page->regions_) {
    auto region_start = static_cast<int32_t>(region->element_->GetCharStart());
    auto region_count = region->element_->GetCharCount();
    int32_t region_end = region_start + region_count;
    if (region_end < char_pos_start) {
      continue;
    } else if (static_cast<int32_t>(region->element_->GetCharStart()) >=
               char_pos_end) {
      break;
    } else {
      GetPageRegionSelectionRectByCharPos(
          region.get(), char_pos_start - region_start,
          char_pos_end - region_start, &rect_vec,
          PointF{region->rect_.GetLeft(), region->rect_.GetTop()}, type,
          coordinate);
    }
  }
  return rect_vec;
}

RectF MarkdownSelection::GetSelectionClosedRectByCharPos(
    const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end,
    RectType type, RectCoordinate coordinate) {
  std::vector<RectF> rect_vec = GetSelectionRectByCharPos(
      page, char_pos_start, char_pos_end, type, coordinate);
  RectF ret;
  if (rect_vec.empty()) {
    return ret;
  }
  ret = rect_vec[0];
  for (size_t i = 1; i < rect_vec.size(); i++) {
    ret.SetLeft(std::min(ret.GetLeft(), rect_vec[i].GetLeft()));
    ret.SetTop(std::min(ret.GetTop(), rect_vec[i].GetTop()));
    ret.SetRight(std::max(ret.GetRight(), rect_vec[i].GetRight()));
    ret.SetBottom(std::max(ret.GetBottom(), rect_vec[i].GetBottom()));
  }

  return ret;
}

int MarkdownSelection::GetPageCharCount(const MarkdownPage* page) {
  if (page->regions_.empty()) {
    return 0;
  }
  auto last_region = page->regions_.back().get();
  auto last_element = last_region->element_.get();
  int char_count = last_element->GetCharCount();
  if (last_element->GetType() == MarkdownElementType::kParagraph) {
    auto layout_region =
        static_cast<MarkdownPageParagraphRegion*>(last_region)->region_.get();
    auto last_line = layout_region->GetLine(layout_region->GetLineCount() - 1);
    char_count = last_line->GetEndCharPos();
  }
  return last_element->GetCharStart() + char_count;
}

uint32_t MarkdownSelection::TypewriterStepToChar(const MarkdownPage* page,
                                                 uint32_t typewriter_step) {
  uint32_t typewriter_char_end = typewriter_step;
  for (auto [char_offset, step_offset] : page->GetTypewriterStepOffset()) {
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

  return typewriter_char_end;
}

void MarkdownSelection::GetPageRegionSelectionRectByCharPos(
    lynx::markdown::MarkdownPageRegion* region, int32_t char_pos_start,
    int32_t char_pos_end, std::vector<RectF>* rect_ptr, PointF offset,
    RectType type, RectCoordinate coordinate) {
  auto clip_rect = RectF::MakeLTRB(
      -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  if (region->scroll_x_) {
    offset.x_ += region->scroll_x_offset_;
    if (type == RectType::kSelection) {
      clip_rect = region->scroll_x_view_rect_;
    }
  }
  if (region->element_->GetType() == MarkdownElementType::kParagraph) {
    GetLayoutRegionSelectionRectByCharPos(
        reinterpret_cast<MarkdownPageParagraphRegion*>(region)->region_.get(),
        char_pos_start, char_pos_end, rect_ptr, offset, type, coordinate,
        clip_rect);
  } else if (region->element_->GetType() == MarkdownElementType::kTable) {
    auto table = reinterpret_cast<MarkdownPageTableRegion*>(region);
    GetTableSelectionRectByCharPos(
        reinterpret_cast<MarkdownTableElement*>(table->element_.get())
            ->GetTable(),
        table->table_.get(), char_pos_start, char_pos_end, rect_ptr, offset,
        type, coordinate, clip_rect);
  }
}

void MarkdownSelection::GetLayoutRegionSelectionRectByCharPos(
    tttext::LayoutRegion* region, int32_t char_pos_start, int32_t char_pos_end,
    std::vector<RectF>* rect_ptr, PointF offset, RectType type,
    RectCoordinate coordinate, RectF clip_rect) {
  if (region == nullptr || region->IsEmpty()) return;
  auto& rect_vec = *rect_ptr;
  if (coordinate == RectCoordinate::kRelative) {
    clip_rect = RectF::MakeLTWH(clip_rect.GetLeft() - offset.x_,
                                clip_rect.GetTop() - offset.y_,
                                clip_rect.GetWidth(), clip_rect.GetHeight());
    offset = {0, 0};
  }
  for (uint32_t i = 0; i < region->GetLineCount(); i++) {
    auto* line = region->GetLine(i);
    if (static_cast<int32_t>(line->GetStartCharPos()) >= char_pos_end) {
      break;
    } else if (static_cast<int32_t>(line->GetEndCharPos()) <= char_pos_start) {
      continue;
    } else {
      float rect[4]{0, 0, 0, 0};
      auto& [left, top, width, height] = rect;
      int32_t char_start =
          std::max(char_pos_start, (int32_t)line->GetStartCharPos());
      int32_t char_end = std::min(char_pos_end, (int32_t)line->GetEndCharPos());
      line->GetBoundingRectByCharRange(rect, char_start, char_end);
      if (width > 0 && height > 0) {
        if (type == RectType::kSelection) {
          rect_vec.emplace_back(ClipRect(
              RectF::MakeLTRB(left + offset.x_, line->GetLineTop() + offset.y_,
                              left + width + offset.x_,
                              line->GetLineBottom() + offset.y_),
              clip_rect));
        } else if (type == RectType::kCharBounding) {
          rect_vec.emplace_back(ClipRect(
              RectF::MakeLTWH(left + offset.x_, top + offset.y_, width, height),
              clip_rect));
        }
      }
    }
  }
}

void MarkdownSelection::GetTableSelectionRectByCharPos(
    lynx::markdown::MarkdownTable* content,
    lynx::markdown::MarkdownTableRegion* table, int32_t char_pos_start,
    int32_t char_pos_end, std::vector<RectF>* rect_ptr, PointF offset,
    RectType type, RectCoordinate coordinate, RectF clip_rect) {
  for (int row = 0; row < table->GetRowCount(); row++) {
    for (int col = 0; col < table->GetColumnCount(); col++) {
      auto& cell = content->GetCell(row, col);
      auto& region_cell = table->GetCell(row, col);
      if (cell.paragraph_ != nullptr) {
        if (static_cast<int32_t>(cell.char_start_) >= char_pos_end) {
          break;
        } else if (static_cast<int32_t>(cell.char_start_ + cell.char_count_) <
                   char_pos_start) {
          continue;
        } else {
          GetLayoutRegionSelectionRectByCharPos(
              region_cell.region_.get(), char_pos_start - cell.char_start_,
              char_pos_end - cell.char_start_, rect_ptr,
              offset +
                  PointF{region_cell.cell_rect_.GetLeft(),
                         region_cell.cell_rect_.GetTop()} +
                  region_cell.region_offset_,
              type, coordinate, clip_rect);
        }
      }
    }
  }
}

std::string MarkdownSelection::GetContentByCharPos(
    const lynx::markdown::MarkdownPage* page, int32_t char_pos_start,
    int32_t char_pos_end,
    std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings) {
  std::string content;
  for (auto& region : page->regions_) {
    if (static_cast<int32_t>(region->element_->GetCharStart() +
                             region->element_->GetCharCount()) <
        char_pos_start) {
      continue;
    } else if (static_cast<int32_t>(region->element_->GetCharStart()) >=
               char_pos_end) {
      break;
    } else {
      GetPageRegionContentByCharPos(
          region.get(), char_pos_start - region->element_->GetCharStart(),
          char_pos_end - region->element_->GetCharStart(), &content,
          inline_element_alt_strings, region->element_->GetCharStart());
    }
  }
  return content;
}

void MarkdownSelection::GetPageRegionContentByCharPos(
    lynx::markdown::MarkdownPageRegion* region, int32_t char_pos_start,
    int32_t char_pos_end, std::string* content,
    std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
    int32_t char_offset) {
  if (region->element_->GetType() == MarkdownElementType::kParagraph) {
    GetLayoutRegionContentByCharPos(
        reinterpret_cast<MarkdownPageParagraphRegion*>(region)->region_.get(),
        char_pos_start, char_pos_end, content, inline_element_alt_strings,
        char_offset, false);
  } else if (region->element_->GetType() == MarkdownElementType::kTable) {
    auto table = reinterpret_cast<MarkdownPageTableRegion*>(region);
    GetTableContentByCharPos(
        reinterpret_cast<MarkdownTableElement*>(table->element_.get())
            ->GetTable(),
        table->table_.get(), char_pos_start, char_pos_end, content,
        inline_element_alt_strings, char_offset);
  }
}

void MarkdownSelection::GetLayoutRegionContentByCharPos(
    tttext::LayoutRegion* region, int32_t char_pos_start, int32_t char_pos_end,
    std::string* content,
    std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
    int32_t char_offset, bool need_add_space) {
  if (region != nullptr && !region->IsEmpty()) {
    auto& para = *(region->GetLine(0)->GetParagraph());
    char_pos_start = std::max(0, char_pos_start);
    char_pos_end = std::min(char_pos_end, (int32_t)para.GetCharCount());
    int32_t char_count = char_pos_end - char_pos_start;
    auto piece =
        para.GetContentString(char_pos_start, char_pos_end - char_pos_start);

    std::string piece_content;
    int32_t current_position = 0;
    for (const auto& alt : *inline_element_alt_strings) {
      int32_t inline_view_char_offset = static_cast<int32_t>(alt.first);
      if (inline_view_char_offset >= char_pos_end + char_offset) {
        break;
      }
      if (inline_view_char_offset >= char_pos_start + char_offset &&
          inline_view_char_offset < char_pos_end + char_offset) {
        int32_t inline_view_pos =
            inline_view_char_offset - char_pos_start - char_offset;
        auto append_piece =
            para.GetContentString(current_position + char_pos_start,
                                  inline_view_pos - current_position);
        piece_content.append(append_piece);
        piece_content.append(alt.second);
        current_position = inline_view_pos + 1;
      }
    }
    if (char_count > current_position) {
      auto remain_piece = para.GetContentString(
          current_position + char_pos_start, char_count - current_position);
      piece_content.append(remain_piece);
    }
    (*content).append(piece_content);

    if (char_pos_end >= static_cast<int32_t>(para.GetCharCount())) {
      (*content).append(need_add_space ? " " : "\n");
    }
  }
}

void MarkdownSelection::GetTableContentByCharPos(
    MarkdownTable* table, MarkdownTableRegion* region, int32_t char_pos_start,
    int32_t char_pos_end, std::string* content,
    std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
    int32_t char_offset) {
  for (int row = 0; row < table->GetRowCount(); row++) {
    for (int col = 0; col < table->GetColumnCount(); col++) {
      auto& cell = table->GetCell(row, col);
      auto& region_cell = region->GetCell(row, col);
      if (region_cell.region_ != nullptr) {
        if (static_cast<int32_t>(cell.char_start_) >= char_pos_end) {
          break;
        } else if (static_cast<int32_t>(cell.char_start_ + cell.char_count_) <
                   char_pos_start) {
          continue;
        } else {
          GetLayoutRegionContentByCharPos(
              region_cell.region_.get(), char_pos_start - cell.char_start_,
              char_pos_end - cell.char_start_, content,
              inline_element_alt_strings, char_offset + cell.char_start_,
              col + 1 != table->GetColumnCount());
        }
      }
    }
  }
}

std::vector<MarkdownSelectionRegion>
MarkdownSelection::GetSelectionRegionsByCharRange(
    lynx::markdown::MarkdownPage* page, int32_t char_pos_start,
    int32_t char_pos_end) {
  auto iter_begin = std::lower_bound(
      page->regions_.begin(), page->regions_.end(), char_pos_start,
      [](const std::unique_ptr<MarkdownPageRegion>& region_l,
         int32_t char_pos) -> bool {
        return static_cast<int32_t>(region_l->element_->GetCharStart() +
                                    region_l->element_->GetCharCount()) <
               char_pos;
      });
  if (iter_begin == page->regions_.end()) {
    return {};
  }
  std::vector<MarkdownSelectionRegion> selection_regions;
  for (; iter_begin != page->regions_.end(); iter_begin++) {
    auto& page_region = *iter_begin;
    if (static_cast<int32_t>((*iter_begin)->element_->GetCharStart()) >=
        char_pos_end) {
      break;
    }
    if (page_region->element_->GetType() == MarkdownElementType::kParagraph) {
      MarkdownSelectionRegion region;
      region.region_ =
          reinterpret_cast<MarkdownPageParagraphRegion*>(page_region.get())
              ->region_.get();
      region.char_pos_offset_ = page_region->element_->GetCharStart();
      region.char_count_ = page_region->element_->GetCharCount();
      region.offset_ =
          PointF{page_region->rect_.GetLeft(), page_region->rect_.GetTop()};
      selection_regions.emplace_back(std::move(region));
    } else if (page_region->element_->GetType() ==
               MarkdownElementType::kTable) {
      auto* table =
          reinterpret_cast<MarkdownPageTableRegion*>(page_region.get())
              ->table_.get();
      auto* content =
          reinterpret_cast<MarkdownTableElement*>(page_region->element_.get())
              ->GetTable();
      for (int row = 0; row < table->GetRowCount(); row++) {
        for (int col = 0; col < table->GetColumnCount(); col++) {
          auto& cell = content->GetCell(row, col);
          auto& region_cell = table->GetCell(row, col);
          if (region_cell.region_ != nullptr) {
            if (static_cast<int32_t>(cell.char_start_) >= char_pos_end) {
              break;
            } else if (static_cast<int32_t>(cell.char_start_ +
                                            cell.char_count_) <=
                       char_pos_start) {
              continue;
            } else {
              MarkdownSelectionRegion region;
              region.region_ = region_cell.region_.get();
              region.char_pos_offset_ = cell.char_start_;
              region.char_count_ = cell.char_count_;
              region.offset_ = PointF{region_cell.cell_rect_.GetLeft(),
                                      region_cell.cell_rect_.GetTop()} +
                               region_cell.region_offset_ +
                               PointF{page_region->rect_.GetLeft(),
                                      page_region->rect_.GetTop()};
              selection_regions.emplace_back(std::move(region));
            }
          }
        }
      }
    }
  }
  return selection_regions;
}

Range MarkdownSelection::GetSentenceOfChar(tttext::Paragraph* paragraph,
                                           int32_t char_pos) {
  auto content_string =
      paragraph->GetContentString(0, paragraph->GetCharCount());
  auto byte_pos_start = base::UTF8IndexToCIndex(
      content_string.data(), content_string.length(), char_pos);
  size_t sentence_start = std::string::npos, sentence_end = std::string::npos,
         sentence_end_length = 0;
  for (auto& patten : SentenceEndPattens()) {
    // match before
    auto sentence_before_end = content_string.rfind(patten, byte_pos_start);
    if (sentence_before_end != std::string::npos) {
      if (sentence_before_end + patten.length() >= byte_pos_start) {
        // current patten range contains char pos, matched current sentence end
        sentence_end = sentence_before_end;
        sentence_end_length = patten.length();
      } else {
        // sentence before matched
        sentence_start = sentence_before_end + patten.length();
      }
      break;
    }
  }
  if (sentence_start == std::string::npos &&
      sentence_end == std::string::npos) {
    // no patten matched before char pos, sentence starts at pos 0
    sentence_start = 0;
  }
  if (sentence_start == std::string::npos) {
    if (sentence_end == 0) {
      sentence_start = 0;
    } else {
      for (auto& patten : SentenceEndPattens()) {
        // match before
        auto sentence_before_end =
            content_string.rfind(patten, sentence_end - 1);
        if (sentence_before_end != std::string::npos) {
          // sentence before matched
          sentence_start = sentence_before_end + patten.length();
          break;
        }
      }
    }
    if (sentence_start == std::string::npos) {
      // no patten matched before sentence end
      sentence_start = 0;
    }
  } else {
    for (auto& patten : SentenceEndPattens()) {
      // match after
      auto current_sentence_end = content_string.find(patten, byte_pos_start);
      if (current_sentence_end != std::string::npos) {
        // sentence end matched
        sentence_end = current_sentence_end;
        sentence_end_length = patten.length();
        break;
      }
    }
    if (sentence_end == std::string::npos) {
      // no sentence end matched
      sentence_end = content_string.length();
      sentence_end_length = 0;
    }
  }
  return {.start_ = static_cast<int32_t>(base::CIndexToUTF8Index(
              content_string.data(), content_string.length(), sentence_start)),
          .end_ = static_cast<int32_t>(base::CIndexToUTF8Index(
              content_string.data(), content_string.length(),
              sentence_end + sentence_end_length))};
}

}  // namespace markdown
}  // namespace lynx
