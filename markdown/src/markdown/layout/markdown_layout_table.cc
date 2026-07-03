// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_table.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_context.h"
#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {
namespace {
constexpr float kInfiniteLayoutSize = std::numeric_limits<float>::max();
}  // namespace

MarkdownLayoutTableCell::MarkdownLayoutTableCell(MarkdownContext* context)
    : MarkdownLayoutNode(context) {}

MarkdownLayoutTableCell::~MarkdownLayoutTableCell() = default;

MarkdownLayoutNode::LayoutResult MarkdownLayoutTableCell::OnLayout(
    LayoutParams params) {
  if (has_definite_width_) {
    params.width = definite_width_;
    params.width_mode = tttext::LayoutMode::kDefinite;
  }
  auto result = MarkdownLayoutNode::OnLayout(params);
  if (params.width_mode == tttext::LayoutMode::kDefinite) {
    result.width = params.width;
  }
  return result;
}

void MarkdownLayoutTableCell::ApplyTableCellHeight(float height) {
  float offset = 0;
  const float extra_height = height - layout_result_.height;
  if (vertical_align_ == MarkdownVerticalAlign::kCenter) {
    offset = extra_height / 2;
  } else if (vertical_align_ == MarkdownVerticalAlign::kBottom) {
    offset = extra_height;
  }
  if (offset > 0) {
    for (auto child = first_child_; child != nullptr;
         child = child->GetNext()) {
      auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
      if (!layout_child->HasLayout()) {
        continue;
      }
      auto rect = layout_child->GetLayoutRect();
      rect.Offset(0, offset);
      layout_child->UpdateLayoutRect(rect);
    }
    layout_result_.baseline += offset;
  }
  layout_result_.height = height;
}

void MarkdownLayoutTableCell::GetContentByCharPos(std::string* result,
                                                  int32_t char_pos_start,
                                                  int32_t char_pos_end) {
  auto length_before = result->length();
  MarkdownLayoutNode::GetContentByCharPos(result, char_pos_start, char_pos_end);
  if (result->length() > length_before && result->back() == '\n') {
    result->pop_back();
    if (GetNext() != nullptr) {
      result->push_back(' ');
    }
  }
}

MarkdownLayoutTableRow::MarkdownLayoutTableRow(MarkdownContext* context)
    : MarkdownLayoutNode(context) {
  SetLayoutMainAxis(LayoutMainAxis::kHorizontal);
  EnableCollapseMargin(false);
}

MarkdownLayoutTableRow::~MarkdownLayoutTableRow() = default;

void MarkdownLayoutTableRow::ClearColumnDefiniteWidth() const {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    reinterpret_cast<MarkdownLayoutTableCell*>(child)->ClearDefiniteWidth();
  }
}

void MarkdownLayoutTableRow::CollectMaxColumnWidths(
    std::vector<float>* column_widths) const {
  int32_t column_index = 0;
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    if (column_index >= column_widths->size()) {
      break;
    }
    auto cell = reinterpret_cast<MarkdownLayoutTableCell*>(child);
    auto rect = cell->GetLayoutRect();
    auto margins = cell->GetMargins();
    float width = rect.GetWidth() + margins.left_ + margins.right_;
    (*column_widths)[column_index] =
        std::max(width, (*column_widths)[column_index]);
    column_index++;
  }
}

void MarkdownLayoutTableRow::UpdateColumnDefiniteWidth(
    const std::vector<float>& widths) const {
  int32_t column_index = 0;
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    if (column_index >= widths.size()) {
      break;
    }
    auto cell = reinterpret_cast<MarkdownLayoutTableCell*>(child);
    cell->SetDefiniteWidth(widths[column_index]);
    column_index++;
  }
}

void MarkdownLayoutTableRow::UpdateCellHeight() const {
  float cell_max_height = 0;
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto cell = reinterpret_cast<MarkdownLayoutTableCell*>(child);
    auto cell_rect = cell->GetLayoutRect();
    auto margins = cell->GetMargins();
    cell_max_height =
        std::max(cell_max_height,
                 cell_rect.GetHeight() + margins.top_ + margins.bottom_);
  }
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto cell = reinterpret_cast<MarkdownLayoutTableCell*>(child);
    auto margins = cell->GetMargins();
    float cell_height = cell_max_height - margins.top_ - margins.bottom_;
    cell->ApplyTableCellHeight(cell_height);
  }
}

void MarkdownLayoutTableRow::GetLineEndCharIndices(
    std::vector<int32_t>* result) {
  result->emplace_back(absolute_char_pos_ + layout_result_.char_count);
}

MarkdownLayoutNode::LayoutResult MarkdownLayoutTableRow::OnLayout(
    LayoutParams params) {
  // max lines means max row count when table layout.
  if (params.max_lines > 0) {
    params.max_lines = std::numeric_limits<int32_t>::max();
  } else {
    return {
        .overflow = true,
        .truncated = false,
    };
  }
  auto result = MarkdownLayoutNode::OnLayout(params);
  result.line_count = 1;
  return result;
}
void MarkdownLayoutTableRow::GetContentByCharPos(std::string* result,
                                                 int32_t char_pos_start,
                                                 int32_t char_pos_end) {
  MarkdownLayoutNode::GetContentByCharPos(result, char_pos_start, char_pos_end);
  result->push_back('\n');
}

MarkdownLayoutTable::MarkdownLayoutTable(MarkdownContext* context)
    : MarkdownLayoutNode(context) {}

MarkdownLayoutTable::~MarkdownLayoutTable() = default;

void MarkdownLayoutTable::ComputeColumnWidths(std::vector<float>& column_widths,
                                              float max_width) const {
  if (min_width_ > max_width) {
    max_width = min_width_;
  }
  const float total_width =
      std::accumulate(column_widths.begin(), column_widths.end(), 0.0f);
  if (total_width > max_width) {
    auto sorted_widths = column_widths;
    std::sort(sorted_widths.begin(), sorted_widths.end());
    uint32_t large_column_start = 0;
    float rest_width = max_width;
    for (; large_column_start < sorted_widths.size(); large_column_start++) {
      const float avg_rest =
          rest_width /
          static_cast<float>(sorted_widths.size() - large_column_start);
      if (sorted_widths[large_column_start] > avg_rest) {
        break;
      }
      rest_width -= sorted_widths[large_column_start];
    }
    float max_column_width = kInfiniteLayoutSize;
    if (large_column_start < sorted_widths.size()) {
      max_column_width = rest_width / static_cast<float>(sorted_widths.size() -
                                                         large_column_start);
    }
    for (auto& column_width : column_widths) {
      column_width = std::min(column_width, max_column_width);
    }
  }

  float min_width = min_width_;
  if (min_width > total_width && column_count_ > 0) {
    const float extra =
        (min_width - total_width) / static_cast<float>(column_count_);
    for (auto& column_width : column_widths) {
      column_width += extra;
    }
  }
}

MarkdownLayoutNode::LayoutResult MarkdownLayoutTable::OnLayout(
    LayoutParams params) {
  if (first_child_ == nullptr) {
    return {};
  }
  visible_row_count_ = 0;
  column_count_ = first_child_->GetChildCount();
  if (GetRowCount() <= 0 || column_count_ <= 0) {
    return {};
  }
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    row->ClearColumnDefiniteWidth();
  }
  MarkdownLayoutNode::OnLayout(
      {.width = kInfiniteLayoutSize,
       .width_mode = tttext::LayoutMode::kAtMost,
       .height = kInfiniteLayoutSize,
       .height_mode = tttext::LayoutMode::kAtMost,
       .max_lines = std::numeric_limits<int32_t>::max(),
       .last_element = false});
  std::vector<float> column_widths(static_cast<size_t>(column_count_), 0);
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    row->CollectMaxColumnWidths(&column_widths);
  }
  float content_max_width = params.width - paddings_.left_ - paddings_.right_ -
                            borders_.left_.width_ - borders_.right_.width_;
  ComputeColumnWidths(column_widths, content_max_width);
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    row->UpdateColumnDefiniteWidth(column_widths);
  }
  auto result = MarkdownLayoutNode::OnLayout(params);
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    row->UpdateCellHeight();
  }
  visible_row_count_ = result.line_count;
  return result;
}

void MarkdownLayoutTable::OnRender(RenderParams params) {
  if (params.canvas == nullptr || visible_row_count_ <= 0) {
    return;
  }
  params.canvas->Save();
  auto* extend_canvas = context_ == nullptr
                            ? nullptr
                            : context_->GetMarkdownCanvasExtend(params.canvas);
  const float radius = borders_.left_.radius_top_;
  float height = ComputeRenderBottom(params);
  if (extend_canvas != nullptr && radius > 0) {
    MarkdownPath path;
    path.AddRoundRect(
        {.rect_ = RectF::MakeLTWH(0, 0, layout_result_.width, height),
         .radius_x_ = radius,
         .radius_y_ = radius});
    extend_canvas->ClipPath(&path);
  } else {
    params.canvas->ClipRect(0, 0, layout_result_.width, height, true);
  }

  DrawTableBackground(params, height);
  DrawTableBorder(params, height);
  DrawChildren(params);
  params.canvas->Restore();
}

float MarkdownLayoutTable::ComputeRenderBottom(
    const RenderParams& params) const {
  int32_t char_count = params.max_char_count;
  float inner_bottom = paddings_.bottom_ + borders_.bottom_.width_;
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    const auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    if (!row->HasLayout()) {
      break;
    }
    if (row->GetCharCount() > char_count) {
      return row->GetLayoutRect().GetBottom() + inner_bottom;
    }
  }
  return layout_rect_.GetHeight();
}

void MarkdownLayoutTable::DrawTableBackground(const RenderParams& params,
                                              float height) const {
  if (table_background_ == MarkdownTableBackground::kNone ||
      params.canvas == nullptr || visible_row_count_ <= 0) {
    return;
  }
  auto painter = params.canvas->CreatePainter();
  painter->SetFillColor(table_background_color_);
  const float radius = borders_.left_.radius_top_;
  if (radius > 0) {
    params.canvas->DrawRoundRect(0, 0, layout_result_.width, height, radius,
                                 painter.get());
  } else {
    params.canvas->DrawRect(0, 0, layout_result_.width, height, painter.get());
  }

  if (table_background_ != MarkdownTableBackground::kChessBoard) {
    return;
  }
  painter->SetFillColor(table_alternate_background_color_);
  int32_t offset = 1;
  for (auto* child = first_child_; child != nullptr; child = child->GetNext()) {
    const auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
    if (!row->HasLayout()) {
      break;
    }
    int32_t column_index = 0;
    auto row_rect = row->GetLayoutRect();
    for (auto row_child = row->GetFirstChild(); row_child != nullptr;
         row_child = row_child->GetNext()) {
      const auto* cell = reinterpret_cast<MarkdownLayoutTableCell*>(row_child);
      if (cell == nullptr || !cell->HasLayout() || column_index % 2 == offset) {
        column_index++;
        continue;
      }
      auto cell_rect = cell->GetLayoutRect();
      params.canvas->DrawRect(row_rect.GetLeft() + cell_rect.GetLeft(),
                              row_rect.GetTop() + cell_rect.GetTop(),
                              row_rect.GetLeft() + cell_rect.GetRight(),
                              row_rect.GetTop() + cell_rect.GetBottom(),
                              painter.get());
      column_index++;
    }
    offset = offset == 1 ? 0 : 1;
  }
}

void MarkdownLayoutTable::DrawTableBorder(const RenderParams& params,
                                          float height) const {
  if (table_border_ == MarkdownTableBorder::kNone || params.canvas == nullptr ||
      visible_row_count_ <= 0) {
    return;
  }
  const float line_width = borders_.left_.width_;
  if (line_width <= 0) {
    return;
  }
  auto painter = params.canvas->CreatePainter();
  painter->SetStrokeColor(borders_.left_.color_);
  painter->SetStrokeWidth(line_width);
  const float half_line_width = line_width / 2;

  if (table_border_ == MarkdownTableBorder::kFullRect) {
    const float radius = borders_.left_.radius_top_;
    if (radius > 0) {
      params.canvas->DrawRoundRect(half_line_width, half_line_width,
                                   layout_result_.width - half_line_width,
                                   height - half_line_width, radius,
                                   painter.get());
    } else {
      params.canvas->DrawLine(half_line_width, half_line_width,
                              layout_result_.width - half_line_width,
                              half_line_width, painter.get());
      params.canvas->DrawLine(half_line_width, half_line_width, half_line_width,
                              height - half_line_width, painter.get());
      params.canvas->DrawLine(layout_result_.width - half_line_width,
                              half_line_width,
                              layout_result_.width - half_line_width,
                              height - half_line_width, painter.get());
      params.canvas->DrawLine(half_line_width, height - half_line_width,
                              layout_result_.width - half_line_width,
                              height - half_line_width, painter.get());
    }
  }

  if (table_border_ == MarkdownTableBorder::kUnderline) {
    for (auto* child = first_child_; child != nullptr;
         child = child->GetNext()) {
      const auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
      if (!row->HasLayout()) {
        break;
      }
      const float y = row->GetLayoutRect().GetBottom() - half_line_width;
      params.canvas->DrawLine(0, y, layout_result_.width, y, painter.get());
    }
    return;
  }

  if (table_split_ == MarkdownTableSplit::kAll ||
      table_split_ == MarkdownTableSplit::kHorizontal) {
    for (auto* child = first_child_;
         child != nullptr && child->GetNext() != nullptr;
         child = child->GetNext()) {
      const auto* row = reinterpret_cast<MarkdownLayoutTableRow*>(child);
      if (!row->HasLayout()) {
        break;
      }
      const float y = row->GetLayoutRect().GetBottom();
      params.canvas->DrawLine(0, y, layout_result_.width, y, painter.get());
    }
  }
  if (table_split_ == MarkdownTableSplit::kAll ||
      table_split_ == MarkdownTableSplit::kVertical) {
    auto* first_row = reinterpret_cast<MarkdownLayoutTableRow*>(first_child_);
    if (first_row == nullptr) {
      return;
    }
    auto row_rect = first_row->GetLayoutRect();
    for (auto row_child = first_row->GetFirstChild();
         row_child != nullptr && row_child->GetNext() != nullptr;
         row_child = row_child->GetNext()) {
      const auto* cell = reinterpret_cast<MarkdownLayoutTableCell*>(row_child);
      if (cell == nullptr || !cell->HasLayout()) {
        continue;
      }
      const float x = row_rect.GetLeft() + cell->GetLayoutRect().GetRight();
      params.canvas->DrawLine(x, 0, x, height, painter.get());
    }
  }
}

}  // namespace serval::markdown
