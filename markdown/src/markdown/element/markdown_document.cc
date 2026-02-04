// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/element/markdown_document.h"

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/markdown_parser.h"
namespace lynx {
namespace markdown {
const MarkdownLink* MarkdownDocument::GetLinkByTouchPosition(PointF point) {
  if (links_.empty()) {
    return nullptr;
  }
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return nullptr;
  }
  auto touch_range = MarkdownSelection::GetCharRangeByPoint(
      page.get(), point, MarkdownSelection::CharRangeType::kChar);
  for (const auto& link : links_) {
    if (touch_range.start_ >= static_cast<int32_t>(link.char_start_) &&
        touch_range.start_ <
            static_cast<int32_t>(link.char_start_ + link.char_count_)) {
      return &link;
    }
  }
  return nullptr;
}

Range MarkdownDocument::GetCharRangeByViewRect(RectF view_rect) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return {};
  }
  PointF start{view_rect.GetLeft(), view_rect.GetTop()};
  PointF end{view_rect.GetRight(), view_rect.GetBottom()};

  auto touch_start = MarkdownSelection::GetCharRangeByPoint(
      page.get(), start, MarkdownSelection::CharRangeType::kChar);
  auto touch_end = MarkdownSelection::GetCharRangeByPoint(
      page.get(), end, MarkdownSelection::CharRangeType::kChar);
  auto touch_range = Range{touch_start.start_, touch_end.end_};
  return touch_range;
}

std::vector<MarkdownLink*> MarkdownDocument::GetLinksByViewRect(
    RectF view_rect) {
  if (links_.empty()) {
    return {};
  }
  auto touch_range = GetCharRangeByViewRect(view_rect);
  if (touch_range.start_ == touch_range.end_)
    return {};
  std::vector<MarkdownLink*> result;
  for (auto& link : links_) {
    if (touch_range.start_ >=
            static_cast<int32_t>(link.char_start_ + link.char_count_) ||
        touch_range.end_ <= static_cast<int32_t>(link.char_start_)) {
      continue;
    }
    result.emplace_back(&link);
  }
  return result;
}

std::vector<MarkdownImage*> MarkdownDocument::GetImageByViewRect(
    RectF view_rect) {
  if (images_.empty()) {
    return {};
  }
  auto touch_range = GetCharRangeByViewRect(view_rect);
  std::vector<MarkdownImage*> result;
  for (auto& image : images_) {
    if (image.char_index_ >= touch_range.start_ &&
        image.char_index_ < touch_range.end_) {
      result.emplace_back(&image);
    }
  }
  return result;
}

PointF MarkdownDocument::GetElementOrigin(int32_t char_index, bool is_block) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return PointF{0.f, 0.f};
  }
  std::vector<RectF> rects = MarkdownSelection::GetSelectionRectByCharPos(
      page.get(), char_index, char_index + 1,
      MarkdownSelection::RectType::kCharBounding);
  if (!rects.empty()) {
    float x = is_block ? 0 : rects[0].GetLeft();
    return PointF{x, rects[0].GetTop()};
  }
  return PointF{0.f, 0.f};
}

PointF MarkdownDocument::GetTruncationOrigin() {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return PointF{0.f, 0.f};
  }
  auto char_count = MarkdownSelection::GetPageCharCount(page.get());
  if (static_cast<uint32_t>(char_count) >=
      para_vec_.back()->GetCharStart() + para_vec_.back()->GetCharCount()) {
    return {0.f, 0.f};
  }
  auto last_region = page->regions_.back().get();
  auto last_element = last_region->element_.get();
  if (last_element->GetType() == MarkdownElementType::kParagraph) {
    auto left_offset = last_region->rect_.GetLeft();
    auto top_offset = last_region->rect_.GetTop();
    return {truncation_delegate_->GetXOffset() + left_offset,
            top_offset + truncation_delegate_->GetYOffset()};
  }
  return {0.f, 0.f};
}

std::pair<float, float> MarkdownDocument::GetInlineViewOrigin(
    const char* idSelector) {
  for (auto& inline_view : inline_views_) {
    if (idSelector == inline_view.id_) {
      auto pos =
          GetElementOrigin(inline_view.char_index_, inline_view.is_block_view_);
      return std::make_pair(pos.x_, pos.y_);
    }
  }

  if (truncation_delegate_ != nullptr &&
      style_.truncation_.truncation_.content_ == idSelector) {
    // is truncation view
    auto pos = GetTruncationOrigin();
    return std::make_pair(pos.x_, pos.y_);
  }

  return std::make_pair(0.f, 0.f);
}

std::vector<std::pair<std::string, PointF>>
MarkdownDocument::GetAllInlineViewOrigin() {
  std::vector<std::pair<std::string, PointF>> inline_views;
  inline_views.reserve(inline_views_.size() + 1);
  for (auto& item : inline_views_) {
    inline_views.emplace_back(
        item.id_, GetElementOrigin(item.char_index_, item.is_block_view_));
  }
  if (truncation_delegate_ != nullptr) {
    // is truncation view
    auto pos = GetTruncationOrigin();
    if (pos != PointF{0, 0}) {
      inline_views.emplace_back(style_.truncation_.truncation_.content_, pos);
    }
  }
  return inline_views;
}

std::vector<std::string> MarkdownDocument::GetAllInlineViewId() {
  std::vector<std::string> inline_views;
  inline_views.reserve(inline_views_.size() + 1);
  for (auto& item : inline_views_) {
    inline_views.emplace_back(item.id_);
  }
  if (truncation_delegate_ != nullptr) {
    // is truncation view
    inline_views.emplace_back(style_.truncation_.truncation_.content_);
  }
  return inline_views;
}

std::string MarkdownDocument::GetContentByCharPos(int32_t char_pos_start,
                                                  int32_t char_pos_end) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return "";
  }
  return MarkdownSelection::GetContentByCharPos(
      page.get(), char_pos_start, char_pos_end, &shape_run_alt_strings_);
}

void MarkdownDocument::SetShapeRunAltString(uint32_t char_offset,
                                            std::string_view content) {
  shape_run_alt_strings_.emplace_back(char_offset, content);
}

void MarkdownDocument::ClearForParse() {
  inline_views_.clear();
  links_.clear();
  para_vec_.clear();
  border_attachments_.clear();
  shape_run_alt_strings_.clear();
  quote_range_.clear();
  images_.clear();
}

void MarkdownDocument::UpdateTruncation(float width) {
  if (style_.truncation_.truncation_.truncation_type_ ==
      MarkdownTruncationType::kText) {
    truncation_text_ =
        base::U8StringToU16(style_.truncation_.truncation_.content_);
    truncation_delegate_ = nullptr;
  } else if (style_.truncation_.truncation_.truncation_type_ ==
             MarkdownTruncationType::kView) {
    truncation_delegate_ = loader_->LoadInlineView(
        style_.truncation_.truncation_.content_.c_str(), width, 1e5);
    truncation_text_.clear();
  }
}

std::vector<Range> MarkdownDocument::GetSyntaxSourceRanges(
    std::string_view tag) {
  std::vector<Range> ranges;
  auto type = TagToSyntaxType(tag);
  if (type == MarkdownSyntaxType::kUndefined)
    return ranges;
  for (auto& para : para_vec_) {
    if (para->GetMarkdownSourceType() == type) {
      ranges.emplace_back(para->GetMarkdownSourceRange());
    }
  }
  return ranges;
}

MarkdownSyntaxType MarkdownDocument::TagToSyntaxType(std::string_view tag) {
  static const std::vector<std::pair<std::string_view, MarkdownSyntaxType>>
      tags_map = {{"codeBlock", MarkdownSyntaxType::kCodeBlock},
                  {"quote", MarkdownSyntaxType::kQuote},
                  {"unorderedList", MarkdownSyntaxType::kUnorderedList},
                  {"orderedList", MarkdownSyntaxType::kOrderedList},
                  {"table", MarkdownSyntaxType::kTable}};
  for (auto& [str, type] : tags_map) {
    if (tag == str) {
      return type;
    }
  }
  return MarkdownSyntaxType::kUndefined;
}

std::vector<std::string> MarkdownDocument::GetAllImageUrl() {
  std::vector<std::string> images;
  images.reserve(images_.size());
  for (auto& image : images_) {
    images.emplace_back(image.url_);
  }
  return images;
}

std::string MarkdownDocument::GetImageByTouchPosition(
    lynx::markdown::PointF point) {
  if (images_.empty()) {
    return "";
  }
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return "";
  }
  auto touch_range = MarkdownSelection::GetCharRangeByPoint(
      page.get(), point, MarkdownSelection::CharRangeType::kChar);
  for (const auto& image : images_) {
    if (touch_range.start_ == static_cast<int32_t>(image.char_index_)) {
      return image.url_;
    }
  }
  return "";
}

bool MarkdownDocument::TouchPointCanScroll(PointF point, float safe_offset) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return false;
  }
  auto region_index =
      MarkdownSelection::FindClosestRegionIndex(page.get(), point.y_);
  auto* region = page->regions_[region_index].get();
  if (region->scroll_x_) {
    auto view_rect = region->scroll_x_view_rect_;
    auto rect = RectF::MakeLTWH(
        view_rect.GetLeft() + safe_offset, view_rect.GetTop(),
        view_rect.GetWidth() - 2 * safe_offset, view_rect.GetHeight());
    if (rect.Contains(point.x_, point.y_)) {
      return true;
    }
  }
  return false;
}

MarkdownTouchState MarkdownDocument::OnTouchEvent(
    lynx::markdown::MarkdownTouchEventType type, lynx::markdown::PointF point) {
  touch_state_ = MarkdownTouchState::kNone;
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return touch_state_;
  }
  if (type == MarkdownTouchEventType::kDown) {
    touch_down_ = true;
    touch_down_point_ = point;
    auto region_index =
        MarkdownSelection::FindClosestRegionIndex(page.get(), point.y_);
    auto* region = page->regions_[region_index].get();
    if (region->scroll_x_ &&
        region->scroll_x_view_rect_.Contains(point.x_, point.y_)) {
      touch_down_region_index_ = region_index;
      touch_down_region_origin_scroll_offset_ = region->scroll_x_offset_;
    } else {
      touch_down_region_index_ = -1;
    }
  } else if (type == MarkdownTouchEventType::kMove) {
    if (touch_down_ && touch_down_region_index_ >= 0 &&
        static_cast<uint32_t>(touch_down_region_index_) <
            page->regions_.size()) {
      auto* touch_down_region = page->regions_[touch_down_region_index_].get();
      float x_offset = point.x_ - touch_down_point_.x_;
      if (touch_down_region->scroll_x_ &&
          (touch_state_ == MarkdownTouchState::kOnScroll ||
           std::abs(x_offset) > 1)) {
        x_offset += touch_down_region_origin_scroll_offset_;
        auto outer_rect = touch_down_region->border_ == nullptr
                              ? touch_down_region->rect_
                              : touch_down_region->border_->rect_;
        float min_offset = touch_down_region->scroll_x_view_rect_.GetRight() -
                           outer_rect.GetRight();
        float max_offset = touch_down_region->scroll_x_view_rect_.GetLeft() -
                           outer_rect.GetLeft();
        x_offset = std::min(max_offset, x_offset);
        x_offset = std::max(min_offset, x_offset);
        if (x_offset != touch_down_region->scroll_x_offset_) {
          touch_down_region->scroll_x_offset_ = x_offset;
          touch_state_ = MarkdownTouchState::kOnScroll;
        }
      }
    }
  } else {
    touch_down_ = false;
  }
  return touch_state_;
}

void MarkdownDocument::ApplyStyleInRange(const MarkdownBaseStylePart& style,
                                         Range range) {
  for (const auto& para : para_vec_) {
    if (para->GetCharStart() > range.end_) {
      break;
    }
    if (para->GetCharStart() + para->GetCharCount() < range.start_) {
      continue;
    }
    if (para->GetType() != MarkdownElementType::kParagraph) {
      continue;
    }
    uint32_t start =
        std::max(static_cast<uint32_t>(range.start_), para->GetCharStart());
    uint32_t end = std::min(static_cast<uint32_t>(range.end_),
                            para->GetCharStart() + para->GetCharCount());
    auto count = end - start;
    start -= para->GetCharStart();
    auto paragraph =
        static_cast<MarkdownParagraphElement*>(para.get())->GetParagraph();
    tttext::Style run_style;
    MarkdownParser::SetTTStyleByMarkdownBaseStyle(this, style, &run_style);
    paragraph->ApplyStyleInRange(run_style, start, count);
  }
}

int32_t MarkdownDocument::MarkdownOffsetToCharOffset(int32_t markdown_offset) {
  auto iter =
      std::lower_bound(markdown_index_to_char_index_.begin(),
                       markdown_index_to_char_index_.end(), markdown_offset,
                       [](const std::pair<Range, Range>& pair, int32_t offset) {
                         return pair.first.end_ <= offset;
                       });
  if (iter == markdown_index_to_char_index_.end()) {
    return markdown_index_to_char_index_.back().second.end_;
  }
  if (markdown_offset < iter->first.start_) {
    return iter->second.start_;
  }
  if (markdown_offset > iter->first.end_) {
    return iter->second.end_;
  }
  return iter->second.start_ + (markdown_offset - iter->first.start_);
}

int32_t MarkdownDocument::GetCharIndexByTouchPosition(
    lynx::markdown::PointF point) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return -1;
  }
  auto touch_range = MarkdownSelection::GetCharRangeByPoint(
      page.get(), point, MarkdownSelection::CharRangeType::kChar);
  return touch_range.start_;
}

const MarkdownTextAttachment*
MarkdownDocument::GetTextClickRangeByTouchPosition(
    lynx::markdown::PointF position) {
  auto page = GetPage();
  const auto& attachments = page->GetTextAttachments();
  if (attachments.empty())
    return nullptr;
  auto index = GetCharIndexByTouchPosition(position);
  for (const auto& range : attachments) {
    if (!range->id_.empty() && index >= range->start_index_ &&
        index < range->end_index_) {
      return range.get();
    }
  }
  return nullptr;
}

int32_t MarkdownDocument::GetRegionIndexByCharIndex(int32_t char_index) {
  auto page = GetPage();
  if (page == nullptr || char_index < 0) {
    return -1;
  }
  for (uint32_t i = 0; i < page->regions_.size(); i++) {
    auto& region = page->regions_[i];
    auto& element = region->element_;
    if (element->GetCharStart() <= static_cast<uint32_t>(char_index) &&
        (element->GetCharCount() + element->GetCharStart() >
         static_cast<uint32_t>(char_index))) {
      return i;
    }
  }
  return -1;
}

std::vector<std::string> MarkdownDocument::GetVisibleInlineViews(
    int32_t animation_step, bool content_complete) {
  std::vector<std::string> result;
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return result;
  }
  int32_t char_count = MarkdownSelection::GetPageCharCount(page.get());
  animation_step = std::min(animation_step, char_count);
  for (auto& inline_view : inline_views_) {
    auto& id = inline_view.id_;
    int32_t index = inline_view.char_index_;
    if (index < animation_step) {
      result.emplace_back(id);
    }
  }
  auto& cursor_id = style_.typewriter_cursor_.typewriter_cursor_.custom_cursor_;
  if (!cursor_id.empty()) {
    if (!content_complete || animation_step < char_count) {
      result.emplace_back(cursor_id);
    }
  }
  if (animation_step >= char_count && !para_vec_.empty() &&
      (static_cast<uint32_t>(char_count) <
       para_vec_.back()->GetCharStart() + para_vec_.back()->GetCharCount()) &&
      (truncation_delegate_ != nullptr)) {
    result.emplace_back(style_.truncation_.truncation_.content_);
  }
  return result;
}

Range MarkdownDocument::GetChangedRegionsWhenAnimationUpdated(int32_t from_step,
                                                              int32_t to_step) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return {0, 0};
  }
  if (from_step > to_step) {
    std::swap(from_step, to_step);
  }
  int32_t char_count = -1;
  for (auto& attachment : page->GetTextAttachments()) {
    char_count = std::min(attachment->start_index_, char_count);
    char_count = std::min(attachment->end_index_, char_count);
  }
  from_step += char_count + 1;
  from_step = std::max(0, from_step);
  auto start = GetRegionIndexByCharIndex(from_step);
  auto end = GetRegionIndexByCharIndex(to_step);
  return {.start_ = start, .end_ = end};
}

Range MarkdownDocument::GetShowedRegions(float top, float bottom) {
  auto page = GetPage();
  if (page == nullptr || page->regions_.empty()) {
    return {0, 0};
  }
  if (top > bottom) {
    std::swap(top, bottom);
  }
  int32_t start = MarkdownSelection::FindClosestRegionIndex(page.get(), top);
  if (start > 0) {
    auto start_rect = page->GetRegionRect(start);
    if (start_rect.GetTop() > top) {
      start -= 1;
    }
  }
  int32_t end = MarkdownSelection::FindClosestRegionIndex(page.get(), bottom);
  if (end + 1 < static_cast<int32_t>(page->GetRegionCount())) {
    auto end_rect = page->GetRegionRect(end);
    if (end_rect.GetBottom() < bottom) {
      end += 1;
    }
  }
  return {start, end};
}

Range MarkdownDocument::GetShowedExtraContents(float top, float bottom) {
  auto page = GetPage();
  if (page == nullptr) {
    return {0, 0};
  }
  auto count = page->GetExtraBorderCount();
  int start = 0;
  int end = count - 1;
  for (auto i = 0u; i < count; i++) {
    auto rect = page->GetExtraBorder(i)->rect_;
    if (rect.GetBottom() > top) {
      start = i;
      break;
    }
  }
  for (uint32_t i = start; i < count; i++) {
    auto rect = page->GetExtraBorder(i)->rect_;
    if (rect.GetTop() > bottom) {
      end = i - 1;
      break;
    }
  }
  return {start, end};
}

void MarkdownDocument::InheritState(
    lynx::markdown::MarkdownDocument* old_document) {
  if (old_document != nullptr && old_document->page_ != nullptr) {
    inherited_scroll_state_ = old_document->page_->GetScrollState();
  }
}

void MarkdownDocument::TrimParagraphSpaces() const {
  if (!para_vec_.empty()) {
    auto& para_front = para_vec_.front();
    auto block_front = para_front->GetBlockStyle();
    block_front.margin_top_ = 0;
    block_front.padding_top_ = 0;
    para_front->SetBlockStyle(block_front);
    auto& para_back = para_vec_.front();
    auto block_back = para_back->GetBlockStyle();
    block_back.margin_bottom_ = 0;
    block_back.padding_bottom_ = 0;
    para_back->SetBlockStyle(block_back);
    para_back->SetSpaceAfter(0);
  }
}

}  // namespace markdown
}  // namespace lynx
