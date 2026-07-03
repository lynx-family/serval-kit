// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_paragraph.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "markdown/element/markdown_context.h"
#include "markdown/utils/markdown_platform.h"
#include "markdown/utils/markdown_string_utils.h"

namespace serval::markdown {

MarkdownLayoutParagraph::MarkdownLayoutParagraph(MarkdownContext* context)
    : MarkdownLayoutNode(context) {}

MarkdownLayoutParagraph::MarkdownLayoutParagraph(
    MarkdownContext* context, std::unique_ptr<tttext::Paragraph> paragraph)
    : MarkdownLayoutNode(context), paragraph_(std::move(paragraph)) {}

MarkdownLayoutParagraph::~MarkdownLayoutParagraph() = default;

MarkdownLayoutNode::LayoutResult MarkdownLayoutParagraph::OnLayout(
    LayoutParams params) {
  layout_region_ = nullptr;
  if (paragraph_ == nullptr || paragraph_->GetCharCount() == 0) {
    return {};
  }
  const float inner_left = paddings_.left_ + borders_.left_.width_;
  const float inner_top = paddings_.top_ + borders_.top_.width_;
  const float inner_right = paddings_.right_ + borders_.right_.width_;
  const float inner_bottom = paddings_.bottom_ + borders_.bottom_.width_;
  region_offset_ = {inner_left, inner_top};
  if (params.height <= inner_top + inner_bottom || params.max_lines == 0) {
    return {.overflow = true, .truncated = false};
  }
  LayoutResult result;
  bool full_filled = false;
  if (context_ != nullptr) {
    auto* text_layout = context_->GetTextLayout();
    if (text_layout != nullptr) {
      tttext::TTTextContext tt_context;
      tt_context.SetLastLineCanOverflow(text_overflow_ ==
                                        MarkdownTextOverflow::kClip);
      layout_region_ = std::make_unique<tttext::LayoutRegion>(
          params.width - inner_left - inner_right,
          params.height - inner_top - inner_bottom, params.width_mode,
          params.height_mode);
      const auto current_para_max_lines =
          paragraph_->GetParagraphStyle().GetMaxLines();
      bool use_global_max_line = false;
      if (params.max_lines >= 0 &&
          static_cast<uint32_t>(params.max_lines) <= current_para_max_lines) {
        paragraph_->GetParagraphStyle().SetMaxLines(params.max_lines);
        use_global_max_line = true;
      }
      auto layout_result = text_layout->LayoutEx(
          paragraph_.get(), layout_region_.get(), tt_context);
      paragraph_->GetParagraphStyle().SetMaxLines(current_para_max_lines);

      if (layout_region_->IsEmpty()) {
        result.baseline = inner_top;
        result.char_count = 0;
      } else {
        result.baseline =
            inner_top + layout_region_->GetLine(0)->GetLineBaseLine();
        result.char_count = static_cast<int32_t>(
            layout_region_->GetLine(layout_region_->GetLineCount() - 1)
                ->GetEndCharPos());
      }
      result.line_count = static_cast<int32_t>(layout_region_->GetLineCount());
      if (paragraph_->GetCharCount() > result.char_count) {
        if (use_global_max_line ||
            (layout_result == tttext::LayoutResult::kBreakPage &&
             !layout_region_->DidExceedMaxLines())) {
          full_filled = true;
        }
      }
    }
  }
  if (layout_region_ == nullptr) {
    return {.overflow = false, .truncated = false};
  }

  if (!full_filled && last_line_align_ != MarkdownTextAlign::kUndefined) {
    tttext::ParagraphHorizontalAlignment converted_align =
        tttext::ParagraphHorizontalAlignment::kLeft;
    switch (last_line_align_) {
      case MarkdownTextAlign::kCenter:
        converted_align = tttext::ParagraphHorizontalAlignment::kCenter;
        break;
      case MarkdownTextAlign::kRight:
        converted_align = tttext::ParagraphHorizontalAlignment::kRight;
        break;
      case MarkdownTextAlign::kJustify:
        converted_align = tttext::ParagraphHorizontalAlignment::kJustify;
        break;
      case MarkdownTextAlign::kLeft:
      case MarkdownTextAlign::kUndefined:
      default:
        converted_align = tttext::ParagraphHorizontalAlignment::kLeft;
        break;
    }
    for (uint32_t index = 0; index < layout_region_->GetLineCount(); index++) {
      auto* line = layout_region_->GetLine(index);
      if (line->IsLastLineOfParagraph()) {
        line->ModifyHorizontalAlignment(converted_align);
      }
    }
  }
  result.width =
      MarkdownPlatform::GetMdLayoutRegionWidth(layout_region_.get()) +
      inner_left + inner_right;
  result.height =
      MarkdownPlatform::GetMdLayoutRegionHeight(layout_region_.get()) +
      inner_top + inner_bottom;
  result.overflow = full_filled;
  result.truncated = full_filled;
  return result;
}

void MarkdownLayoutParagraph::OnRender(RenderParams params) {
  DrawBackground(params);
  DrawBorder(params);
  if (layout_region_ == nullptr) {
    return;
  }
  params.canvas->Translate(region_offset_.x_, region_offset_.y_);
  tttext::LayoutDrawer drawer(params.canvas);
  // todo: max char count
  drawer.DrawLayoutPage(layout_region_.get());
  params.canvas->Translate(-region_offset_.x_, -region_offset_.y_);
}

bool MarkdownLayoutParagraph::ForceAddTruncation() {
  if (layout_region_ == nullptr || layout_region_->IsEmpty()) {
    return false;
  }
  auto* line = layout_region_->GetLine(layout_region_->GetLineCount() - 1);
  if (line == nullptr) {
    return false;
  }
  const float width_before =
      MarkdownPlatform::GetMdLayoutRegionWidth(layout_region_.get());
  line->StripByEllipsis(nullptr);
  layout_region_->UpdateLayoutedSize(line, tttext::TTTextContext());
  const float width_after =
      MarkdownPlatform::GetMdLayoutRegionWidth(layout_region_.get());
  layout_result_.width += std::max(0.f, width_after - width_before);
  return true;
}

int32_t MarkdownLayoutParagraph::FindLineByY(float y) const {
  for (int index = 0; index < layout_region_->GetLineCount(); index++) {
    auto* line = layout_region_->GetLine(index);
    if (line->GetLineBottom() < y) {
      continue;
    }
    if (line->GetLineTop() <= y && line->GetLineBottom() >= y) {
      return index;
    }
    if (line->GetLineTop() > y) {
      if (index == 0) {
        return 0;
      }
      auto previous_line = layout_region_->GetLine(index - 1);
      float diff_previous = y - previous_line->GetLineBottom();
      float diff = line->GetLineTop() - y;
      if (diff > diff_previous) {
        return index - 1;
      } else {
        return index;
      }
    }
  }
  return static_cast<int32_t>(layout_region_->GetLineCount() - 1);
}

Range MarkdownLayoutParagraph::GetCharRangeByPoint(PointF point,
                                                   CharRangeType type) {
  if (layout_region_ == nullptr || layout_region_->IsEmpty() ||
      paragraph_ == nullptr || layout_result_.char_count == 0) {
    return {absolute_char_pos_, absolute_char_pos_};
  }
  if (type == CharRangeType::kParagraph) {
    return {absolute_char_pos_, absolute_char_pos_ + layout_result_.char_count};
  }
  point -= region_offset_;
  const int32_t line_index = FindLineByY(point.y_);
  const auto* line = layout_region_->GetLine(static_cast<uint32_t>(line_index));
  auto char_pos = static_cast<int32_t>(line->GetStartCharPos() +
                                       line->GetCharPosByCoordinateX(point.x_));
  if (type == CharRangeType::kSentence) {
    auto content_string =
        paragraph_->GetContentString(0, paragraph_->GetCharCount());
    auto [sentence_start, sentence_end] =
        ExpandToSentence(content_string, char_pos);
    return {
        .start_ =
            absolute_char_pos_ + static_cast<int32_t>(CIndexToUTF8Index(
                                     content_string.data(),
                                     content_string.length(), sentence_start)),
        .end_ =
            absolute_char_pos_ +
            static_cast<int32_t>(CIndexToUTF8Index(
                content_string.data(), content_string.length(), sentence_end)),
    };
  }
  if (type == CharRangeType::kWord) {
    const auto word =
        paragraph_->GetWordBoundary(static_cast<uint32_t>(char_pos));
    return {absolute_char_pos_ + static_cast<int32_t>(word.first),
            absolute_char_pos_ + static_cast<int32_t>(word.second)};
  }
  return {absolute_char_pos_ + char_pos, absolute_char_pos_ + char_pos + 1};
}

std::pair<int32_t, int32_t> MarkdownLayoutParagraph::ExpandToSentence(
    std::string_view content_string, int32_t index) {
  static constexpr std::string_view patterns[] = {
      "。", "？", "\n", "\r", "！", "……", ". ", "? ", "! ", "; ", "；",
  };
  const auto byte_pos_start =
      UTF8IndexToCIndex(content_string.data(), content_string.length(),
                        static_cast<size_t>(index));
  size_t sentence_start = std::string::npos;
  size_t sentence_end = std::string::npos;
  size_t sentence_end_length = 0;
  for (const auto& pattern : patterns) {
    const auto sentence_before_end =
        content_string.rfind(pattern, byte_pos_start);
    if (sentence_before_end != std::string::npos) {
      if (sentence_before_end + pattern.length() >= byte_pos_start) {
        sentence_end = sentence_before_end;
        sentence_end_length = pattern.length();
      } else {
        sentence_start = sentence_before_end + pattern.length();
      }
      break;
    }
  }
  if (sentence_start == std::string::npos &&
      sentence_end == std::string::npos) {
    sentence_start = 0;
  }
  if (sentence_start == std::string::npos) {
    if (sentence_end == 0) {
      sentence_start = 0;
    } else {
      for (const auto& pattern : patterns) {
        const auto sentence_before_end =
            content_string.rfind(pattern, sentence_end - 1);
        if (sentence_before_end != std::string::npos) {
          sentence_start = sentence_before_end + pattern.length();
          break;
        }
      }
    }
    if (sentence_start == std::string::npos) {
      sentence_start = 0;
    }
  } else {
    for (const auto& pattern : patterns) {
      const auto current_sentence_end =
          content_string.find(pattern, byte_pos_start);
      if (current_sentence_end != std::string::npos) {
        sentence_end = current_sentence_end;
        sentence_end_length = pattern.length();
        break;
      }
    }
    if (sentence_end == std::string::npos) {
      sentence_end = content_string.length();
      sentence_end_length = 0;
    }
  }
  return {sentence_start, sentence_end + sentence_end_length};
}

void MarkdownLayoutParagraph::GetSelectionRectByCharPos(
    std::vector<RectF>* result, int32_t char_pos_start, int32_t char_pos_end,
    RectType type, RectCoordinate coordinate) {
  if (layout_region_ == nullptr || layout_region_->IsEmpty() ||
      result == nullptr) {
    return;
  }
  char_pos_start -= absolute_char_pos_;
  char_pos_end -= absolute_char_pos_;
  PointF offset = absolute_position_ + region_offset_;
  if (coordinate == RectCoordinate::kRelative) {
    offset = {0, 0};
  }
  for (uint32_t index = 0; index < layout_region_->GetLineCount(); index++) {
    auto* line = layout_region_->GetLine(index);
    if (static_cast<int32_t>(line->GetEndCharPos()) <= char_pos_start) {
      continue;
    }
    if (static_cast<int32_t>(line->GetStartCharPos()) >= char_pos_end) {
      break;
    }
    float rect[4]{0, 0, 0, 0};
    const int32_t char_start =
        std::max(char_pos_start, static_cast<int32_t>(line->GetStartCharPos()));
    const int32_t char_end =
        std::min(char_pos_end, static_cast<int32_t>(line->GetEndCharPos()));
    line->GetBoundingRectByCharRange(rect, char_start, char_end);
    auto& [left, top, width, height] = rect;
    if (width <= 0 || height <= 0) {
      continue;
    }
    if (type == RectType::kSelection || type == RectType::kLineBounding) {
      result->emplace_back(RectF::MakeLTRB(
          left + offset.x_, line->GetLineTop() + offset.y_,
          left + width + offset.x_, line->GetLineBottom() + offset.y_));
    } else {
      result->emplace_back(
          RectF::MakeLTWH(left + offset.x_, top + offset.y_, width, height));
    }
  }
}

void MarkdownLayoutParagraph::GetContentByCharPos(std::string* result,
                                                  int32_t char_pos_start,
                                                  int32_t char_pos_end) {
  if (layout_region_ == nullptr || layout_region_->IsEmpty() ||
      result == nullptr) {
    return;
  }
  auto* paragraph = layout_region_->GetLine(0)->GetParagraph();
  char_pos_start = std::max(0, char_pos_start - absolute_char_pos_);
  char_pos_end =
      std::min(char_pos_end - absolute_char_pos_, layout_result_.char_count);
  const int32_t char_count = char_pos_end - char_pos_start;
  if (char_count <= 0) {
    return;
  }
  result->append(paragraph->GetContentString(char_pos_start, char_count));
  if (char_pos_end >= static_cast<int32_t>(paragraph->GetCharCount())) {
    result->append("\n");
  }
}

void MarkdownLayoutParagraph::GetLineEndCharIndices(
    std::vector<int32_t>* result) {
  if (layout_region_ == nullptr || layout_region_->IsEmpty() ||
      result == nullptr) {
    return;
  }
  for (uint32_t index = 0; index < layout_region_->GetLineCount(); index++) {
    auto* line = layout_region_->GetLine(index);
    result->emplace_back(absolute_char_pos_ +
                         static_cast<int32_t>(line->GetEndCharPos()));
  }
}

}  // namespace serval::markdown
