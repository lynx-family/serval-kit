// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/layout/markdown_layout_node.h"

#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/style/markdown_style.h"

#include <limits>
namespace serval::markdown {
MarkdownLayoutNode::MarkdownLayoutNode(MarkdownContext* context)
    : context_(context), background_() {}
MarkdownLayoutNode::~MarkdownLayoutNode() = default;

MarkdownLayoutNode::LayoutResult MarkdownLayoutNode::Layout(
    LayoutParams params) {
  layout_result_ = OnLayout(params);
  return layout_result_;
}
void MarkdownLayoutNode::Align(AlignParams params) {
  OnAlign(params);
}
void MarkdownLayoutNode::Render(RenderParams params) {
  OnRender(params);
}
int32_t MarkdownLayoutNode::GetCharCount() const {
  return layout_result_.char_count;
}
int32_t MarkdownLayoutNode::GetLineCount() const {
  return layout_result_.line_count;
}
Range MarkdownLayoutNode::GetCharRangeByPoint(PointF point,
                                              CharRangeType type) {
  auto ToChildPoint = [](PointF point, const MarkdownLayoutNode* child) {
    return point -
           PointF{child->layout_rect_.GetLeft(), child->layout_rect_.GetTop()};
  };
  auto FindPrevious = [](MarkdownNode* node) {
    auto* previous = reinterpret_cast<MarkdownLayoutNode*>(node);
    while (previous != nullptr &&
           (previous->GetCharCount() == 0 || !previous->has_layout_)) {
      previous = reinterpret_cast<MarkdownLayoutNode*>(previous->GetPrevious());
    }
    return previous;
  };
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (!layout_child->has_layout_ || layout_child->GetCharCount() == 0) {
      continue;
    }
    if (layout_child->layout_rect_.Contains(point.x_, point.y_)) {
      return layout_child->GetCharRangeByPoint(
          ToChildPoint(point, layout_child), type);
    }
    if (layout_child->layout_rect_.GetTop() > point.y_) {
      auto* previous = FindPrevious(layout_child->GetPrevious());
      if (previous != nullptr) {
        if (point.y_ - previous->layout_rect_.GetBottom() >
            layout_child->layout_rect_.GetTop() - point.y_) {
          return layout_child->GetCharRangeByPoint(
              ToChildPoint(point, layout_child), type);
        } else {
          return previous->GetCharRangeByPoint(ToChildPoint(point, previous),
                                               type);
        }
      }
    }
  }
  auto previous = FindPrevious(last_child_);
  if (previous != nullptr) {
    return previous->GetCharRangeByPoint(ToChildPoint(point, previous), type);
  }
  return {0, 0};
}
void MarkdownLayoutNode::GetSelectionRectByCharPos(std::vector<RectF>* result,
                                                   int32_t char_pos_start,
                                                   int32_t char_pos_end,
                                                   RectType type,
                                                   RectCoordinate coordinate) {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (layout_child->absolute_char_pos_ >= char_pos_end) {
      break;
    }
    if (layout_child->absolute_char_pos_ + layout_child->GetCharCount() <=
        char_pos_start) {
      continue;
    }
    layout_child->GetSelectionRectByCharPos(result, char_pos_start,
                                            char_pos_end, type, coordinate);
  }
}
void MarkdownLayoutNode::GetContentByCharPos(std::string* result,
                                             int32_t char_pos_start,
                                             int32_t char_pos_end) {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (layout_child->absolute_char_pos_ >= char_pos_end) {
      break;
    }
    if (layout_child->absolute_char_pos_ + layout_child->GetCharCount() <=
        char_pos_start) {
      continue;
    }
    layout_child->GetContentByCharPos(result, char_pos_start, char_pos_end);
  }
}
void MarkdownLayoutNode::GetLineEndCharIndices(std::vector<int32_t>* result) {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    layout_child->GetLineEndCharIndices(result);
  }
}

void MarkdownLayoutNode::CleanChildrenLayoutResult() const {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    layout_child->has_layout_ = false;
    layout_child->layout_rect_ = RectF::MakeEmpty();
  }
}

MarkdownLayoutNode::LayoutResult MarkdownLayoutNode::OnLayout(
    LayoutParams params) {
  const float inner_left = paddings_.left_ + borders_.left_.width_;
  const float inner_top = paddings_.top_ + borders_.top_.width_;
  const float inner_right = paddings_.right_ + borders_.right_.width_;
  const float inner_bottom = paddings_.bottom_ + borders_.bottom_.width_;

  CleanChildrenLayoutResult();

  return LayoutChildren(params, inner_left, inner_top, inner_right,
                        inner_bottom);
}

MarkdownLayoutNode::LayoutResult MarkdownLayoutNode::LayoutChildren(
    LayoutParams params, float offset_left, float offset_top,
    float offset_right, float offset_bottom) {
  if (params.max_lines == 0 || params.height <= offset_top + offset_bottom) {
    return {
        .overflow = true,
        .truncated = false,
    };
  }

  LayoutResult layout_result;
  float previous_left = offset_left;
  const float content_right = params.width - offset_right;

  float previous_top = offset_top;
  const float content_bottom = params.height - offset_bottom;

  float previous_margin = 0;
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    LayoutParams child_params;
    child_params.width_mode = params.width_mode;
    child_params.width = content_right - previous_left;
    child_params.height_mode = params.height_mode;
    child_params.height = content_bottom - previous_top;
    child_params.max_lines = params.max_lines - layout_result.line_count;
    child_params.last_element = params.last_element && child == last_child_;

    if (layout_main_axis_ == LayoutMainAxis::kHorizontal) {
      child_params.height -=
          layout_child->margins_.top_ + layout_child->margins_.bottom_;
      const float collapsed_margin =
          CollapseMargin(previous_margin, layout_child->margins_.left_);
      child_params.width -= collapsed_margin + layout_child->margins_.right_;
      previous_margin = layout_child->margins_.right_;
      previous_left += collapsed_margin;
      previous_top += layout_child->margins_.top_;
    } else {
      child_params.width -=
          layout_child->margins_.left_ + layout_child->margins_.right_;
      const float collapsed_margin =
          CollapseMargin(previous_margin, layout_child->margins_.top_);
      child_params.height -= collapsed_margin + layout_child->margins_.bottom_;
      previous_margin = layout_child->margins_.bottom_;
      previous_left += layout_child->margins_.left_;
      previous_top += collapsed_margin;
    }

    const auto child_result = layout_child->Layout(child_params);

    if (child_result.overflow && !child_result.truncated) {
      layout_result.overflow = true;
      layout_result.truncated = false;
      break;
    }

    layout_child->layout_rect_ = RectF::MakeLTWH(
        previous_left, previous_top, child_result.width, child_result.height);
    layout_child->has_layout_ = true;
    const float child_right =
        previous_left + child_result.width + layout_child->margins_.right_;
    const float child_bottom =
        previous_top + child_result.height + layout_child->margins_.bottom_;

    layout_result.width =
        std::max(layout_result.width, child_right + offset_right);
    layout_result.height =
        std::max(layout_result.height, child_bottom + offset_bottom);
    layout_result.char_count += child_result.char_count;
    layout_result.line_count += child_result.line_count;

    if (layout_main_axis_ == LayoutMainAxis::kHorizontal) {
      previous_left += child_result.width;
      previous_top -= layout_child->margins_.top_;
    } else {
      previous_left -= layout_child->margins_.left_;
      previous_top += child_result.height;
    }

    if (child_result.overflow) {
      layout_result.overflow = true;
      layout_result.truncated = child_result.truncated;
      break;
    }
  }

  if (layout_result.overflow && !layout_result.truncated) {
    layout_result_ = layout_result;
    if (ForceAddTruncation()) {
      layout_result_.truncated = true;
    }
    layout_result = layout_result_;
  }

  if (first_child_ != nullptr) {
    const auto first_layout_child =
        reinterpret_cast<MarkdownLayoutNode*>(first_child_);
    if (first_layout_child->has_layout_) {
      layout_result.baseline = offset_top + first_layout_child->margins_.top_ +
                               first_layout_child->layout_result_.baseline;
    }
  }

  return layout_result;
}

bool MarkdownLayoutNode::ForceAddTruncation() {
  for (auto child = last_child_; child != nullptr;
       child = child->GetPrevious()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (!layout_child->has_layout_) {
      continue;
    }
    float old_width = layout_child->layout_result_.width;
    if (layout_child->ForceAddTruncation()) {
      float new_width = layout_child->layout_result_.width;
      auto& margins = layout_child->margins_;
      if (layout_main_axis_ == LayoutMainAxis::kHorizontal) {
        layout_result_.width += new_width - old_width;
        layout_child->layout_rect_.SetWidth(new_width);
        for (auto after_child = layout_child->GetNext(); after_child != nullptr;
             after_child = after_child->GetNext()) {
          auto after_layout_child =
              reinterpret_cast<MarkdownLayoutNode*>(after_child);
          after_layout_child->layout_rect_.SetLeft(
              after_layout_child->layout_rect_.GetLeft() + new_width -
              old_width);
        }
      } else {
        layout_result_.width =
            std::max(layout_result_.width,
                     layout_child->layout_rect_.GetRight() + margins.right_ +
                         paddings_.right_ + borders_.right_.width_);
        layout_child->layout_rect_.SetWidth(new_width);
      }
      return true;
    }
  }
  return false;
}
void MarkdownLayoutNode::OnAlign(AlignParams params) {
  absolute_position_ = params.absolute_position;
  absolute_char_pos_ = params.absolute_char_pos;
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (!layout_child->has_layout_) {
      continue;
    }
    params.absolute_position =
        absolute_position_ + PointF{layout_child->layout_rect_.GetLeft(),
                                    layout_child->layout_rect_.GetTop()};
    layout_child->Align(params);
    params.absolute_char_pos += layout_child->GetCharCount();
  }
}
void MarkdownLayoutNode::OnRender(RenderParams params) {
  DrawBackground(params);
  DrawBorder(params);
  DrawChildren(params);
}
void MarkdownLayoutNode::DrawChildren(RenderParams params) const {
  for (auto child = first_child_; child != nullptr; child = child->GetNext()) {
    auto* layout_child = reinterpret_cast<MarkdownLayoutNode*>(child);
    if (!layout_child->has_layout_) {
      continue;
    }
    params.canvas->Save();
    params.canvas->Translate(layout_child->layout_rect_.GetLeft(),
                             layout_child->layout_rect_.GetTop());
    layout_child->Render(params);
    params.canvas->Restore();
    params.max_char_count -= layout_child->GetCharCount();
    if (params.max_char_count < 0) {
      break;
    }
  }
}
void MarkdownLayoutNode::DrawBackground(const RenderParams& params) const {
  auto radius = GetBorderRadius();
  auto painter = params.canvas->CreatePainter();
  if (background_.background_drawable_ == nullptr) {
    painter->SetFillColor(background_.background_color_);
    if (radius > 0) {
      params.canvas->DrawRoundRect(0, 0, layout_result_.width,
                                   layout_result_.height, radius,
                                   painter.get());
    } else {
      params.canvas->DrawRect(0, 0, layout_result_.width, layout_result_.height,
                              painter.get());
    }
  } else {
    const auto rect =
        RectF::MakeLTWH(0, 0, layout_result_.width, layout_result_.height);
    if (radius > 0) {
      MarkdownPath path;
      path.AddRoundRect(
          {.rect_ = rect, .radius_x_ = radius, .radius_y_ = radius});
      background_.background_drawable_->DrawOnPath(params.canvas, &path, rect,
                                                   painter.get());
    } else {
      background_.background_drawable_->DrawOnRect(params.canvas, rect,
                                                   painter.get());
    }
  }
}
void MarkdownLayoutNode::DrawBorder(const RenderParams& params) const {
  float radius = GetBorderRadius();
  auto painter = params.canvas->CreatePainter();
  if (radius > 0) {
    auto border_width = borders_.left_.width_;
    auto half_border_width = border_width / 2;
    painter->SetStrokeColor(borders_.left_.color_);
    painter->SetStrokeWidth(border_width);
    params.canvas->DrawRoundRect(half_border_width, half_border_width,
                                 layout_result_.width - half_border_width,
                                 layout_result_.height - half_border_width,
                                 radius, painter.get());
  } else {
    if (borders_.left_.type_ != MarkdownBorderType::kNone) {
      auto border_width = borders_.left_.width_;
      auto half_border_width = border_width / 2;
      painter->SetStrokeWidth(border_width);
      painter->SetStrokeColor(borders_.left_.color_);
      params.canvas->DrawLine(half_border_width, 0, half_border_width,
                              layout_result_.height, painter.get());
    }
    if (borders_.right_.type_ != MarkdownBorderType::kNone) {
      auto border_width = borders_.right_.width_;
      auto half_border_width = border_width / 2;
      painter->SetStrokeWidth(border_width);
      painter->SetStrokeColor(borders_.right_.color_);
      params.canvas->DrawLine(layout_result_.width - half_border_width, 0,
                              layout_result_.width - half_border_width,
                              layout_result_.height, painter.get());
    }
    if (borders_.top_.type_ != MarkdownBorderType::kNone) {
      auto border_width = borders_.top_.width_;
      auto half_border_width = border_width / 2;
      painter->SetStrokeWidth(border_width);
      painter->SetStrokeColor(borders_.top_.color_);
      params.canvas->DrawLine(0, half_border_width, layout_result_.width,
                              half_border_width, painter.get());
    }
    if (borders_.bottom_.type_ != MarkdownBorderType::kNone) {
      auto border_width = borders_.bottom_.width_;
      auto half_border_width = border_width / 2;
      painter->SetStrokeWidth(border_width);
      painter->SetStrokeColor(borders_.bottom_.color_);
      params.canvas->DrawLine(
          0, layout_result_.height - half_border_width, layout_result_.width,
          layout_result_.height - half_border_width, painter.get());
    }
  }
}
float MarkdownLayoutNode::GetBorderRadius() const {
  // temporary only support round rect
  if (borders_.left_.type_ != MarkdownBorderType::kNone &&
      borders_.right_.type_ != MarkdownBorderType::kNone &&
      borders_.bottom_.type_ != MarkdownBorderType::kNone &&
      borders_.top_.type_ != MarkdownBorderType::kNone) {
    return borders_.left_.radius_top_;
  }
  return 0;
}
float MarkdownLayoutNode::CollapseMargin(float margin_1, float margin_2) const {
  if (!collapse_margin_) {
    return margin_1 + margin_2;
  }
  if (margin_1 > 0 && margin_2 > 0) {
    return std::max(margin_1, margin_2);
  }
  if (margin_1 < 0 && margin_2 < 0) {
    return std::min(margin_1, margin_2);
  }
  return margin_1 + margin_2;
}
tttext::TextLine* MarkdownLayoutNode::GetMarkerBaseLine() {
  if (first_child_ == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<MarkdownLayoutNode*>(first_child_)
      ->GetMarkerBaseLine();
}
void MarkdownLayoutNode::EnableCollapseMargin(bool enable) {
  collapse_margin_ = enable;
}
bool MarkdownLayoutNode::HasLayout() const {
  return has_layout_;
}
RectF MarkdownLayoutNode::GetLayoutRect() const {
  return layout_rect_;
}
void MarkdownLayoutNode::UpdateLayoutRect(RectF rect) {
  layout_rect_ = rect;
}
}  // namespace serval::markdown
