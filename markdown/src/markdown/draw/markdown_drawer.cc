// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/draw/markdown_drawer.h"
#include "markdown/element/markdown_table.h"
namespace lynx {
namespace markdown {

void MarkdownDrawer::DrawPage(const lynx::markdown::MarkdownPage& page) {
  tttext::LayoutDrawer drawer(canvas_);
  canvas_->Save();
  canvas_->ClipRect(0, 0, std::min(page.GetLayoutWidth(), page.max_width_),
                    std::min(page.GetLayoutHeight(), page.max_height_), true);
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
  canvas_->ClipRect(0, 0, region_rect.GetWidth(), region_rect.GetHeight(),
                    true);
  canvas_->Translate(-region_rect.GetLeft(), -region_rect.GetTop());
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
  if (border.line_style_.line_.radius_ > 0) {
    canvas_->DrawRoundRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                           rect.GetBottom(), border.line_style_.line_.radius_,
                           painter.get());
  } else {
    canvas_->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                      rect.GetBottom(), painter.get());
  }
}

void MarkdownDrawer::DrawBorder(
    const lynx::markdown::MarkdownPageRegionBorder& border) {
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

void MarkdownDrawer::DrawTable(const lynx::markdown::MarkdownTableRegion& table,
                               const MarkdownElement& element,
                               tttext::LayoutDrawer* drawer) {
  if (table.Empty() || terminated_) {
    return;
  }
  // draw cell border
  auto painter = canvas_->CreatePainter();
  painter->SetStrokeColor(element.GetBorderStyle().border_color_);
  auto line_width = element.GetBorderStyle().border_width_;
  painter->SetStrokeWidth(line_width);
  auto radius = element.GetBorderStyle().border_radius_;
  canvas_->DrawRoundRect(
      line_width / 2, line_width / 2, table.total_width_ - line_width / 2,
      table.total_height_ - line_width / 2, radius, painter.get());
  for (int row = 0; row < table.GetRowCount() - 1; row++) {
    auto& cell = table.GetCell(row, 0);
    canvas_->DrawLine(0, cell.cell_rect_.GetBottom(), table.total_width_,
                      cell.cell_rect_.GetBottom(), painter.get());
  }
  for (int col = 0; col < table.GetColumnCount() - 1; col++) {
    auto& cell = table.GetCell(0, col);
    canvas_->DrawLine(cell.cell_rect_.GetRight(), 0, cell.cell_rect_.GetRight(),
                      table.total_height_, painter.get());
  }

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
}

void MarkdownDrawer::DrawTextRegion(tttext::LayoutRegion* page,
                                    tttext::LayoutDrawer* drawer) {
  if (page != nullptr) {
    drawer->DrawLayoutPage(page);
  }
}
}  // namespace markdown
}  // namespace lynx
