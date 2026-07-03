// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_quote.h"

#include <algorithm>
#include <cmath>

namespace serval::markdown {

MarkdownLayoutQuote::MarkdownLayoutQuote(MarkdownContext* context)
    : MarkdownLayoutNode(context) {}

MarkdownLayoutQuote::~MarkdownLayoutQuote() = default;

void MarkdownLayoutQuote::OnRender(RenderParams params) {
  DrawBackground(params);
  DrawQuoteLine(params);
  DrawChildren(params);
}

void MarkdownLayoutQuote::DrawQuoteLine(const RenderParams& params) const {
  if (params.canvas == nullptr) {
    return;
  }
  const auto& line = line_style_.line_;
  if (line.line_type_ == MarkdownLineType::kNone) {
    return;
  }
  const float line_width =
      line.width_ > 0 ? line.width_ : borders_.left_.width_;
  if (line_width <= 0) {
    return;
  }
  const uint32_t color = line.color_ != 0
                             ? line.color_
                             : static_cast<uint32_t>(borders_.left_.color_);
  auto painter = params.canvas->CreatePainter();
  painter->SetFillColor(color);
  const auto rect = RectF::MakeLTWH(
      line_width / 2, std::min(line.shrink_, layout_result_.height), line_width,
      std::max(0.f, layout_result_.height - line.shrink_ * 2));
  if (rect.GetHeight() <= 0) {
    return;
  }
  if (line.line_type_ == MarkdownLineType::kSolid) {
    if (line.radius_ > 0) {
      params.canvas->DrawRoundRect(rect.GetLeft(), rect.GetTop(),
                                   rect.GetRight(), rect.GetBottom(),
                                   line.radius_, painter.get());
    } else {
      params.canvas->DrawRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                              rect.GetBottom(), painter.get());
    }
    return;
  }
  if (line.line_type_ != MarkdownLineType::kDashed) {
    return;
  }

  constexpr float kDefaultDashLength = 2.5f;
  constexpr float kDefaultGapLength = 1.5f;
  const auto nums = static_cast<int>(
      std::floor(rect.GetHeight() / (kDefaultDashLength + kDefaultGapLength)));
  const int num_dashes = std::max(nums, 1);
  float gap_len = kDefaultDashLength;
  const float dash_len = kDefaultDashLength;
  if (num_dashes > 1) {
    const int num_gaps = num_dashes - 1;
    gap_len = (rect.GetHeight() - dash_len * nums) / num_gaps;
  }
  for (int index = 0; index < num_dashes; index++) {
    const float top = rect.GetTop() + index * (dash_len + gap_len);
    const float bottom = std::min(top + dash_len, rect.GetBottom());
    if (line.radius_ > 0) {
      params.canvas->DrawRoundRect(rect.GetLeft(), top, rect.GetRight(), bottom,
                                   line.radius_, painter.get());
    } else {
      params.canvas->DrawRect(rect.GetLeft(), top, rect.GetRight(), bottom,
                              painter.get());
    }
  }
}

}  // namespace serval::markdown
