// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_PARAGRAPH_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_PARAGRAPH_H_
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/layout/markdown_layout_node.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace serval::markdown {
class MarkdownLayoutParagraph : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutParagraph(MarkdownContext* context);
  MarkdownLayoutParagraph(MarkdownContext* context,
                          std::unique_ptr<tttext::Paragraph> paragraph);
  ~MarkdownLayoutParagraph() override;
  void SetParagraph(std::unique_ptr<tttext::Paragraph> paragraph) {
    paragraph_ = std::move(paragraph);
    layout_region_ = nullptr;
  }
  tttext::Paragraph* GetParagraph() const { return paragraph_.get(); }
  void SetTextOverflow(MarkdownTextOverflow overflow) {
    text_overflow_ = overflow;
  }
  MarkdownTextOverflow GetTextOverflow() const { return text_overflow_; }
  void SetLastLineAlign(MarkdownTextAlign align) { last_line_align_ = align; }
  MarkdownTextAlign GetLastLineAlign() const { return last_line_align_; }

 public:
  Range GetCharRangeByPoint(PointF point, CharRangeType type) override;
  void GetSelectionRectByCharPos(std::vector<RectF>* result,
                                 int32_t char_pos_start, int32_t char_pos_end,
                                 RectType type,
                                 RectCoordinate coordinate) override;
  void GetContentByCharPos(std::string* result, int32_t char_pos_start,
                           int32_t char_pos_end) override;
  void GetLineEndCharIndices(std::vector<int32_t>* result) override;
  int32_t FindLineByY(float y) const;
  static std::pair<int32_t, int32_t> ExpandToSentence(std::string_view content, int32_t index) ;

 protected:
  LayoutResult OnLayout(LayoutParams params) override;
  void OnRender(RenderParams params) override;
  bool ForceAddTruncation() override;

  std::unique_ptr<tttext::Paragraph> paragraph_;
  std::unique_ptr<tttext::LayoutRegion> layout_region_;
  PointF region_offset_;
  MarkdownTextOverflow text_overflow_{MarkdownTextOverflow::kClip};
  MarkdownTextAlign last_line_align_{MarkdownTextAlign::kUndefined};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_PARAGRAPH_H_
