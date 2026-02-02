// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_INITIALIZER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_INITIALIZER_H_
#include <limits>

#include "markdown/style/markdown_style.h"
#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_screen_metrics.h"
namespace lynx {
namespace markdown {
class MarkdownStyleInitializer {
 public:
  static float Dp(float dp) { return MarkdownScreenMetrics::DPToPx(dp); }
  static void InitialNormalText(MarkdownNormalTextStyle* style) {
    ResetBaseStyle(&style->base_);
    ResetBlockStyle(&style->block_);
  }
  static void InitialOtherStyleByNormalTextStyle(MarkdownStyle* style) {
    InitialH1Style(style);
    InitialH2Style(style);
    InitialH3Style(style);
    InitialH4Style(style);
    InitialH5Style(style);
    InitialH6Style(style);
    InitialLinkStyle(style);
    InitialInlineCodeStyle(style);
    InitialCodeBlockStyle(style);
    InitialQuoteStyle(style);
    InitialOrderedListStyle(style);
    InitialUnorderedListStyle(style);
    InitialRefStyle(style);
    InitialTableStyle(style);
    InitialTableCellStyle(style);
    InitialSplitStyle(style);
    InitialTypewriterCursorStyle(style);
    InitialDoubleBracesStyle(style);
    InitialMarkStyle(style);
    InitialTruncationStyle(style);
    InitialListItemStyle(style);
    InitialTableHeaderStyle(style);
    InitialImageStyle(style);
    InitialQuoteBorderLineStyle(style);
    InitialUnorderedListMarkerStyle(style);
    InitialOrderedListNumberStyle(style);
    InitialImageCaptionStyle(style);
  }
  static void InitialNormalTextStyle(MarkdownStyle* style) {
    InitialNormalText(&style->normal_text_);
  }
  static void ResetBorderStyle(MarkdownBorderStylePart* border_style) {
    memset(border_style, 0, sizeof(MarkdownBorderStylePart));
  }
  static void ResetBlockStyle(MarkdownBlockStylePart* block_style) {
    memset(block_style, 0, sizeof(MarkdownBlockStylePart));
  }
  static void ResetDecorationStyle(
      MarkdownDecorationStylePart* decoration_style) {
    memset(decoration_style, 0, sizeof(MarkdownDecorationStylePart));
  }

  static void InitialH1Style(MarkdownStyle* style) {
    style->h1_.base_ = style->normal_text_.base_;
    style->h1_.base_.font_size_ *= DEFAULT_H1_SCALE;
    style->h1_.base_.line_height_ *= DEFAULT_H1_SCALE;
    style->h1_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h1_.block_);
  }

  static void InitialH2Style(MarkdownStyle* style) {
    style->h2_.base_ = style->normal_text_.base_;
    style->h2_.base_.font_size_ *= DEFAULT_H2_SCALE;
    style->h2_.base_.line_height_ *= DEFAULT_H2_SCALE;
    style->h2_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h2_.block_);
  }

  static void InitialH3Style(MarkdownStyle* style) {
    style->h3_.base_ = style->normal_text_.base_;
    style->h3_.base_.font_size_ *= DEFAULT_H3_SCALE;
    style->h3_.base_.line_height_ *= DEFAULT_H3_SCALE;
    style->h3_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h3_.block_);
  }

  static void InitialH4Style(MarkdownStyle* style) {
    style->h4_.base_ = style->normal_text_.base_;
    style->h4_.base_.font_size_ *= DEFAULT_H4_SCALE;
    style->h4_.base_.line_height_ *= DEFAULT_H4_SCALE;
    style->h4_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h4_.block_);
  }

  static void InitialH5Style(MarkdownStyle* style) {
    style->h5_.base_ = style->normal_text_.base_;
    style->h5_.base_.font_size_ *= DEFAULT_H5_SCALE;
    style->h5_.base_.line_height_ *= DEFAULT_H5_SCALE;
    style->h5_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h5_.block_);
  }

  static void InitialH6Style(MarkdownStyle* style) {
    style->h6_.base_ = style->normal_text_.base_;
    style->h6_.base_.font_size_ *= DEFAULT_H6_SCALE;
    style->h6_.base_.line_height_ *= DEFAULT_H6_SCALE;
    style->h6_.base_.font_weight_ = MarkdownFontWeight::kBold;
    ResetBlockStyle(&style->h6_.block_);
  }

  static void InitialLinkStyle(MarkdownStyle* style) {
    style->link_.base_ = style->normal_text_.base_;
    style->link_.base_.font_size_ = FONT_SIZE_UNDEFINED;
    ResetDecorationStyle(&style->link_.decoration_);
  }

  static void InitialInlineCodeStyle(MarkdownStyle* style) {
    ResetBlockStyle(&style->inline_code_.block_);
    ResetBorderStyle(&style->inline_code_.border_);
    style->inline_code_.base_ = style->normal_text_.base_;
    style->inline_code_.base_.background_color_ =
        DEFAULT_INLINE_CODE_BACKGROUND_COLOR;
    style->inline_code_.border_.border_radius_ =
        Dp(DEFAULT_INLINE_CODE_BORDER_RADIUS);
  }

  static void InitialCodeBlockStyle(MarkdownStyle* style) {
    style->code_block_.base_ = style->normal_text_.base_;
    style->code_block_.base_.font_size_ = Dp(DEFAULT_CODE_BLOCK_FONT_SIZE);
    style->code_block_.base_.line_height_ = Dp(DEFAULT_CODE_BLOCK_LINE_HEIGHT);
    style->code_block_.base_.color_ = DEFAULT_CODE_BLOCK_COLOR;
    style->code_block_.base_.text_overflow_ = DEFAULT_CODE_BLOCK_OVERFLOW;
    ResetBlockStyle(&style->code_block_.block_);
    ResetBorderStyle(&style->code_block_.border_);
    style->code_block_.block_.margin_top_ = Dp(DEFAULT_CODE_BLOCK_MARGIN);
    style->code_block_.block_.margin_bottom_ = Dp(DEFAULT_CODE_BLOCK_MARGIN);
    style->code_block_.block_.margin_left_ = 0;
    style->code_block_.block_.margin_right_ = 0;
    style->code_block_.block_.padding_top_ = Dp(DEFAULT_CODE_BLOCK_PADDING);
    style->code_block_.block_.padding_bottom_ = Dp(DEFAULT_CODE_BLOCK_PADDING);
    style->code_block_.block_.padding_left_ = Dp(DEFAULT_CODE_BLOCK_PADDING);
    style->code_block_.block_.padding_right_ = Dp(DEFAULT_CODE_BLOCK_PADDING);
    style->code_block_.border_.border_type_ = DEFAULT_CODE_BLOCK_BORDER_TYPE;
    style->code_block_.border_.border_radius_ =
        Dp(DEFAULT_CODE_BLOCK_BORDER_RADIUS);
    style->code_block_.border_.border_color_ = DEFAULT_CODE_BLOCK_BORDER_COLOR;
    style->code_block_.border_.border_width_ =
        Dp(DEFAULT_CODE_BLOCK_BORDER_WIDTH);
    style->code_block_.scroll_.scroll_x_ = false;
  }

  static void InitialQuoteStyle(MarkdownStyle* style) {
    style->quote_.base_ = style->normal_text_.base_;
    ResetBlockStyle(&style->quote_.block_);
    style->quote_.block_.padding_left_ = Dp(DEFAULT_QUOTE_PADDING_LEFT);
    style->quote_.block_.margin_left_ = Dp(DEFAULT_QUOTE_MARGIN_LEFT);
    style->quote_.block_.margin_top_ = Dp(DEFAULT_QUOTE_MARGIN_TOP);
    style->quote_.block_.margin_bottom_ = Dp(DEFAULT_QUOTE_MARGIN_BOTTOM);
    style->quote_.border_.border_type_ = DEFAULT_QUOTE_BORDER_TYPE;
    style->quote_.border_.border_color_ = DEFAULT_QUOTE_BORDER_COLOR;
    style->quote_.border_.border_width_ = Dp(DEFAULT_QUOTE_BORDER_WIDTH);
    style->quote_.border_.border_radius_ = 0;
    style->quote_.indent_.indent_ = Dp(DEFAULT_QUOTE_INDENT);
  }

  static void InitialOrderedListStyle(MarkdownStyle* style) {
    ResetBlockStyle(&style->ordered_list_.block_);
    style->ordered_list_.base_ = style->normal_text_.base_;
    style->ordered_list_.base_.paragraph_space_ =
        Dp(DEFAULT_ORDERED_LIST_PARAGRAPH_SPACE);
    style->ordered_list_.ordered_list_.number_font_ =
        style->ordered_list_.base_.font_;
    style->ordered_list_.ordered_list_.number_font_size_ =
        style->ordered_list_.base_.font_size_;
    style->ordered_list_.ordered_list_.number_color_ =
        style->ordered_list_.base_.color_;
    style->ordered_list_.ordered_list_.number_type_ =
        DEFAULT_ORDERED_LIST_NUMBER_TYPE;
    style->ordered_list_.ordered_list_.number_margin_right_ =
        Dp(DEFAULT_ORDERED_LIST_NUMBER_MARGIN);
    style->ordered_list_.indent_.indent_ = Dp(DEFAULT_ORDERED_LIST_INDENT);
  }

  static void InitialUnorderedListStyle(MarkdownStyle* style) {
    ResetBlockStyle(&style->unordered_list_.block_);
    style->unordered_list_.base_ = style->normal_text_.base_;
    style->unordered_list_.base_.paragraph_space_ =
        Dp(DEFAULT_UNORDERED_LIST_PARAGRAPH_SPACE);
    style->unordered_list_.unordered_list_.mark_color_ =
        DEFAULT_UNORDERED_LIST_MARK_COLOR;
    style->unordered_list_.unordered_list_.mark_margin_right_ =
        Dp(DEFAULT_UNORDERED_LIST_MARK_MARGIN);
    style->unordered_list_.unordered_list_.mark_type_ =
        DEFAULT_UNORDERED_LIST_MARK_TYPE;
    style->unordered_list_.unordered_list_.mark_size_ =
        Dp(DEFAULT_UNORDERED_LIST_MARK_SIZE);
    style->unordered_list_.indent_.indent_ = DEFAULT_UNORDERED_LIST_INDENT;
  }

  static void InitialRefStyle(MarkdownStyle* style) {
    style->ref_.base_ = style->normal_text_.base_;
    style->ref_.base_.font_size_ = Dp(DEFAULT_REF_FONT_SIZE);
    style->ref_.ref_.background_type_ = DEFAULT_REF_BACKGROUND_TYPE;
    style->ref_.base_.color_ = DEFAULT_REF_COLOR;
    style->ref_.base_.background_color_ = DEFAULT_REF_BACKGROUND_COLOR;
    ResetBlockStyle(&(style->ref_.block_));
    style->ref_.block_.margin_left_ = Dp(DEFAULT_REF_MARGIN);
    style->ref_.block_.margin_right_ = Dp(DEFAULT_REF_MARGIN);
  }

  static void InitialTableStyle(MarkdownStyle* style) {
    ResetBlockStyle(&(style->table_.block_));
    ResetBorderStyle(&style->table_.border_);
    ResetBlockStyle(&(style->table_cell_.block_));
    style->table_.block_.margin_bottom_ =
        style->normal_text_.base_.paragraph_space_;
    style->table_.border_.border_radius_ = Dp(DEFAULT_TABLE_BORDER_RADIUS);
    style->table_.border_.border_color_ = DEFAULT_TABLE_BORDER_COLOR;
    style->table_.border_.border_width_ = Dp(DEFAULT_TABLE_BORDER_WIDTH);
    style->table_.scroll_.scroll_x_ = false;
    style->table_.table_.alt_color_ = 0;
    style->table_.table_.table_background_ = MarkdownTableBackground::kNone;
    style->table_.table_.table_border_ = MarkdownTableBorder::kFullRect;
    style->table_.table_.table_split_ = MarkdownTableSplit::kAll;
  }

  static void InitialTableCellStyle(MarkdownStyle* style) {
    style->table_cell_.base_ = style->normal_text_.base_;
    style->table_cell_.base_.text_overflow_ = DEFAULT_TABLE_OVERFLOW;
    style->table_cell_.block_.padding_left_ = Dp(DEFAULT_TABLE_CELL_PADDING);
    style->table_cell_.block_.padding_right_ = Dp(DEFAULT_TABLE_CELL_PADDING);
    style->table_cell_.block_.padding_top_ = Dp(DEFAULT_TABLE_CELL_PADDING);
    style->table_cell_.block_.padding_bottom_ = Dp(DEFAULT_TABLE_CELL_PADDING);
    style->table_cell_.align_.vertical_align_ = DEFAULT_CELL_ALIGN;
  }

  static void InitialSplitStyle(MarkdownStyle* style) {
    ResetBorderStyle(&style->split_.border_);
    ResetBlockStyle(&style->split_.block_);
    style->split_.border_.border_color_ = DEFAULT_SPLIT_COLOR;
    style->split_.border_.border_width_ = Dp(DEFAULT_SPLIT_WIDTH);
    style->split_.border_.border_type_ = DEFAULT_SPLIT_BORDER_TYPE;
  }

  static void InitialTypewriterCursorStyle(MarkdownStyle* style) {
    style->typewriter_cursor_.typewriter_cursor_.custom_cursor_ = {};
    style->typewriter_cursor_.typewriter_cursor_.vertical_align_ =
        DEFAULT_TYPEWRITER_VERTICAL_ALIGN;
  }

  static void InitialDoubleBracesStyle(MarkdownStyle* style) {
    style->double_braces_.base_ = style->normal_text_.base_;
    ResetBlockStyle(&style->double_braces_.block_);
    ResetBorderStyle(&style->double_braces_.border_);
  }

  static void InitialMarkStyle(MarkdownStyle* style) {
    style->mark_.base_ = style->normal_text_.base_;
    ResetBlockStyle(&style->mark_.block_);
    ResetBorderStyle(&style->mark_.border_);
  }

  static void InitialTruncationStyle(MarkdownStyle* style) {
    style->truncation_.truncation_.truncation_type_ = DEFAULT_TRUNCATION_TYPE;
    style->truncation_.truncation_.content_ = DEFAULT_TRUNCATION_CONTENT;
  }

  static void InitialListItemStyle(MarkdownStyle* style) {
    ResetBlockStyle(&style->list_item_.block_);
  }

  static void InitialTableHeaderStyle(MarkdownStyle* style) {
    style->table_header_.base_ = style->table_cell_.base_;
    style->table_header_.base_.font_weight_ = MarkdownFontWeight::kBold;
    style->table_header_.block_ = style->table_cell_.block_;
    style->table_header_.align_ = style->table_cell_.align_;
  }

  static void InitialImageStyle(MarkdownStyle* style) {
    style->image_.size_.width_ = SIZE_UNDEFINED;
    style->image_.size_.height_ = SIZE_UNDEFINED;
    style->image_.image_.enable_alt_text_ = true;
    style->image_.image_.radius_ = 0;
    ResetBlockStyle(&(style->image_.block_));
  }

  static void InitialQuoteBorderLineStyle(MarkdownStyle* style) {
    style->quote_border_line_.line_.width_ =
        style->quote_.border_.border_width_;
    style->quote_border_line_.line_.color_ =
        style->quote_.border_.border_color_;
    style->quote_border_line_.line_.radius_ = 0;
    style->quote_border_line_.line_.shrink_ = 0;
    style->quote_border_line_.line_.line_type_ = DEFAULT_QUOTE_LINE_TYPE;
  }

  static void InitialUnorderedListMarkerStyle(MarkdownStyle* style) {
    ResetBlockStyle(&style->unordered_list_marker_.block_);
    style->unordered_list_marker_.block_.margin_right_ =
        style->unordered_list_.unordered_list_.mark_margin_right_;
    style->unordered_list_marker_.marker_.mark_type_ =
        style->unordered_list_.unordered_list_.mark_type_;
    style->unordered_list_marker_.marker_.color_ =
        style->unordered_list_.unordered_list_.mark_color_;
    style->unordered_list_marker_.size_.width_ =
        style->unordered_list_.unordered_list_.mark_size_;
    style->unordered_list_marker_.size_.height_ = SIZE_UNDEFINED;
    style->unordered_list_marker_.align_.vertical_align_ = DEFAULT_MARKER_ALIGN;
  }

  static void ResetBaseStyle(MarkdownBaseStylePart* base) {
    base->font_size_ = Dp(DEFAULT_TEXT_FONT_SIZE);
    base->font_ = DEFAULT_TEXT_FONT;
    base->font_weight_ = DEFAULT_FONT_WEIGHT;
    base->font_style_ = MarkdownFontStyle::kUndefined;
    base->background_color_ = DEFAULT_TEXT_BACKGROUND_COLOR;
    base->color_ = DEFAULT_TEXT_COLOR;
    base->line_height_ = Dp(DEFAULT_TEXT_LINE_HEIGHT);
    base->paragraph_space_ = Dp(DEFAULT_TEXT_PARAGRAPH_SPACE);
    base->text_overflow_ = DEFAULT_TEXT_OVERFLOW;
    base->text_maxline_ = DEFAULT_PARAGRAPH_MAXLINE;
    base->line_space_ = Dp(DEFAULT_LINE_SPACE);
    base->word_break_ = MarkdownWordBreak::kNormal;
    base->direction_ = MarkdownDirection::kNormal;
    base->text_align_ = MarkdownTextAlign::kUndefined;
    base->text_indent_ = 0;
    base->last_line_alignment_ = MarkdownTextAlign::kUndefined;
  }

  static void ClearBaseStyle(MarkdownBaseStylePart* base) {
    base->font_ = "";
    base->font_size_ = SIZE_UNDEFINED;
    base->color_ = COLOR_UNDEFINED;
    base->background_color_ = COLOR_UNDEFINED;
    base->font_weight_ = DEFAULT_FONT_WEIGHT;
    base->font_style_ = MarkdownFontStyle::kUndefined;
    base->word_break_ = MarkdownWordBreak::kNormal;
    base->line_height_ = SIZE_UNDEFINED;
    base->paragraph_space_ = SIZE_UNDEFINED;
    base->text_overflow_ = DEFAULT_TEXT_OVERFLOW;
    base->text_maxline_ = DEFAULT_PARAGRAPH_MAXLINE;
    base->line_space_ = SIZE_UNDEFINED;
    base->direction_ = MarkdownDirection::kNormal;
    base->text_align_ = MarkdownTextAlign::kUndefined;
    base->text_indent_ = 0;
    base->last_line_alignment_ = MarkdownTextAlign::kUndefined;
  }

  static void InitializeSpanStyle(MarkdownSpanStyle* style) {
    ResetBlockStyle(&style->block_);
    ResetBorderStyle(&style->border_);
    ResetDecorationStyle(&style->decoration_);
    style->align_.vertical_align_ = MarkdownVerticalAlign::kBaseline;
    ClearBaseStyle(&(style->base_));
  }

  static void InitialOrderedListNumberStyle(MarkdownStyle* style) {
    style->ordered_list_number_.base_ = style->ordered_list_.base_;
    style->ordered_list_number_.base_.font_ =
        style->ordered_list_.ordered_list_.number_font_;
    style->ordered_list_number_.base_.font_size_ =
        style->ordered_list_.ordered_list_.number_font_size_;
    style->ordered_list_number_.base_.color_ =
        style->ordered_list_.ordered_list_.number_color_;
    ResetBlockStyle(&style->ordered_list_number_.block_);
    style->ordered_list_number_.block_.margin_right_ =
        style->ordered_list_.ordered_list_.number_margin_right_;
  }
  static void InitialImageCaptionStyle(MarkdownStyle* style) {
    style->image_caption_.base_ = style->normal_text_.base_;
    style->image_caption_.image_caption_.caption_position_ =
        MarkdownCaptionPosition::kBottom;
    style->image_caption_.base_.text_align_ = DEFAULT_IMAGE_CAPTION_ALIGN;
  }
  static void InitialBoldStyle(MarkdownStyle* style) {
    ClearBaseStyle(&(style->bold_.base_));
    style->bold_.base_.font_weight_ = MarkdownFontWeight::kBold;
  }
  static void InitialItalicStyle(MarkdownStyle* style) {
    ClearBaseStyle(&(style->italic_.base_));
    style->italic_.base_.font_style_ = MarkdownFontStyle::kItalic;
  }

 public:
  constexpr static float DEFAULT_TEXT_FONT_SIZE = 17;
  constexpr static const char* DEFAULT_TEXT_FONT = "";
  constexpr static MarkdownFontWeight DEFAULT_FONT_WEIGHT =
      MarkdownFontWeight::kNormal;
  constexpr static uint32_t DEFAULT_TEXT_COLOR = 0xff000000;
  constexpr static uint32_t DEFAULT_TEXT_BACKGROUND_COLOR = 0;
  constexpr static float DEFAULT_TEXT_LINE_HEIGHT = 30;
  constexpr static float DEFAULT_TEXT_PARAGRAPH_SPACE = 12;
  constexpr static MarkdownTextOverflow DEFAULT_TEXT_OVERFLOW =
      MarkdownTextOverflow::kEllipsis;
  constexpr static float DEFAULT_H1_SCALE = 2.3;
  constexpr static float DEFAULT_H2_SCALE = 2.0;
  constexpr static float DEFAULT_H3_SCALE = 1.8;
  constexpr static float DEFAULT_H4_SCALE = 1.5;
  constexpr static float DEFAULT_H5_SCALE = 1.3;
  constexpr static float DEFAULT_H6_SCALE = 1.0;
  constexpr static uint32_t DEFAULT_INLINE_CODE_BACKGROUND_COLOR = 0xFFd3d3d3;
  constexpr static float DEFAULT_CODE_BLOCK_FONT_SIZE = 14;
  constexpr static float DEFAULT_CODE_BLOCK_LINE_HEIGHT = 22;
  constexpr static float DEFAULT_CODE_BLOCK_MARGIN = 12;
  constexpr static float DEFAULT_CODE_BLOCK_PADDING = 12;
  constexpr static uint32_t DEFAULT_CODE_BLOCK_COLOR = 0xbf161823;
  constexpr static MarkdownBorderType DEFAULT_CODE_BLOCK_BORDER_TYPE =
      MarkdownBorderType::kSolid;
  constexpr static float DEFAULT_CODE_BLOCK_BORDER_RADIUS = 7;
  constexpr static uint32_t DEFAULT_CODE_BLOCK_BORDER_COLOR = 0xffe8e8e8;
  constexpr static float DEFAULT_CODE_BLOCK_BORDER_WIDTH = 1;
  constexpr static MarkdownTextOverflow DEFAULT_CODE_BLOCK_OVERFLOW =
      MarkdownTextOverflow::kClip;
  constexpr static float DEFAULT_QUOTE_MARGIN_LEFT = 8;
  constexpr static float DEFAULT_QUOTE_MARGIN_TOP = 5;
  constexpr static float DEFAULT_QUOTE_MARGIN_BOTTOM = 5;
  constexpr static float DEFAULT_QUOTE_PADDING_LEFT = 8;
  constexpr static MarkdownBorderType DEFAULT_QUOTE_BORDER_TYPE =
      MarkdownBorderType::kSolid;
  constexpr static uint32_t DEFAULT_QUOTE_BORDER_COLOR = 0xffd0c6ff;
  constexpr static float DEFAULT_QUOTE_BORDER_WIDTH = 2;
  constexpr static float DEFAULT_QUOTE_INDENT = 18;
  constexpr static MarkdownNumberType DEFAULT_ORDERED_LIST_NUMBER_TYPE =
      MarkdownNumberType::kNumber;
  constexpr static float DEFAULT_ORDERED_LIST_NUMBER_MARGIN = 4;
  constexpr static float DEFAULT_ORDERED_LIST_INDENT = 20;
  constexpr static float DEFAULT_ORDERED_LIST_PARAGRAPH_SPACE = 6;
  constexpr static MarkdownMarkType DEFAULT_UNORDERED_LIST_MARK_TYPE =
      MarkdownMarkType::kMixed;
  constexpr static float DEFAULT_UNORDERED_LIST_MARK_MARGIN = 4;
  constexpr static float DEFAULT_UNORDERED_LIST_INDENT = -1;
  constexpr static float DEFAULT_UNORDERED_LIST_PARAGRAPH_SPACE = 6;
  constexpr static uint32_t DEFAULT_UNORDERED_LIST_MARK_COLOR = 0xffd0c6ff;
  constexpr static float DEFAULT_UNORDERED_LIST_MARK_SIZE = 4;
  constexpr static float DEFAULT_REF_FONT_SIZE = 10;
  constexpr static MarkdownBackgroundType DEFAULT_REF_BACKGROUND_TYPE =
      MarkdownBackgroundType::kCapsule;
  constexpr static uint32_t DEFAULT_REF_COLOR = 0xff6100ff;
  constexpr static uint32_t DEFAULT_REF_BACKGROUND_COLOR = 0xfff6f4ff;
  constexpr static float DEFAULT_REF_MARGIN = 2;
  constexpr static float DEFAULT_TABLE_CELL_PADDING = 5;
  constexpr static uint32_t DEFAULT_SPLIT_COLOR = 0xff000000;
  constexpr static float DEFAULT_SPLIT_WIDTH = 1;
  constexpr static MarkdownBorderType DEFAULT_SPLIT_BORDER_TYPE =
      MarkdownBorderType::kSolid;
  constexpr static uint32_t DEFAULT_TABLE_BORDER_COLOR = 0xff000000;
  constexpr static float DEFAULT_TABLE_BORDER_WIDTH = 1;
  constexpr static MarkdownTextOverflow DEFAULT_TABLE_OVERFLOW =
      MarkdownTextOverflow::kClip;
  constexpr static float DEFAULT_TABLE_BORDER_RADIUS = 7;
  constexpr static float DEFAULT_INLINE_CODE_BORDER_RADIUS = 2;
  constexpr static MarkdownVerticalAlign DEFAULT_TYPEWRITER_VERTICAL_ALIGN =
      MarkdownVerticalAlign::kBaseline;
  constexpr static float FONT_SIZE_UNDEFINED = -1;
  constexpr static MarkdownTruncationType DEFAULT_TRUNCATION_TYPE =
      MarkdownTruncationType::kText;
  constexpr static auto DEFAULT_TRUNCATION_CONTENT = "…";
  constexpr static int32_t DEFAULT_PARAGRAPH_MAXLINE = -1;
  constexpr static float DEFAULT_LINE_SPACE = 0;
  constexpr static float SIZE_UNDEFINED = -1;
  constexpr static MarkdownVerticalAlign DEFAULT_CELL_ALIGN =
      MarkdownVerticalAlign::kCenter;
  constexpr static MarkdownVerticalAlign DEFAULT_MARKER_ALIGN =
      MarkdownVerticalAlign::kCenter;
  constexpr static uint32_t COLOR_UNDEFINED = 1;
  // Empty string means "use platform default".
  constexpr static const char* FONT_UNDEFINED = "";
  constexpr static MarkdownTableBorder DEFAULT_TABLE_BORDER =
      MarkdownTableBorder::kFullRect;
  constexpr static MarkdownTableBackground DEFAULT_TABLE_BACKGROUND =
      MarkdownTableBackground::kNone;
  constexpr static MarkdownLineType DEFAULT_QUOTE_LINE_TYPE =
      MarkdownLineType::kSolid;
  constexpr static MarkdownTextAlign DEFAULT_IMAGE_CAPTION_ALIGN =
      MarkdownTextAlign::kCenter;
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_INITIALIZER_H_
