// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_REGION_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_REGION_H_
#include <memory>
#include <vector>
#include "markdown/element/markdown_element.h"
#include "markdown/element/markdown_paragraph.h"

namespace lynx::markdown {
struct MarkdownPageRegionBorder {
  RectF rect_;
  MarkdownBorderStylePart border_style_;
  MarkdownBorder border_;
};
struct MarkdownQuoteBorder {
  RectF rect_;
  MarkdownQuoteBorderLineStyle line_style_;
};
class MarkdownInlineBorderDelegate;
struct MarkdownInlineBorder {
  MarkdownInlineBorderDelegate* left_;
  MarkdownInlineBorderDelegate* right_;
};
class MarkdownPageRegion {
 public:
  MarkdownPageRegion() = default;
  virtual ~MarkdownPageRegion() = default;

 public:
  RectF rect_;
  std::unique_ptr<MarkdownPageRegionBorder> border_;

  bool scroll_x_{false};
  float scroll_x_offset_{0};
  RectF scroll_x_view_rect_;

  std::shared_ptr<MarkdownElement> element_;
};

class MarkdownPageParagraphRegion : public MarkdownPageRegion {
 public:
  ~MarkdownPageParagraphRegion() override = default;

 public:
  std::unique_ptr<tttext::LayoutRegion> region_{nullptr};
};

class MarkdownPageBlockRegion : public MarkdownPageRegion {
 public:
  ~MarkdownPageBlockRegion() override = default;

 public:
  std::vector<std::unique_ptr<MarkdownPageRegion>> children_;
};

}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_REGION_H_
