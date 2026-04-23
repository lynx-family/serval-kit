// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_H_
#include <string>
#include "markdown/utils/markdown_marco.h"
namespace serval::markdown {
class MarkdownDocument;
class MarkdownDomNode;
class L_EXPORT MarkdownParserImpl {
 public:
  static void ParseMarkdown(const std::string& parser_name,
                            MarkdownDocument* document, void* ud = nullptr);
  static void ParsePlainText(MarkdownDocument* document);

 protected:
  static void ConvertDomTree(MarkdownDocument* document, MarkdownDomNode* root);
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_H_
