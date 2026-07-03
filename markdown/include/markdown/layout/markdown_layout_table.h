// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_TABLE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_TABLE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "markdown/layout/markdown_layout_node.h"
#include "markdown/style/markdown_style.h"

namespace serval::markdown {

class MarkdownLayoutTable;

class MarkdownLayoutTableCell : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutTableCell(MarkdownContext* context);
  ~MarkdownLayoutTableCell() override;

  void SetVerticalAlign(MarkdownVerticalAlign align) {
    vertical_align_ = align;
  }
  MarkdownVerticalAlign GetVerticalAlign() const { return vertical_align_; }

  void ClearDefiniteWidth() { has_definite_width_ = false; }
  void SetDefiniteWidth(float width) {
    definite_width_ = width;
    has_definite_width_ = true;
  }
  void ApplyTableCellHeight(float height);
  void GetContentByCharPos(std::string* result, int32_t char_pos_start, int32_t char_pos_end) override;

 protected:
  LayoutResult OnLayout(LayoutParams params) override;

  MarkdownVerticalAlign vertical_align_{MarkdownVerticalAlign::kCenter};
  bool has_definite_width_{false};
  float definite_width_{0};
};

class MarkdownLayoutTableRow : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutTableRow(MarkdownContext* context);
  ~MarkdownLayoutTableRow() override;
  void ClearColumnDefiniteWidth() const;
  void CollectMaxColumnWidths(std::vector<float>* column_widths) const;
  void UpdateColumnDefiniteWidth(const std::vector<float>& widths) const;
  void UpdateCellHeight() const;
  void GetLineEndCharIndices(std::vector<int32_t>* result) override;
  void GetContentByCharPos(std::string* result, int32_t char_pos_start, int32_t char_pos_end) override;
protected:
  LayoutResult OnLayout(LayoutParams params) override;
};

class MarkdownLayoutTable : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutTable(MarkdownContext* context);
  ~MarkdownLayoutTable() override;

  int32_t GetRowCount() const { return GetChildCount(); }
  int32_t GetColumnCount() const { return column_count_; }

  void SetTextOverflow(MarkdownTextOverflow overflow) {
    text_overflow_ = overflow;
  }
  MarkdownTextOverflow GetTextOverflow() const { return text_overflow_; }
  void SetMinWidth(float min_width) { min_width_ = min_width; }
  float GetMinWidth() const { return min_width_; }
  int32_t GetVisibleRowCount() const { return visible_row_count_; }

  void SetTableBorder(MarkdownTableBorder border) { table_border_ = border; }
  MarkdownTableBorder GetTableBorder() const { return table_border_; }
  void SetTableBackground(MarkdownTableBackground background) {
    table_background_ = background;
  }
  MarkdownTableBackground GetTableBackground() const {
    return table_background_;
  }
  void SetTableSplit(MarkdownTableSplit split) { table_split_ = split; }
  MarkdownTableSplit GetTableSplit() const { return table_split_; }
  void SetTableBackgroundColor(uint32_t color) {
    table_background_color_ = color;
  }
  uint32_t GetTableBackgroundColor() const { return table_background_color_; }
  void SetTableAlternateBackgroundColor(uint32_t color) {
    table_alternate_background_color_ = color;
  }
  uint32_t GetTableAlternateBackgroundColor() const {
    return table_alternate_background_color_;
  }

 protected:
  LayoutResult OnLayout(LayoutParams params) override;
  void OnRender(RenderParams params) override;
  float ComputeRenderBottom(const RenderParams& params) const;

  void DrawTableBackground(const RenderParams& params, float height) const;
  void DrawTableBorder(const RenderParams& params, float height) const;
  void ComputeColumnWidths(std::vector<float>& widths, float max_width) const;

  int32_t visible_row_count_{0};
  int32_t column_count_{0};
  MarkdownTextOverflow text_overflow_{MarkdownTextOverflow::kClip};
  float min_width_{0};
  MarkdownTableBorder table_border_{MarkdownTableBorder::kNone};
  MarkdownTableBackground table_background_{MarkdownTableBackground::kNone};
  MarkdownTableSplit table_split_{MarkdownTableSplit::kNone};
  uint32_t table_background_color_{0};
  uint32_t table_alternate_background_color_{0};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_TABLE_H_
