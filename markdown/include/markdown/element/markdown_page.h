// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PAGE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PAGE_H_

#include <memory>
#include <utility>
#include <vector>

#include "markdown/element/markdown_attachments.h"
#include "markdown/element/markdown_region.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {
struct ScrollState {
  uint32_t index_;
  MarkdownElementType type_;
  float scroll_offset_;
};
class MarkdownPage {
 public:
  MarkdownPage() = default;
  void SetElements(
      const std::vector<std::shared_ptr<MarkdownElement>>& elements) {
    elements_ = elements;
  }
  float GetLayoutHeight() const { return layout_height_; }
  float GetLayoutWidth() const { return layout_width_; }
  float GetMaxWidth() const { return max_width_; }
  float GetMaxHeight() const { return max_height_; }
  int GetLineCount() const { return line_count_; }
  bool FullFilled() { return full_filled_; }
  void SetCustomTypewriterCursor(
      std::unique_ptr<tttext::RunDelegate> custom_typewriter_cursor) {
    custom_typewriter_cursor_ = std::move(custom_typewriter_cursor);
  }
  tttext::RunDelegate* GetCustomTypewriterCursor() {
    return custom_typewriter_cursor_.get();
  }
  MarkdownPageRegion* GetRegion(uint32_t index) const {
    if (index >= regions_.size())
      return nullptr;
    return regions_[index].get();
  }
  RectF GetRegionRect(uint32_t index) const {
    if (index >= regions_.size())
      return RectF::MakeEmpty();
    auto* region = regions_[index].get();
    if (region->border_ != nullptr) {
      return region->border_->rect_;
    }
    return region->rect_;
  }
  uint32_t GetRegionCount() const { return regions_.size(); }
  MarkdownQuoteBorder* GetExtraBorder(uint32_t index) const {
    if (index >= quote_borders_.size())
      return nullptr;
    return quote_borders_[index].get();
  }
  uint32_t GetExtraBorderCount() const { return quote_borders_.size(); }

  void ClearAttachments() { attachments_.clear(); }
  void AddTextAttachments(
      std::vector<std::unique_ptr<MarkdownTextAttachment>> attachment) {
    for (auto& value : attachment) {
      attachments_.emplace_back(std::move(value));
    }
  }
  const std::vector<std::unique_ptr<MarkdownTextAttachment>>&
  GetTextAttachments() const {
    return attachments_;
  }
  void SetBorderAttachments(
      std::vector<std::unique_ptr<MarkdownTextAttachment>>&& attachments) {
    border_attachments_ = std::move(attachments);
  }
  const std::vector<std::unique_ptr<MarkdownTextAttachment>>&
  GetBorderAttachments() const {
    return border_attachments_;
  }
  std::vector<ScrollState> GetScrollState() const;
  void ApplyScrollState(const std::vector<ScrollState>& states);
  void AddRegion(std::unique_ptr<MarkdownPageRegion> region) {
    regions_.emplace_back(std::move(region));
  }

 private:
  std::vector<std::shared_ptr<MarkdownElement>> elements_;
  std::vector<std::unique_ptr<MarkdownPageRegion>> regions_;
  std::vector<std::unique_ptr<MarkdownTextAttachment>> attachments_;
  std::vector<std::unique_ptr<MarkdownTextAttachment>> border_attachments_;
  int line_count_{0};
  bool full_filled_{false};
  float layout_width_;
  float layout_height_;
  float max_width_;
  float max_height_;
  std::unique_ptr<tttext::RunDelegate> custom_typewriter_cursor_{nullptr};
  // TODO(zhouchaoying): temporarily fix quote border, will be removed next
  // commit
  std::vector<std::unique_ptr<MarkdownQuoteBorder>> quote_borders_;
  friend class MarkdownLayout;
  friend class MarkdownDrawer;
  friend class MarkdownParser;
  friend class MarkdownSelection;
  friend class MarkdownDocument;
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PAGE_H_
