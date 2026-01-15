// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_LIST_ITEM_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_LIST_ITEM_H_
#include <memory>
#include "markdown/element/markdown_element.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown {
class MarkdownListItem : public MarkdownBlockElement {
 public:
  MarkdownListItem() { type_ = MarkdownElementType::kListItem; }
  ~MarkdownListItem() override = default;
  void SetMarker(const std::shared_ptr<tttext::RunDelegate>& delegate) {
    marker_ = delegate;
  }
  const std::shared_ptr<tttext::RunDelegate>& GetMarker() const {
    return marker_;
  }

 protected:
  std::shared_ptr<tttext::RunDelegate> marker_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_LIST_ITEM_H_
