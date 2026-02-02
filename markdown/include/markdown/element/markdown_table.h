// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_TABLE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_TABLE_H_
#include <memory>
#include <utility>
#include <vector>

#include "markdown/element/markdown_paragraph.h"
#include "markdown/element/markdown_region.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {

struct MarkdownTableCell {
  std::unique_ptr<tttext::Paragraph> paragraph_{nullptr};
  tttext::ParagraphHorizontalAlignment alignment_{
      tttext::ParagraphHorizontalAlignment::kLeft};
  MarkdownVerticalAlign vertical_alignment_{MarkdownVerticalAlign::kCenter};
  uint32_t char_start_{0};
  uint32_t char_count_{0};
};
template <typename Value>
class MarkdownTableMatrix {
 public:
  MarkdownTableMatrix() : MarkdownTableMatrix(0, 0) {}
  MarkdownTableMatrix(int rows, int columns) { Resize(rows, columns); }
  void Resize(int int_rows, int int_columns) {
    size_t rows = static_cast<size_t>(int_rows);
    size_t columns = static_cast<size_t>(int_columns);
    if (rows > cells_.size()) {
      size_t resize_start = cells_.size();
      cells_.resize(rows);
      for (size_t index = resize_start; index < cells_.size(); index++) {
        cells_[index].resize(cells_.front().size());
      }
    } else {
      cells_.resize(rows);
    }
    if (!cells_.empty() && columns > cells_.front().size()) {
      for (auto& row : cells_) {
        row.resize(columns);
      }
    }
  }

  int GetRowCount() const { return cells_.size(); }

  int GetColumnCount() const {
    return cells_.empty() ? 0 : cells_.front().size();
  }

  const Value& GetCell(int row, int column) const {
    return cells_[row][column];
  }

  Value& GetCell(int row, int column) { return cells_[row][column]; }

  void SetCell(int row, int column, Value&& value) {
    cells_[row][column] = std::move(value);
  }

  bool Empty() const { return cells_.empty() || cells_.front().empty(); }

 private:
  std::vector<std::vector<Value>> cells_;

  friend class MarkdownSelection;
};

class MarkdownTable : public MarkdownTableMatrix<MarkdownTableCell> {
 public:
  MarkdownTable() : MarkdownTableMatrix() {}
  MarkdownTable(int rows, int columns)
      : MarkdownTableMatrix<MarkdownTableCell>(rows, columns) {}
  void SetCellStyle(const MarkdownBlockStylePart& cell_block_style) {
    cell_block_style_ = cell_block_style;
  }
  void SetCellBackground(const uint32_t color) { cell_background_ = color; }
  uint32_t GetCellBackground() const { return cell_background_; }
  void SetHeaderStyle(const MarkdownBlockStylePart& header_block_style) {
    header_block_style_ = header_block_style;
  }
  void SetHeaderBackground(const uint32_t color) { header_background_ = color; }
  uint32_t GetHeaderBackground() const { return header_background_; }
  void SetTableStyle(const MarkdownTableStylePart& table_style_part) {
    table_style_ = table_style_part;
  }
  const MarkdownTableStylePart& GetTableStyle() const { return table_style_; }
  uint32_t GetCharCount() const { return char_count_; }
  void SetCharCount(uint32_t char_count) { char_count_ = char_count; }

 private:
  MarkdownBlockStylePart cell_block_style_{};
  uint32_t cell_background_{0};
  MarkdownBlockStylePart header_block_style_{};
  uint32_t header_background_{0};
  MarkdownTableStylePart table_style_{};
  uint32_t char_count_{0};
  friend class MarkdownLayout;
  friend class MarkdownSelection;
  friend class MarkdownParser;
};

class MarkdownTableElement : public MarkdownElement {
 public:
  MarkdownTableElement() : MarkdownElement(MarkdownElementType::kTable) {}
  ~MarkdownTableElement() override = default;

 public:
  void SetTable(std::unique_ptr<MarkdownTable> table) {
    table_ = std::move(table);
  }
  MarkdownTable* GetTable() const { return table_.get(); }

 protected:
  std::unique_ptr<MarkdownTable> table_{nullptr};
};

struct MarkdownTableRegionCell {
  std::unique_ptr<tttext::LayoutRegion> region_{nullptr};
  RectF cell_rect_{};
  PointF region_offset_{};

  float GetRegionWidth() const {
    return region_ == nullptr
               ? 0
               : MarkdownPlatform::GetMdLayoutRegionWidth(region_.get());
  }
  float GetRegionHeight() const {
    return region_ == nullptr
               ? 0
               : MarkdownPlatform::GetMdLayoutRegionHeight(region_.get());
  }
};

class MarkdownTableRegion
    : public MarkdownTableMatrix<MarkdownTableRegionCell> {
 public:
  MarkdownTableRegion() = default;
  MarkdownTableRegion(int rows, int columns)
      : MarkdownTableMatrix<MarkdownTableRegionCell>(rows, columns) {}

  friend class MarkdownLayout;
  friend class MarkdownDrawer;
  friend class MarkdownSelection;

 private:
  float total_width_{0};
  float total_height_{0};
};

class MarkdownPageTableRegion : public MarkdownPageRegion {
 public:
  ~MarkdownPageTableRegion() override = default;

 public:
  std::unique_ptr<MarkdownTableRegion> table_{nullptr};
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_TABLE_H_
