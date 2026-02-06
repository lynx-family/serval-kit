// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_
#include <limits>
#include <string>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_element.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/element/markdown_table.h"
#include "markdown/parser/embed/markdown_inline_node.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
struct line;
namespace lynx {
namespace markdown {
class MarkdownDocument;
class MarkdownResourceLoader;
struct MarkdownContext {
  std::vector<int32_t> para_stack_;
  std::unique_ptr<tttext::Paragraph> current_paragraph_{nullptr};
  std::unique_ptr<MarkdownTable> current_table_{nullptr};
  std::string extra_class_{};
  MarkdownBlockStylePart block_style_;
  MarkdownBorderStylePart border_style_;
  MarkdownBorder border_type_;
  float text_size_{0};
  std::vector<int32_t> list_index_stack_;
  std::vector<int32_t> list_level_stack_;
  int list_level_{0};
  int list_extra_level_{0};
  int list_checked_{-1};
  int quote_level_{0};
  int quote_start_para_{-1};
  int list_start_index_{1};
  int hn_{-1};
  bool have_normal_text_{false};
  tttext::RulerType line_height_rule_{tttext::RulerType::kExact};
  uint32_t char_offset_{0};
  std::string_view markdown_source_{};
  std::vector<int32_t> byte_index_to_char_index_;
  uint32_t markdown_start_{0};
  uint32_t markdown_end_{std::numeric_limits<uint32_t>::max()};
  uint32_t processed_markdown_length_{0};
  bool enable_split_render_{true};
  float max_width_{-1};
  float indent_{0};
  Range markdown_source_range_{0, 0};
  std::vector<int32_t> lines_offset_;
  uint32_t line_index_{0};
};
class MarkdownParserEmbed {
 public:
  explicit MarkdownParserEmbed(MarkdownDocument* document)
      : document_(document) {}
  ~MarkdownParserEmbed() = default;
  void Parse(const char* src, int size, int32_t markdown_start = 0,
             int32_t markdown_end = std::numeric_limits<int32_t>::max(),
             float width = -1);
  void ParsePlainText(const char* src, int size);

 private:
  static void OnParagraphStart(int type, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnParagraphStart(type);
  }
  static void OnParagraphText(line* line, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnParagraphText(line);
  }
  static void OnHeaderNumber(int hn, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnHeaderNumber(hn);
  }
  static void OnParagraphAlign(int align_type, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnParagraphAlign(align_type);
  }
  static void OnListCheck(int checked, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnListCheck(checked);
  }
  static void OnParagraphEnd(void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnParagraphEnd();
  }
  static void OnListIndex(int list_index, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnListIndex(list_index);
  }
  static void OnListExtraLevel(int list_level, void* ud) {
    reinterpret_cast<MarkdownParserEmbed*>(ud)->OnListExtraLevel(list_level);
  }

 private:
  void OnParagraphStart(int type);
  void OnParagraphText(line* line);
  void OnHeaderNumber(int hn);
  void OnParagraphAlign(int align_type);
  void OnListCheck(int checked);
  void OnListIndex(int index);
  void OnListExtraLevel(int level);
  void OnParagraphEnd();
  void HandleTableLines(line* line);
  void ParseInlineSyntax(const std::string& content, tttext::Paragraph* para,
                         const tttext::Style& base_style,
                         bool* have_normal_text, uint32_t char_offset,
                         uint32_t markdown_offset, bool check_paragraph_tag);
  void AppendNodeToParagraph(MarkdownInlineNode* node, tttext::Paragraph* para,
                             const tttext::Style& base_style,
                             uint32_t char_offset, uint32_t markdown_offset);
  void AppendChildrenToParagraph(MarkdownInlineNode* node,
                                 tttext::Paragraph* para,
                                 const tttext::Style& base_style,
                                 uint32_t char_offset,
                                 uint32_t markdown_offset);
  void AppendLinkToParagraph(MarkdownLinkNode* node, tttext::Paragraph* para,
                             const tttext::Style& base_style,
                             uint32_t char_offset, uint32_t markdown_offset);
  void AppendImgToParagraph(MarkdownImageNode* node, tttext::Paragraph* para,
                            const tttext::Style& base_style,
                            uint32_t char_offset, uint32_t markdown_offset);
  void AppendInlineCode(MarkdownInlineNode* node, tttext::Paragraph* para,
                        const tttext::Style& base_style, uint32_t char_offset,
                        uint32_t markdown_offset);
  void AppendRawText(MarkdownInlineNode* node, tttext::Paragraph* para,
                     const tttext::Style& base_style, uint32_t char_offset,
                     uint32_t markdown_offset);
  void AppendInlineHtml(MarkdownInlineHtmlTag* node, tttext::Paragraph* para,
                        const tttext::Style& base_style, uint32_t char_offset,
                        uint32_t markdown_offset);
  void AppendDoubleBraces(MarkdownInlineNode* node, tttext::Paragraph* para,
                          const tttext::Style& base_style, uint32_t char_offset,
                          uint32_t markdown_offset);
  void AppendDoubleSquareBracket(MarkdownInlineNode* node,
                                 tttext::Paragraph* para,
                                 const tttext::Style& base_style,
                                 uint32_t char_offset,
                                 uint32_t markdown_offset);

  static void AppendInlineBorderLeft(const MarkdownBlockStylePart& block,
                                     const MarkdownBorderStylePart& border,
                                     MarkdownBackgroundStylePart* background,
                                     tttext::Paragraph* para,
                                     tttext::Style* style);
  static void AppendInlineBorderRight(MarkdownDocument* document,
                                      const MarkdownBaseStylePart& base,
                                      const MarkdownBlockStylePart& block,
                                      const MarkdownBorderStylePart& border,
                                      MarkdownBackgroundStylePart* background,
                                      tttext::Paragraph* para,
                                      uint32_t char_offset_start,
                                      uint32_t char_offset_end);

  std::vector<std::string_view> Split(const std::string_view& content,
                                      char split);
  std::string_view TrimSpace(std::string_view origin);
  std::vector<tttext::ParagraphHorizontalAlignment> HandleTableAlign(
      const std::string_view& content);
  void SetParagraphStyle(const MarkdownBaseStylePart& base_style_part,
                         tttext::ParagraphStyle* paragraph_style,
                         MarkdownElement* element);
  static void SetParagraphStyle(MarkdownDocument* document,
                                const MarkdownBaseStylePart& base_style_part,
                                tttext::ParagraphStyle* paragraph_style,
                                MarkdownElement* element,
                                tttext::RulerType line_height_rule);
  void SetTTStyleByMarkdownBaseStyle(
      const MarkdownBaseStylePart& base_style_part, tttext::Style* style);
  static void SetTTStyleByMarkdownBaseStyle(
      MarkdownDocument* document, const MarkdownBaseStylePart& base_style_part,
      tttext::Style* style);
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
  static tttext::WriteDirection ConvertWriteDirection(
      MarkdownDirection direction);
  static tttext::ParagraphHorizontalAlignment ConvertTextAlign(
      MarkdownTextAlign align);

  void GenerateElement(MarkdownElement* element);
  void GenerateParagraph(int type, MarkdownParagraphElement* para);
  void GenerateTable(MarkdownTableElement* table);
  void AppendOrderedListNumber();
  void AppendUnorderedListMark();

  static const MarkdownBaseStylePart& GetHNStyle(const MarkdownStyle& style,
                                                 int hn);
  static const MarkdownBlockStylePart& GetHNBlockStyle(
      const MarkdownStyle& style, int hn);
  std::pair<uint32_t, uint32_t> GetTextLineByteRangeByMarkdownRange(
      uint32_t line_offset, uint32_t line_length);
  int32_t MarkdownSourceByteIndexToCharIndex(int32_t byte_index) const;
  static std::vector<int32_t> CalculateByteIndexToCharIndexMap(
      std::string_view string);

  MarkdownContext context_{};
  MarkdownStyle style_{};
  MarkdownResourceLoader* loader_{nullptr};
  MarkdownDocument* document_{nullptr};
  friend class MarkdownLayout;
  friend class MarkdownDocument;
  friend class MarkdownConverter;
};
}  // namespace markdown
}  // namespace lynx

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_DISCOUNT_MARKDOWN_PARSER_DISCOUNT_H_
