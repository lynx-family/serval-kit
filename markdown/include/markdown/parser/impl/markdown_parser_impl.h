// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_
#include <string>
namespace lynx::markdown {
class MarkdownDocument;
class MarkdownDomNode;
class MarkdownParserImpl {
 public:
  static void ParseMarkdown(const std::string& parser_name,
                            MarkdownDocument* document, void* ud = nullptr);
  static void ParsePlainText(MarkdownDocument* document);

 protected:
  explicit MarkdownParserImpl(MarkdownDocument* document);
  void ConvertDomTree(MarkdownDomNode* root);

  MarkdownDocument* document_{nullptr};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_PARSER_IMPL_MARKDOWN_PARSER_IMPL_
