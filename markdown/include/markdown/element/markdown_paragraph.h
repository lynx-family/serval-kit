// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PARAGRAPH_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PARAGRAPH_H_

#include <memory>

#include <utility>
#include "markdown/element/markdown_element.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {

class MarkdownParagraphElement : public MarkdownElement {
 public:
  explicit MarkdownParagraphElement(
      MarkdownElementType type = MarkdownElementType::kParagraph)
      : MarkdownElement(type) {}
  ~MarkdownParagraphElement() override = default;

 public:
  void SetParagraph(std::unique_ptr<tttext::Paragraph> paragraph) {
    paragraph_ = std::move(paragraph);
  }
  tttext::Paragraph* GetParagraph() const { return paragraph_.get(); }

 protected:
  std::unique_ptr<tttext::Paragraph> paragraph_{nullptr};
};

}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_PARAGRAPH_H_
