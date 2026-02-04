// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_

#include <string>
namespace lynx::markdown {
class MarkdownDomNode;
class MarkdownParser {
 public:
  virtual ~MarkdownParser() = default;
  virtual MarkdownDomNode* Parse(std::string_view source, void* ud) = 0;
  static void RegisterParser(const std::string& name, MarkdownParser* parser);
};
}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_
