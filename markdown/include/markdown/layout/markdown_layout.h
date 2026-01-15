// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_H_

#include <memory>
#include <utility>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_page.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx {
namespace markdown {
class MarkdownTable;
class MarkdownTableRegion;
class MarkdownLayout {
 public:
  explicit MarkdownLayout(MarkdownDocument* document);
  void SetPaddings(Paddings paddings);
  std::pair<float, float> Layout(float width, float height, int text_max_lines);

  static std::pair<float, float> MeasureParagraph(tttext::Paragraph* paragraph,
                                                  float width, float height,
                                                  int max_lines);

 private:
  void Layout(const std::shared_ptr<MarkdownElement>& paragraph, int max_lines,
              bool last);
  std::unique_ptr<MarkdownPageRegion> LayoutElement(
      const MarkdownElement& paragraph, int max_lines, float max_width,
      float max_height, float region_left, float region_top, bool last);
  void ForceAppendEllipsis(MarkdownPageRegion* region);
  static std::unique_ptr<MarkdownTableRegion> LayoutTable(
      MarkdownTable* table, float width, float height, int max_lines,
      MarkdownTextOverflow overflow, bool* full_filled);
  static std::unique_ptr<tttext::LayoutRegion> LayoutParagraph(
      tttext::Paragraph* paragraph, float width, tttext::LayoutMode width_mode,
      float height, int max_lines, MarkdownTextOverflow overflow,
      bool* full_filled, bool last);
  static void SwapScrollState(MarkdownPage* origin_page,
                              MarkdownPage* new_page);

 private:
  Paddings paddings_{};
  float max_width_{0};
  float max_height_{0};
  float current_layout_bottom_{0};
  float current_margin_bottom_{0};
  MarkdownDocument* document_{nullptr};
  std::shared_ptr<MarkdownPage> page_{nullptr};
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_H_
