// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_

#include "markdown/parser/markdown_parser.h"

namespace lynx {
namespace markdown {
class MarkdownParserDiscount final : public MarkdownParser {
 public:
  ~MarkdownParserDiscount() override = default;

 protected:
  void Parse(MarkdownDocument* document, void* ud) override;
};
}  // namespace markdown
}  // namespace lynx

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_
