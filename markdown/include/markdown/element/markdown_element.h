// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_ELEMENT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_ELEMENT_H_
#include <memory>
#include <utility>
#include <vector>

#include "markdown/style/markdown_style.h"
#include "markdown/style/markdown_style_initializer.h"
#include "markdown/utils/markdown_definition.h"

namespace lynx::markdown {
enum class MarkdownElementType { kNone, kParagraph, kTable, kBlock, kListItem };
enum class MarkdownBorder {
  kNone,
  kLeft = 1,
  kTop = 1 << 1,
  kRight = 1 << 2,
  kBottom = 1 << 3,
  kRect = kLeft + kTop + kRight + kBottom
};
enum class MarkdownSyntaxType : uint8_t {
  kUndefined = 0,
  kSource,
  kParagraph,
  kUnorderedList,
  kOrderedList,
  kCodeBlock,
  kQuote,
  kTable,
  kSplit,
};

class MarkdownElement {
 public:
  MarkdownElement(MarkdownElementType type)
      : space_after_(0), overflow_(), char_start_(0), char_count_(0) {
    MarkdownStyleInitializer::ResetBlockStyle(&block_style_);
    MarkdownStyleInitializer::ResetBorderStyle(&border_style_);
    border_type_ = MarkdownBorder::kNone;
    type_ = type;
  }
  virtual ~MarkdownElement() = default;
  void SetBlockStyle(const MarkdownBlockStylePart& block_style_part) {
    block_style_ = block_style_part;
  }
  void SetBorderStyle(const MarkdownBorderStylePart& border_style_part) {
    border_style_ = border_style_part;
  }
  void SetBorderType(MarkdownBorder border_type) { border_type_ = border_type; }
  const MarkdownBlockStylePart& GetBlockStyle() const { return block_style_; }
  const MarkdownBorderStylePart& GetBorderStyle() const {
    return border_style_;
  }
  MarkdownBorder GetBorderType() const { return border_type_; }
  MarkdownElementType GetType() const { return type_; }
  float GetSpaceAfter() const { return space_after_; }
  void SetSpaceAfter(float space_after) { space_after_ = space_after; }
  void SetTextOverflow(MarkdownTextOverflow overflow) { overflow_ = overflow; }
  MarkdownTextOverflow GetTextOverflow() const { return overflow_; }
  uint32_t GetCharStart() const { return char_start_; }
  uint32_t GetCharCount() const { return char_count_; }
  void SetCharStart(uint32_t index) { char_start_ = index; }
  void SetCharCount(uint32_t count) { char_count_ = count; }
  MarkdownSyntaxType GetMarkdownSourceType() const {
    return markdown_source_type_;
  }
  void SetMarkdownSourceType(MarkdownSyntaxType markdown_source_type) {
    markdown_source_type_ = markdown_source_type;
  }
  const Range& GetMarkdownSourceRange() const { return markdown_source_range_; }
  void SetMarkdownSourceRange(const Range& markdown_source_range) {
    markdown_source_range_ = markdown_source_range;
  }
  bool ScrollX() const { return scroll_x_; }
  void SetScrollX(bool scroll) { scroll_x_ = scroll; }
  MarkdownTextAlign GetLastLineAlign() const { return last_line_align_; }
  void SetLastLineAlign(MarkdownTextAlign align) { last_line_align_ = align; }

 protected:
  MarkdownElementType type_;
  MarkdownBlockStylePart block_style_;
  MarkdownBorderStylePart border_style_;
  MarkdownBorder border_type_;
  float space_after_;
  MarkdownTextOverflow overflow_;
  uint32_t char_start_;
  uint32_t char_count_;
  MarkdownSyntaxType markdown_source_type_{MarkdownSyntaxType::kUndefined};
  Range markdown_source_range_{0, 0};
  bool scroll_x_{false};
  MarkdownTextAlign last_line_align_{MarkdownTextAlign::kUndefined};
};

class MarkdownBlockElement : public MarkdownElement {
 public:
  MarkdownBlockElement() : MarkdownElement(MarkdownElementType::kBlock) {}
  ~MarkdownBlockElement() override = default;
  void AddChild(std::unique_ptr<MarkdownElement> child) {
    children_.emplace_back(std::move(child));
  }
  int32_t GetChildCount() const {
    return static_cast<int32_t>(children_.size());
  }
  MarkdownElement* GetChild(int32_t index) { return children_[index].get(); }
  std::vector<std::unique_ptr<MarkdownElement>>& GetChildren() {
    return children_;
  }

 private:
  std::vector<std::unique_ptr<MarkdownElement>> children_;
};

}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_ELEMENT_H_
