// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_SELECTION_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_SELECTION_H_
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "markdown/element/markdown_page.h"
#include "markdown/element/markdown_table.h"
#include "markdown/utils/markdown_definition.h"

namespace lynx {
namespace markdown {

struct MarkdownSelectionRegion {
  tttext::LayoutRegion* region_;
  int32_t char_pos_offset_;
  int32_t char_count_;
  PointF offset_;
};
class MarkdownSelection {
 public:
  enum class RectType { kSelection, kLineBounding, kCharBounding };
  enum class RectCoordinate { kAbsolute, kRelative };
  enum class CharRangeType { kChar, kWord, kSentence, kParagraph };

 public:
  static Range GetCharRangeByPoint(const MarkdownPage* page, PointF point,
                                   CharRangeType type);
  /**
   * @param type selection: selection rect, rect top/bottom = line top/bottom
   *             char bounding: bounding rect, rect top/bottom = char
   * min-top/max-bottom
   * @param coordinate absolute: from page origin
   *                   relative: from text region origin
   */
  static std::vector<RectF> GetSelectionRectByCharPos(
      const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end,
      RectType type = RectType::kSelection,
      RectCoordinate coordinate = RectCoordinate::kAbsolute);
  static RectF GetSelectionClosedRectByCharPos(
      const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end,
      RectType type = RectType::kSelection,
      RectCoordinate coordinate = RectCoordinate::kAbsolute);
  static std::string GetContentByCharPos(
      const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end,
      std::vector<std::pair<uint32_t, std::string>>*
          inline_element_alt_strings);
  static int GetPageCharCount(const MarkdownPage* page);
  static int FindClosestRegionIndex(const MarkdownPage* page, float y);
  static std::vector<MarkdownSelectionRegion> GetSelectionRegionsByCharRange(
      const MarkdownPage* page, int32_t char_pos_start, int32_t char_pos_end);

 private:
  static inline std::vector<std::string_view> SentenceEndPattens() {
    return {
        "。", "？", "\n", "\r", "！", "……", ". ", "? ", "! ", "; ", "；",
    };
  }
  static int FindClosestMarkdownRegionLineIndexOrRowIndex(
      MarkdownPageRegion* region, float y) {
    y -= region->rect_.GetTop();
    if (region->element_->GetType() == MarkdownElementType::kParagraph) {
      return FindRegionLineIndex(
          reinterpret_cast<MarkdownPageParagraphRegion*>(region)->region_.get(),
          y);
    } else if (region->element_->GetType() == MarkdownElementType::kTable) {
      return FindTableRowIndex(
          reinterpret_cast<MarkdownPageTableRegion*>(region)->table_.get(), y);
    } else {
      return -1;
    }
  }
  static int FindClosestMarkdownRegionCharIndexOrColumnIndex(
      MarkdownPageRegion* region, int line_index, float x) {
    x -= region->rect_.GetLeft() +
         (region->scroll_x_ ? region->scroll_x_offset_ : 0.f);
    if (region->element_->GetType() == MarkdownElementType::kParagraph) {
      return GetCharPosInLineByX(
          reinterpret_cast<MarkdownPageParagraphRegion*>(region)
              ->region_->GetLine(line_index),
          x);
    } else if (region->element_->GetType() == MarkdownElementType::kTable) {
      return FindTableColumnIndex(
          reinterpret_cast<MarkdownPageTableRegion*>(region)->table_.get(), x);
    }
    return -1;
  }
  static std::pair<int, int> FindCellLineAndCharIndex(
      MarkdownPageTableRegion* region, int row_index, int column_index,
      PointF pos) {
    if (region->table_ == nullptr) {
      return {-1, -1};
    }
    auto& cell = region->table_->GetCell(row_index, column_index);
    if (cell.region_ == nullptr) {
      return {0, 0};
    }
    pos -= PointF{region->rect_.GetLeft() + cell.cell_rect_.GetLeft(),
                  region->rect_.GetTop() + cell.cell_rect_.GetTop()} +
           cell.region_offset_;
    if (region->scroll_x_) {
      pos.x_ -= region->scroll_x_offset_;
    }
    auto line_index = FindRegionLineIndex(cell.region_.get(), pos.y_);
    auto char_index =
        GetCharPosInLineByX(cell.region_->GetLine(line_index), pos.x_);
    return {line_index, char_index};
  }
  static int GetCharPosInLineByX(tttext::TextLine* line, float x) {
    return line->GetCharPosByCoordinateX(x);
  }
  static int FindRegionLineIndex(tttext::LayoutRegion* region, float y);
  static int FindTableRowIndex(MarkdownTableRegion* table, float y);
  static int FindTableColumnIndex(MarkdownTableRegion* table, float x);

  static void GetPageRegionSelectionRectByCharPos(MarkdownPageRegion* region,
                                                  int32_t char_pos_start,
                                                  int32_t char_pos_end,
                                                  std::vector<RectF>* rect_ptr,
                                                  PointF offset, RectType type,
                                                  RectCoordinate coordinate);
  static void GetLayoutRegionSelectionRectByCharPos(
      tttext::LayoutRegion* region, int32_t char_pos_start,
      int32_t char_pos_end, std::vector<RectF>* rect_ptr, PointF offset,
      RectType type, RectCoordinate coordinate, RectF clip_rect);
  static void GetTableSelectionRectByCharPos(
      MarkdownTable* table, MarkdownTableRegion* region, int32_t char_pos_start,
      int32_t char_pos_end, std::vector<RectF>* rect_ptr, PointF offset,
      RectType type, RectCoordinate coordinate, RectF clip_rect);

  static void GetPageRegionContentByCharPos(
      MarkdownPageRegion* region, int32_t char_pos_start, int32_t char_pos_end,
      std::string* content,
      std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
      int32_t char_offset);
  static void GetLayoutRegionContentByCharPos(
      tttext::LayoutRegion* region, int32_t char_pos_start,
      int32_t char_pos_end, std::string* content,
      std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
      int32_t char_offset, bool need_add_space);
  static void GetTableContentByCharPos(
      MarkdownTable* table, MarkdownTableRegion* region, int32_t char_pos_start,
      int32_t char_pos_end, std::string* content,
      std::vector<std::pair<uint32_t, std::string>>* inline_element_alt_strings,
      int32_t char_offset);

  static Range GetSentenceOfChar(tttext::Paragraph* paragraph,
                                 int32_t char_pos);
  static RectF ClipRect(RectF origin, RectF clip) {
    float left = std::max(origin.GetLeft(), clip.GetLeft());
    float top = std::max(origin.GetTop(), clip.GetTop());
    float right = std::min(origin.GetRight(), clip.GetRight());
    float bottom = std::min(origin.GetBottom(), clip.GetBottom());
    if (left > right) {
      if (left == clip.GetLeft()) {
        right = left;
      } else {
        left = right;
      }
    }
    if (top > bottom) {
      if (bottom == clip.GetBottom()) {
        top = bottom;
      } else {
        bottom = top;
      }
    }
    return RectF::MakeLTRB(left, top, right, bottom);
  }
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_SELECTION_H_
