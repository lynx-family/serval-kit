// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_H_

#include <cstdint>
#include <string>
#include <unordered_map>
namespace lynx {
namespace markdown {
// AUTO GEN START
enum class MarkdownStyleTag {
  kNormalText = 0,
  kH1 = 1,
  kH2 = 2,
  kH3 = 3,
  kH4 = 4,
  kH5 = 5,
  kH6 = 6,
  kLink = 7,
  kInlineCode = 8,
  kCodeBlock = 9,
  kQuote = 10,
  kOrderedList = 11,
  kUnorderedList = 12,
  kRef = 13,
  kTable = 14,
  kTableCell = 15,
  kSplit = 16,
  kTypewriterCursor = 17,
  kMark = 18,
  kDoubleBraces = 19,
  kTruncation = 20,
  kListItem = 21,
  kTableHeader = 22,
  kImage = 23,
  kQuoteBorderLine = 24,
  kUnorderedListMarker = 25,
  kSpan = 26,
  kOrderedListNumber = 27,
  kImageCaption = 28,
  kBold = 29,
  kItalic = 30,
};
enum class MarkdownStyleOp {
  kRangeEnd = 0,
  kFont = 1,
  kFontSize = 2,
  kFontWeight = 3,
  kFontStyle = 4,
  kLineHeight = 5,
  kParagraphSpace = 6,
  kColor = 7,
  kBackgroundColor = 8,
  kTextOverflow = 9,
  kTextMaxline = 10,
  kMarginTop = 11,
  kMarginBottom = 12,
  kMarginLeft = 13,
  kMarginRight = 14,
  kPaddingTop = 15,
  kPaddingBottom = 16,
  kPaddingLeft = 17,
  kPaddingRight = 18,
  kBorderType = 19,
  kBorderColor = 20,
  kBorderWidth = 21,
  kBorderRadius = 22,
  kBackgroundImage = 23,
  kBackgroundSize = 24,
  kBackgroundRepeat = 25,
  kBackgroundPosition = 26,
  kIndent = 27,
  kNumberType = 28,
  kNumberFont = 29,
  kNumberFontSize = 30,
  kNumberColor = 31,
  kNumberMarginRight = 32,
  kMarkType = 33,
  kMarkColor = 34,
  kMarkSize = 35,
  kMarkMarginRight = 36,
  kBackgroundType = 37,
  kCustomCursor = 38,
  kVerticalAlign = 39,
  kTruncationType = 40,
  kContent = 41,
  kTextDecorationStyle = 42,
  kTextDecorationLine = 43,
  kTextDecorationColor = 44,
  kTextDecorationThickness = 45,
  kLineSpace = 46,
  kScrollX = 47,
  kMaxWidth = 48,
  kWidth = 49,
  kHeight = 50,
  kRadius = 51,
  kShrink = 52,
  kEnableAltText = 53,
  kWordBreak = 54,
  kDirection = 55,
  kAltImage = 56,
  kTextAlign = 57,
  kCaptionPosition = 58,
  kMaxHeight = 59,
  kMinWidth = 60,
  kMinHeight = 61,
  kTableBorder = 62,
  kTableBackground = 63,
  kAltColor = 64,
  kLineType = 65,
  kTextIndent = 66,
  kLastLineAlignment = 67,
  kTableSplit = 68,
};
enum class MarkdownBorderType {
  kNone = 0,
  kSolid = 1,
};
enum class MarkdownNumberType {
  kNumber = 0,
  kAlphabet = 1,
  kRomanNumerals = 2,
  kMixed = 3,
};
enum class MarkdownMarkType {
  kCircle = 0,
  kRing = 1,
  kSquare = 2,
  kMixed = 3,
};
enum class MarkdownBackgroundType {
  kNone = 0,
  kCapsule = 1,
};
enum class MarkdownTextOverflow {
  kClip = 0,
  kEllipsis = 1,
};
enum class MarkdownVerticalAlign {
  kBaseline = 0,
  kTop = 1,
  kBottom = 2,
  kCenter = 3,
};
enum class MarkdownFontWeight {
  kNormal = 0,
  kBold = 1,
  k100 = 2,
  k200 = 3,
  k300 = 4,
  k400 = 5,
  k500 = 6,
  k600 = 7,
  k700 = 8,
  k800 = 9,
  k900 = 10,
};
enum class MarkdownTruncationType {
  kText = 0,
  kView = 1,
};
enum class MarkdownTextDecorationStyle {
  kNone = 0,
  kSolid = 1,
  kDouble = 2,
  kDotted = 3,
  kDashed = 4,
  kWavy = 5,
};
enum class MarkdownTextDecorationLine {
  kNone = 0,
  kUnderline = 1,
  kOverline = 2,
  kLineThrough = 3,
};
enum class MarkdownWordBreak {
  kNormal = 0,
  kBreakAll = 1,
};
enum class MarkdownDirection {
  kNormal = 0,
  kLtr = 1,
  kRtl = 2,
};
enum class MarkdownTextAlign {
  kUndefined = 0,
  kLeft = 1,
  kCenter = 2,
  kRight = 3,
  kJustify = 4,
};
enum class MarkdownCaptionPosition {
  kBottom = 0,
  kTop = 1,
};
enum class MarkdownTableBorder {
  kNone = 0,
  kFullRect = 1,
  kUnderline = 2,
};
enum class MarkdownTableBackground {
  kNone = 0,
  kSolid = 1,
  kChessBoard = 2,
};
enum class MarkdownLineType {
  kNone = 0,
  kSolid = 1,
  kDouble = 2,
  kDotted = 3,
  kDashed = 4,
  kWavy = 5,
};
enum class MarkdownTableSplit {
  kNone = 0,
  kVertical = 1,
  kHorizontal = 2,
  kAll = 3,
};
enum class MarkdownFontStyle {
  kUndefined = 0,
  kNormal = 1,
  kItalic = 2,
};
struct MarkdownBaseStylePart {
  std::string font_;
  float font_size_;
  MarkdownFontWeight font_weight_;
  MarkdownFontStyle font_style_;
  float line_height_;
  float paragraph_space_;
  uint32_t color_;
  uint32_t background_color_;
  MarkdownTextOverflow text_overflow_;
  int text_maxline_;
  float line_space_;
  MarkdownWordBreak word_break_;
  MarkdownDirection direction_;
  MarkdownTextAlign text_align_;
  float text_indent_;
  MarkdownTextAlign last_line_alignment_;
};
struct MarkdownBlockStylePart {
  float margin_top_;
  float margin_bottom_;
  float margin_left_;
  float margin_right_;
  float padding_top_;
  float padding_bottom_;
  float padding_left_;
  float padding_right_;
  float max_width_;
  float max_height_;
  float min_width_;
  float min_height_;
};
struct MarkdownBorderStylePart {
  MarkdownBorderType border_type_;
  uint32_t border_color_;
  float border_width_;
  float border_radius_;
};
struct MarkdownBackgroundStylePart {
  std::string background_image_;
  std::string background_size_;
  std::string background_repeat_;
  std::string background_position_;
};
struct MarkdownIndentStylePart {
  float indent_;
};
struct MarkdownOrderedListStylePart {
  MarkdownNumberType number_type_;
  std::string number_font_;
  float number_font_size_;
  uint32_t number_color_;
  float number_margin_right_;
};
struct MarkdownUnorderedListStylePart {
  MarkdownMarkType mark_type_;
  uint32_t mark_color_;
  float mark_size_;
  float mark_margin_right_;
};
struct MarkdownRefStylePart {
  MarkdownBackgroundType background_type_;
};
struct MarkdownTypewriterCursorStylePart {
  std::string custom_cursor_;
  MarkdownVerticalAlign vertical_align_;
};
struct MarkdownTruncationStylePart {
  MarkdownTruncationType truncation_type_;
  std::string content_;
};
struct MarkdownDecorationStylePart {
  MarkdownTextDecorationStyle text_decoration_style_;
  MarkdownTextDecorationLine text_decoration_line_;
  uint32_t text_decoration_color_;
  float text_decoration_thickness_;
};
struct MarkdownScrollStylePart {
  bool scroll_x_;
};
struct MarkdownSizeStylePart {
  float width_;
  float height_;
};
struct MarkdownLineStylePart {
  float width_;
  uint32_t color_;
  float radius_;
  float shrink_;
  MarkdownLineType line_type_;
};
struct MarkdownMarkerStylePart {
  MarkdownMarkType mark_type_;
  uint32_t color_;
};
struct MarkdownImageStylePart {
  bool enable_alt_text_;
  std::string alt_image_;
  float radius_;
};
struct MarkdownAlignStylePart {
  MarkdownVerticalAlign vertical_align_;
};
struct MarkdownImageCaptionStylePart {
  MarkdownCaptionPosition caption_position_;
};
struct MarkdownTableStylePart {
  MarkdownTableBorder table_border_;
  MarkdownTableBackground table_background_;
  uint32_t background_color_;
  uint32_t alt_color_;
  MarkdownTableSplit table_split_;
};
struct MarkdownNormalTextStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH1Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH2Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH3Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH4Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH5Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownH6Style {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownLinkStyle {
  MarkdownBaseStylePart base_;
  MarkdownDecorationStylePart decoration_;
};
struct MarkdownInlineCodeStyle {
  MarkdownBaseStylePart base_;
  MarkdownBorderStylePart border_;
  MarkdownBlockStylePart block_;
};
struct MarkdownCodeBlockStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownScrollStylePart scroll_;
};
struct MarkdownQuoteStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownIndentStylePart indent_;
};
struct MarkdownOrderedListStyle {
  MarkdownBaseStylePart base_;
  MarkdownOrderedListStylePart ordered_list_;
  MarkdownBlockStylePart block_;
  MarkdownIndentStylePart indent_;
};
struct MarkdownUnorderedListStyle {
  MarkdownBaseStylePart base_;
  MarkdownUnorderedListStylePart unordered_list_;
  MarkdownBlockStylePart block_;
  MarkdownIndentStylePart indent_;
};
struct MarkdownRefStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownRefStylePart ref_;
};
struct MarkdownTableStyle {
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownScrollStylePart scroll_;
  MarkdownTableStylePart table_;
};
struct MarkdownTableCellStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownAlignStylePart align_;
};
struct MarkdownSplitStyle {
  MarkdownBorderStylePart border_;
  MarkdownBlockStylePart block_;
};
struct MarkdownTypewriterCursorStyle {
  MarkdownTypewriterCursorStylePart typewriter_cursor_;
};
struct MarkdownMarkStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownBackgroundStylePart background_;
};
struct MarkdownDoubleBracesStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownBackgroundStylePart background_;
};
struct MarkdownTruncationStyle {
  MarkdownTruncationStylePart truncation_;
};
struct MarkdownListItemStyle {
  MarkdownBlockStylePart block_;
};
struct MarkdownTableHeaderStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownAlignStylePart align_;
};
struct MarkdownImageStyle {
  MarkdownBlockStylePart block_;
  MarkdownSizeStylePart size_;
  MarkdownImageStylePart image_;
};
struct MarkdownQuoteBorderLineStyle {
  MarkdownLineStylePart line_;
};
struct MarkdownUnorderedListMarkerStyle {
  MarkdownBlockStylePart block_;
  MarkdownSizeStylePart size_;
  MarkdownMarkerStylePart marker_;
  MarkdownAlignStylePart align_;
};
struct MarkdownSpanStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
  MarkdownBorderStylePart border_;
  MarkdownBackgroundStylePart background_;
  MarkdownDecorationStylePart decoration_;
  MarkdownAlignStylePart align_;
};
struct MarkdownOrderedListNumberStyle {
  MarkdownBaseStylePart base_;
  MarkdownBlockStylePart block_;
};
struct MarkdownImageCaptionStyle {
  MarkdownBaseStylePart base_;
  MarkdownImageCaptionStylePart image_caption_;
};
struct MarkdownBoldStyle {
  MarkdownBaseStylePart base_;
};
struct MarkdownItalicStyle {
  MarkdownBaseStylePart base_;
};
struct MarkdownStyle {
  MarkdownNormalTextStyle normal_text_;
  MarkdownH1Style h1_;
  MarkdownH2Style h2_;
  MarkdownH3Style h3_;
  MarkdownH4Style h4_;
  MarkdownH5Style h5_;
  MarkdownH6Style h6_;
  MarkdownLinkStyle link_;
  MarkdownInlineCodeStyle inline_code_;
  MarkdownCodeBlockStyle code_block_;
  MarkdownQuoteStyle quote_;
  MarkdownOrderedListStyle ordered_list_;
  MarkdownUnorderedListStyle unordered_list_;
  MarkdownRefStyle ref_;
  MarkdownTableStyle table_;
  MarkdownTableCellStyle table_cell_;
  MarkdownSplitStyle split_;
  MarkdownTypewriterCursorStyle typewriter_cursor_;
  MarkdownMarkStyle mark_;
  MarkdownDoubleBracesStyle double_braces_;
  MarkdownTruncationStyle truncation_;
  MarkdownListItemStyle list_item_;
  MarkdownTableHeaderStyle table_header_;
  MarkdownImageStyle image_;
  MarkdownQuoteBorderLineStyle quote_border_line_;
  MarkdownUnorderedListMarkerStyle unordered_list_marker_;
  std::unordered_map<std::string, MarkdownSpanStyle> span_styles_;
  MarkdownOrderedListNumberStyle ordered_list_number_;
  MarkdownImageCaptionStyle image_caption_;
  MarkdownBoldStyle bold_;
  MarkdownItalicStyle italic_;
};
// AUTO GEN END
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_H_
