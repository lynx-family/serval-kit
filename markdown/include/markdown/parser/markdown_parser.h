// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_
#include <cstdint>
#include <string>
#include "markdown/element/markdown_element.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown {
class MarkdownDocument;
class MarkdownInlineBorderDelegate;
class MarkdownParser {
 public:
  virtual ~MarkdownParser() = default;
  virtual void Parse(MarkdownDocument* document, void* ud) = 0;
  static void RegisterParser(const std::string& name, MarkdownParser* parser);
  static void Parse(const std::string& parser_name, MarkdownDocument* document,
                    void* ud = nullptr);
  static void ParsePlainText(MarkdownDocument* document);

  static void SetTTStyleByMarkdownBaseStyle(
      const MarkdownBaseStylePart& base_style_part, tttext::Style* style);
  static void SetParagraphStyle(MarkdownDocument* document,
                                const MarkdownBaseStylePart& base_style_part,
                                tttext::ParagraphStyle* paragraph_style,
                                MarkdownElement* element,
                                tttext::RulerType line_height_rule);
  static void SetDecorationStyle(
      const MarkdownDecorationStylePart& decoration_style_part,
      tttext::Style* style);
  static std::string MarkdownNumberTypeToString(MarkdownNumberType type,
                                                int index);
  static tttext::LineType ConvertDecorationStyle(
      MarkdownTextDecorationStyle type);
  static tttext::DecorationType ConvertDecorationLine(
      MarkdownTextDecorationLine line);
  static tttext::CharacterVerticalAlignment ConvertVerticalAlign(
      MarkdownVerticalAlign align);
  static MarkdownInlineBorderDelegate* AppendInlineBorderLeft(
      const MarkdownDocument* document, const MarkdownBaseStylePart& base,
      const MarkdownBlockStylePart& block,
      const MarkdownBorderStylePart& border,
      MarkdownBackgroundStylePart* background, tttext::Paragraph* para,
      tttext::Style* style, uint32_t char_offset);
  static void AppendInlineBorderRight(MarkdownDocument* document,
                                      MarkdownInlineBorderDelegate* left,
                                      const MarkdownBaseStylePart& base,
                                      const MarkdownBlockStylePart& block,
                                      const MarkdownBorderStylePart& border,
                                      MarkdownBackgroundStylePart* background,
                                      tttext::Paragraph* para,
                                      uint32_t char_offset);
  static const MarkdownBaseStylePart& GetHNStyle(const MarkdownStyle& style,
                                                 int hn);
  static const MarkdownBlockStylePart& GetHNBlockStyle(
      const MarkdownStyle& style, int hn);
};
}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_PARSER_H_
