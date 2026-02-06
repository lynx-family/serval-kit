// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/draw/markdown_drawer.h"

#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_attachments.h"
#include "markdown/element/markdown_table.h"
#include "markdown/layout/markdown_selection.h"
namespace lynx {
namespace markdown {

void MarkdownDrawer::DrawPage(const lynx::markdown::MarkdownPage& page) {
  tttext::LayoutDrawer drawer(canvas_);
  canvas_->Save();
  canvas_->ClipRect(0, 0, std::min(page.GetLayoutWidth(), page.max_width_),
                    std::min(page.GetLayoutHeight(), page.max_height_), true);
  const auto& attachments = page.GetTextAttachments();
  for (const auto& attachment : attachments) {
    if (attachment->attachment_layer_ == AttachmentLayer::kBackground) {
      DrawAttachment(page, attachment.get());
    }
  }
  for (const auto& attachment : page.GetBorderAttachments()) {
    DrawAttachment(page, attachment.get());
  }
  auto extra_border = page.quote_borders_.begin();
  for (auto& region : page.regions_) {
    // TODO(zhouchaoying): temporarily fix quote border, will be removed next
    // commit
    while (extra_border != page.quote_borders_.end() &&
           (*extra_border)->rect_.GetTop() <= region->rect_.GetTop()) {
      DrawQuoteLine(*((*extra_border)));
      extra_border++;
    }
    canvas_->Save();
    if (region->scroll_x_) {
      canvas_->ClipRect(region->scroll_x_view_rect_.GetLeft(),
                        region->scroll_x_view_rect_.GetTop(),
                        region->scroll_x_view_rect_.GetRight(),
                        region->scroll_x_view_rect_.GetBottom(), true);
      canvas_->Translate(region->scroll_x_offset_, 0);
    }
    if (region->border_ != nullptr) {
      DrawBorder(*region->border_);
    }
    DrawRegion(*region, &drawer);
    canvas_->Restore();
    if (terminated_) {
      break;
    }
  }
  for (const auto& attachment : attachments) {
    if (attachment->attachment_layer_ == AttachmentLayer::kForeGround) {
      DrawAttachment(page, attachment.get());
    }
  }
  canvas_->Restore();
}

void MarkdownDrawer::DrawRegion(const MarkdownPage& page,
                                uint32_t region_index) {
  if (region_index >= page.regions_.size())
    return;
  tttext::LayoutDrawer drawer(canvas_);
  canvas_->Save();
  const auto* region = page.regions_[region_index].get();
  auto region_rect = page.GetRegionRect(region_index);
  canvas_->ClipRect(region_rect.GetLeft(), region_rect.GetTop(),
                    region_rect.GetRight(), region_rect.GetBottom(), false);
  if (region->scroll_x_) {
    canvas_->ClipRect(region->scroll_x_view_rect_.GetLeft(),
                      region->scroll_x_view_rect_.GetTop(),
                      region->scroll_x_view_rect_.GetRight(),
                      region->scroll_x_view_rect_.GetBottom(), true);
  }
  int32_t region_start = region->element_->GetCharStart();
  int32_t region_end = region->element_->GetCharCount() + region_start;
  const auto& attachments = page.GetTextAttachments();
  for (const auto& attachment : attachments) {
    if (attachment->attachment_layer_ == AttachmentLayer::kBackground) {
      DrawAttachmentOnRegion(page, attachment.get(), region_start, region_end);
    }
  }
  for (const auto& attachment : page.GetBorderAttachments()) {
    DrawAttachmentOnRegion(page, attachment.get(), region_start, region_end);
  }
  canvas_->Save();
  if (region->scroll_x_) {
    canvas_->Translate(region->scroll_x_offset_, 0);
  }
  if (region->border_ != nullptr) {
    DrawBorder(*region->border_);
  }
  DrawRegion(*region, &drawer);
  canvas_->Restore();
  for (const auto& attachment : attachments) {
    if (attachment->attachment_layer_ == AttachmentLayer::kForeGround) {
      DrawAttachmentOnRegion(page, attachment.get(), region_start, region_end);
    }
  }
  canvas_->Restore();
}

void MarkdownDrawer::DrawQuoteBorder(const MarkdownPage& page,
                                     uint32_t border_index) {
  auto* border = page.GetExtraBorder(border_index);
  if (border == nullptr)
    return;
  canvas_->Save();
  auto border_rect = border->rect_;
  canvas_->Translate(-border_rect.GetLeft(), -border_rect.GetTop());
  DrawQuoteLine(*border);
  canvas_->Restore();
}

void MarkdownDrawer::DrawQuoteLine(const MarkdownQuoteBorder& border) {
  canvas_->Save();
  auto painter = canvas_->CreatePainter();
  painter->SetFillColor(border.line_style_.line_.color_);
  const auto rect = RectF::MakeLTWH(
      border.rect_.GetLeft(),
      border.rect_.GetTop() + border.line_style_.line_.shrink_,
      border.line_style_.line_.width_,
      border.rect_.GetHeight() - border.line_style_.line_.shrink_ * 2);
  if (border.line_style_.line_.line_type_ == MarkdownLineType::kSolid) {
    if (border.line_style_.line_.radius_ > 0) {
      canvas_->DrawRoundRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                             rect.GetBottom(), border.line_style_.line_.radius_,
                             painter.get());
    } else {
      canvas_->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                        rect.GetBottom(), painter.get());
    }
  } else if (border.line_style_.line_.line_type_ == MarkdownLineType::kDashed) {
    constexpr float kDefaultDashLength = 2.5f;
    constexpr float kDefaultGapLength = 1.5f;
    auto nums = static_cast<int>(std::floor(
        rect.GetHeight() / (kDefaultDashLength + kDefaultGapLength)));
    const int num_dashes = std::max(nums, 1);
    float gap_len = kDefaultDashLength;
    float dash_len = kDefaultDashLength;
    if (num_dashes > 1) {
      const int num_gaps = num_dashes - 1;
      gap_len = (rect.GetHeight() - dash_len * nums) / num_gaps;
    }
    for (int i = 0; i < num_dashes; i++) {
      const float top = rect.GetTop() + i * (dash_len + gap_len);
      const float bottom = std::min(top + dash_len, rect.GetBottom());
      if (border.line_style_.line_.radius_ > 0) {
        canvas_->DrawRoundRect(rect.GetLeft(), top, rect.GetRight(), bottom,
                               border.line_style_.line_.radius_, painter.get());
      } else {
        canvas_->DrawRect(rect.GetLeft(), top, rect.GetRight(), bottom,
                          painter.get());
      }
    }
  }
}

void MarkdownDrawer::DrawBorder(
    const lynx::markdown::MarkdownPageRegionBorder& border) {
  if (border.border_style_.border_color_ == 0)
    return;
  canvas_->Save();
  auto painter = canvas_->CreatePainter();
  painter->SetStrokeWidth(border.border_style_.border_width_);
  painter->SetStrokeColor(border.border_style_.border_color_);
  auto rect = border.rect_;
  const auto half_width = border.border_style_.border_width_ * 0.5;
  rect.SetLeft(rect.GetLeft() + half_width);
  rect.SetTop(rect.GetTop() + half_width);
  rect.SetWidth(rect.GetWidth() - half_width * 2);
  rect.SetHeight(rect.GetHeight() - half_width * 2);
  if (border.border_ == MarkdownBorder::kLeft) {
    canvas_->DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetLeft(),
                      rect.GetBottom(), painter.get());
  } else if (border.border_ == MarkdownBorder::kRect) {
    if (border.border_style_.border_radius_ > 0) {
      canvas_->DrawRoundRect(
          rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom(),
          border.border_style_.border_radius_, painter.get());
    } else {
      canvas_->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                        rect.GetBottom(), painter.get());
    }
  } else if (border.border_ == MarkdownBorder::kTop) {
    canvas_->DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                      rect.GetTop(), painter.get());
  }
  canvas_->Restore();
}

void MarkdownDrawer::DrawRegion(
    const lynx::markdown::MarkdownPageRegion& region,
    tttext::LayoutDrawer* drawer) {
  if (terminated_) {
    return;
  }
  canvas_->Save();
  canvas_->Translate(region.rect_.GetLeft(), region.rect_.GetTop());
  auto type = region.element_->GetType();
  if (type == MarkdownElementType::kParagraph) {
    auto* para_region =
        reinterpret_cast<const MarkdownPageParagraphRegion*>(&region);
    DrawTextRegion(para_region->region_.get(), drawer);
  } else if (type == MarkdownElementType::kTable) {
    auto* table_region =
        reinterpret_cast<const MarkdownPageTableRegion*>(&region);
    DrawTable(*table_region->table_, *table_region->element_, drawer);
  }
  canvas_->Restore();
}

void MarkdownDrawer::DrawTableBackground(const MarkdownTableRegion& table,
                                         const MarkdownElement& element) {
  auto& table_style = static_cast<const MarkdownTableElement*>(&element)
                          ->GetTable()
                          ->GetTableStyle();
  if (table_style.table_background_ == MarkdownTableBackground::kNone)
    return;

  auto painter = canvas_->CreatePainter();
  painter->SetFillColor(table_style.background_color_);
  auto radius = element.GetBorderStyle().border_radius_;
  if (radius > 0) {
    canvas_->DrawRoundRect(0, 0, table.total_width_, table.total_height_,
                           radius, painter.get());
  } else {
    canvas_->DrawRect(0, 0, table.total_width_, table.total_height_,
                      painter.get());
  }
  if (table_style.table_background_ == MarkdownTableBackground::kChessBoard) {
    painter->SetFillColor(table_style.alt_color_);
    int32_t offset = 1;
    for (int32_t row = 0; row < table.GetRowCount(); row++) {
      for (int32_t column = offset; column < table.GetColumnCount();
           column += 2) {
        auto rect = table.GetCell(row, column).cell_rect_;
        canvas_->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                          rect.GetBottom(), painter.get());
      }
      offset = offset == 1 ? 0 : 1;
    }
  }
}

void MarkdownDrawer::DrawTableBorder(const MarkdownTableRegion& table,
                                     const MarkdownElement& element) {
  auto& table_style = static_cast<const MarkdownTableElement*>(&element)
                          ->GetTable()
                          ->GetTableStyle();
  if (table_style.table_border_ == MarkdownTableBorder::kNone)
    return;
  // draw cell border
  auto painter = canvas_->CreatePainter();
  painter->SetStrokeColor(element.GetBorderStyle().border_color_);
  auto line_width = element.GetBorderStyle().border_width_;
  painter->SetStrokeWidth(line_width);

  if (table_style.table_border_ == MarkdownTableBorder::kFullRect) {
    auto radius = element.GetBorderStyle().border_radius_;
    canvas_->DrawRoundRect(
        line_width / 2, line_width / 2, table.total_width_ - line_width / 2,
        table.total_height_ - line_width / 2, radius, painter.get());
    if (table_style.table_split_ == MarkdownTableSplit::kAll ||
        table_style.table_split_ == MarkdownTableSplit::kHorizontal) {
      for (int row = 0; row < table.GetRowCount() - 1; row++) {
        auto& cell = table.GetCell(row, 0);
        canvas_->DrawLine(0, cell.cell_rect_.GetBottom(), table.total_width_,
                          cell.cell_rect_.GetBottom(), painter.get());
      }
    }
    if (table_style.table_split_ == MarkdownTableSplit::kAll ||
        table_style.table_split_ == MarkdownTableSplit::kVertical) {
      for (int col = 0; col < table.GetColumnCount() - 1; col++) {
        auto& cell = table.GetCell(0, col);
        canvas_->DrawLine(cell.cell_rect_.GetRight(), 0,
                          cell.cell_rect_.GetRight(), table.total_height_,
                          painter.get());
      }
    }
  } else if (table_style.table_border_ == MarkdownTableBorder::kUnderline) {
    for (int row = 0; row < table.GetRowCount(); row++) {
      auto& cell = table.GetCell(row, 0);
      canvas_->DrawLine(
          0, cell.cell_rect_.GetBottom() - line_width / 2, table.total_width_,
          cell.cell_rect_.GetBottom() - line_width / 2, painter.get());
    }
  }
}

void MarkdownDrawer::DrawCellBackground(const MarkdownTableRegion& table,
                                        const MarkdownElement& element) {
  auto* markdown_table =
      static_cast<const MarkdownTableElement*>(&element)->GetTable();
  auto header_background = markdown_table->GetHeaderBackground();
  auto cell_background = markdown_table->GetCellBackground();
  auto painter = canvas_->CreatePainter();
  if (header_background != 0) {
    painter->SetFillColor(header_background);
    float bottom = table.GetCell(0, 0).cell_rect_.GetBottom();
    canvas_->DrawRect(0, 0, table.total_width_, bottom, painter.get());
  }
  if (cell_background != 0) {
    painter->SetFillColor(cell_background);
    for (auto index = 1; index < table.GetRowCount(); index++) {
      float top = table.GetCell(index, 0).cell_rect_.GetTop();
      float bottom = table.GetCell(index, 0).cell_rect_.GetBottom();
      canvas_->DrawRect(0, top, table.total_width_, bottom, painter.get());
    }
  }
}

void MarkdownDrawer::DrawTable(const lynx::markdown::MarkdownTableRegion& table,
                               const MarkdownElement& element,
                               tttext::LayoutDrawer* drawer) {
  if (table.Empty() || terminated_) {
    return;
  }
  canvas_->Save();
  auto extend_canvas = MarkdownPlatform::GetMarkdownCanvasExtend(canvas_);
  float radius = element.GetBorderStyle().border_radius_;
  if (extend_canvas != nullptr && radius > 0) {
    MarkdownPath path;
    path.AddRoundRect({.rect_ = RectF::MakeLTWH(0, 0, table.total_width_,
                                                table.total_height_),
                       .radius_x_ = radius,
                       .radius_y_ = radius});
    extend_canvas->ClipPath(&path);
  }
  DrawTableBackground(table, element);
  DrawCellBackground(table, element);
  DrawTableBorder(table, element);

  // draw cells
  for (int row = 0; row < table.GetRowCount(); row++) {
    for (int col = 0; col < table.GetColumnCount(); col++) {
      auto& cell = table.GetCell(row, col);
      canvas_->Save();
      canvas_->Translate(cell.cell_rect_.GetLeft() + cell.region_offset_.x_,
                         cell.cell_rect_.GetTop() + cell.region_offset_.y_);
      if (cell.region_ != nullptr) {
        DrawTextRegion(cell.region_.get(), drawer);
      }
      canvas_->Restore();
    }
  }
  canvas_->Restore();
}

void MarkdownDrawer::DrawTextRegion(tttext::LayoutRegion* page,
                                    tttext::LayoutDrawer* drawer) {
  if (page != nullptr) {
    drawer->DrawLayoutPage(page);
  }
}

void MarkdownDrawer::DrawAttachment(const MarkdownPage& page,
                                    MarkdownTextAttachment* attachment) {
  const auto rects = MarkdownSelection::GetSelectionRectByCharPos(
      &page, attachment->start_index_, attachment->end_index_,
      MarkdownSelection::RectType::kLineBounding);
  attachment->DrawOnMultiLines(canvas_, rects);
}

void MarkdownDrawer::DrawAttachmentOnRegion(const MarkdownPage& page,
                                            MarkdownTextAttachment* attachment,
                                            int32_t region_char_start,
                                            int32_t region_char_end) {
  if (region_char_start > attachment->end_index_ ||
      region_char_end <= attachment->start_index_) {
    return;
  }
  DrawAttachment(page, attachment);
}

}  // namespace markdown
}  // namespace lynx
