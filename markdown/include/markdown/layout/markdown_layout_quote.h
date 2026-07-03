// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_QUOTE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_QUOTE_H_

#include "markdown/layout/markdown_layout_node.h"
#include "markdown/style/markdown_style.h"

namespace serval::markdown {

class MarkdownLayoutQuote : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutQuote(MarkdownContext* context);
  ~MarkdownLayoutQuote() override;

  void SetQuoteBorderLineStyle(MarkdownQuoteBorderLineStyle style) {
    line_style_ = style;
  }
  const MarkdownQuoteBorderLineStyle& GetQuoteBorderLineStyle() const {
    return line_style_;
  }

 protected:
  void OnRender(RenderParams params) override;

 private:
  void DrawQuoteLine(const RenderParams& params) const;

 private:
  MarkdownQuoteBorderLineStyle line_style_{};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_QUOTE_H_
