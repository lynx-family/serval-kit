// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_list_item.h"

#include <algorithm>

namespace serval::markdown {

MarkdownLayoutListItem::MarkdownLayoutListItem(MarkdownContext* context)
    : MarkdownLayoutNode(context) {}

MarkdownLayoutListItem::~MarkdownLayoutListItem() = default;

MarkdownLayoutNode::LayoutResult MarkdownLayoutListItem::OnLayout(
    LayoutParams params) {
  const float inner_left = paddings_.left_ + borders_.left_.width_;
  const float inner_top = paddings_.top_ + borders_.top_.width_;
  const float inner_right = paddings_.right_ + borders_.right_.width_;
  const float inner_bottom = paddings_.bottom_ + borders_.bottom_.width_;

  marker_measure_result_ = {};
  marker_offset_ = {};
  if (marker_ != nullptr) {
    marker_measure_result_ =
        marker_->Measure({.width_ = params.width - inner_left - inner_right,
                          .width_mode_ = tttext::LayoutMode::kAtMost,
                          .height_ = params.height - inner_top - inner_bottom,
                          .height_mode_ = tttext::LayoutMode::kAtMost});
  }
  const float marker_width = marker_measure_result_.width_;

  CleanChildrenLayoutResult();
  auto layout_result = LayoutChildren(params, inner_left + marker_width,
                                      inner_top, inner_right, inner_bottom);

  if (marker_ != nullptr) {
    auto first_line = GetMarkerBaseLine();
    if (first_line != nullptr) {
      float line_top = layout_result.baseline - first_line->GetLineBaseLine();
      float line_bottom = line_top + first_line->GetLineHeight();
      float marker_top;
      if (marker_align_ == MarkdownVerticalAlign::kBaseline) {
        marker_top = layout_result.baseline - marker_measure_result_.baseline_;
      } else if (marker_align_ == MarkdownVerticalAlign::kCenter) {
        marker_top =
            (line_top + line_bottom) / 2 - marker_measure_result_.height_ / 2;
      } else if (marker_align_ == MarkdownVerticalAlign::kTop) {
        marker_top = line_top;
      } else {
        marker_top = line_bottom - marker_measure_result_.height_;
      }
      marker_offset_ = {inner_left, marker_top};
    } else {
      marker_offset_ = {inner_left, inner_top};
    }

    layout_result.height = std::max(
        layout_result.height,
        marker_offset_.y_ + marker_measure_result_.height_ + inner_bottom);
  }

  return layout_result;
}

void MarkdownLayoutListItem::OnRender(RenderParams params) {
  DrawBackground(params);
  DrawBorder(params);
  if (marker_ != nullptr) {
    marker_->Draw(params.canvas, marker_offset_.x_, marker_offset_.y_);
  }
  DrawChildren(params);
}

}  // namespace serval::markdown
