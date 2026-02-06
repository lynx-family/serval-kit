// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_PARSER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_PARSER_H_
#include <memory>
#include <string_view>

namespace lynx::markdown {
class MarkdownInlineNode;
class MarkdownInlineSyntaxParser {
 public:
  static std::unique_ptr<MarkdownInlineNode> Parse(
      const std::string_view content);
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_INLINE_PARSER_H_
