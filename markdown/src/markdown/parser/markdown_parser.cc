// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/markdown_parser.h"

#include <unordered_map>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/embed/markdown_parser_embed.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
namespace lynx::markdown {

class ParserMap {
 public:
  ParserMap() = default;
  MarkdownParser* GetParser(const std::string& name) {
    const auto iter = parser_map_.find(name);
    if (iter == parser_map_.end() || iter->second == nullptr) {
      return nullptr;
    }
    return iter->second;
  }
  void RegisterParser(const std::string& name, MarkdownParser* parser) {
    if (parser == nullptr || name.empty()) {
      return;
    }
    parser_map_[name] = parser;
  }
  std::unordered_map<std::string, MarkdownParser*> parser_map_;
};

ParserMap& GetParserMap() {
  static ParserMap parser_map;
  return parser_map;
}

void MarkdownParser::RegisterParser(const std::string& name,
                                    MarkdownParser* parser) {
  GetParserMap().RegisterParser(name, parser);
}

void MarkdownParserImpl::ParseMarkdown(const std::string& parser_name,
                                       MarkdownDocument* document, void* ud) {
  if (!parser_name.empty()) {
    const auto parser = GetParserMap().GetParser(parser_name);
    if (parser != nullptr) {
      auto* dom = parser->Parse(document->GetMarkdownContent(), ud);
      MarkdownParserImpl impl(document);
      impl.ConvertDomTree(dom);
      return;
    }
  }
  MarkdownParserEmbed discount_parser(document);
  auto& content = document->GetMarkdownContent();
  auto range = document->GetMarkdownContentRange();
  auto width = document->GetMaxWidth();
  discount_parser.Parse(content.c_str(), static_cast<int32_t>(content.length()),
                        range.start_, range.end_, width);
}

void MarkdownParserImpl::ParsePlainText(MarkdownDocument* document) {
  MarkdownParserEmbed discount_parser(document);
  auto& content = document->GetMarkdownContent();
  discount_parser.ParsePlainText(content.c_str(),
                                 static_cast<int32_t>(content.length()));
}

MarkdownParserImpl::MarkdownParserImpl(MarkdownDocument* document)
    : document_(document) {}

void MarkdownParserImpl::ConvertDomTree(MarkdownDomNode* root) {}

}  // namespace lynx::markdown
