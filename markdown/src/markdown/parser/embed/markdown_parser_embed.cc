// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/embed/markdown_parser_embed.h"

#include <limits>

#include "base/include/string/string_utils.h"
#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/element/markdown_table.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/parser/embed/markdown_inline_node.h"
#include "markdown/parser/embed/markdown_inline_parser.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/style/markdown_style.h"
#include "markdown/style/markdown_style_initializer.h"
#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
extern "C" {
#include "discount/discount_lite/markdown.h"
}

namespace lynx::markdown {
MarkdownSyntaxType ParagraphTypeToSyntaxType(ParagraphType type) {
  switch (type) {
    case CODE:
      return MarkdownSyntaxType::kCodeBlock;
    case QUOTE:
      return MarkdownSyntaxType::kQuote;
    case MARKUP:
      return MarkdownSyntaxType::kParagraph;
    case UL:
      return MarkdownSyntaxType::kUnorderedList;
    case OL:
      return MarkdownSyntaxType::kOrderedList;
    case TABLE:
      return MarkdownSyntaxType::kTable;
    case SOURCE:
      return MarkdownSyntaxType::kSource;
    default:
      return MarkdownSyntaxType::kUndefined;
  }
}
constexpr static const char* kInlineViewSchema = "inlineview://";
constexpr static const char* kBlockViewSchema = "blockview://";

void MarkdownParserEmbed::Parse(const char* src, int size,
                                int32_t markdown_start, int32_t markdown_end,
                                float width) {
  if (document_ == nullptr)
    return;
  markdown_start = base::UTF8IndexToCIndex(src, size, markdown_start);
  markdown_end = base::UTF8IndexToCIndex(src, size, markdown_end);
  auto* doc = mkd_string(src, size, 0);
  doc->cb.ud = this;
  doc->cb.paragraph_start = &MarkdownParserEmbed::OnParagraphStart;
  doc->cb.paragraph_end = &MarkdownParserEmbed::OnParagraphEnd;
  doc->cb.paragraph_text = &MarkdownParserEmbed::OnParagraphText;
  doc->cb.align = &MarkdownParserEmbed::OnParagraphAlign;
  doc->cb.header_number = &MarkdownParserEmbed::OnHeaderNumber;
  doc->cb.list_check = &MarkdownParserEmbed::OnListCheck;
  doc->cb.list_index = &MarkdownParserEmbed::OnListIndex;
  doc->cb.list_extra_level = &MarkdownParserEmbed::OnListExtraLevel;
  mkd_initialize();
  context_.char_offset_ = 0;
  context_.markdown_source_ = std::string_view(src, size);
  context_.byte_index_to_char_index_ =
      CalculateByteIndexToCharIndexMap(context_.markdown_source_);
  context_.markdown_start_ = markdown_start;
  context_.markdown_end_ = markdown_end;
  context_.max_width_ = width;
  context_.indent_ = 0;
  document_->ClearForParse();
  style_ = document_->GetStyle();
  loader_ = document_->GetResourceLoader();
  document_->UpdateTruncation(width);
  mkd_compile(doc, 0);
  mkd_cleanup(doc);
}

void MarkdownParserEmbed::OnParagraphStart(int type) {
  context_.para_stack_.push_back(type);
  if (type != OL && type != UL && type != SOURCE && type != QUOTE &&
      type != HR) {
    if ((type != TABLE && context_.current_paragraph_ == nullptr) ||
        (type == TABLE && context_.current_table_ == nullptr)) {
      if (type == TABLE) {
        context_.current_table_ = std::make_unique<MarkdownTable>();
        context_.current_paragraph_ = nullptr;
      } else {
        context_.current_paragraph_ = tttext::Paragraph::Create();
        context_.current_table_ = nullptr;
      }
      context_.text_size_ = style_.normal_text_.base_.font_size_;
      context_.have_normal_text_ = false;
      context_.line_height_rule_ = tttext::RulerType::kExact;
      MarkdownStyleInitializer::ResetBlockStyle(&context_.block_style_);
      MarkdownStyleInitializer::ResetBorderStyle(&context_.border_style_);
      context_.border_type_ = MarkdownBorder::kNone;
      context_.markdown_source_range_ = {0, 0};
      context_.extra_class_ = {};
    }
    float quote_indent = 0;
    if (context_.quote_level_ > 0) {
      context_.border_type_ = MarkdownBorder::kLeft;
      context_.block_style_ = style_.quote_.block_;
      context_.border_style_ = style_.quote_.border_;
      context_.border_style_.border_color_ =
          style_.quote_border_line_.line_.color_;
      context_.border_style_.border_width_ =
          style_.quote_border_line_.line_.width_;
      // TODO(zhouchaoying): temporarily fix quote border, will be removed next
      // commit
      if (context_.quote_level_ == 1) {
        context_.border_style_.border_color_ = 0;
      }

      context_.block_style_.margin_left_ +=
          (context_.quote_level_ - 1) * style_.quote_.indent_.indent_;
      quote_indent = context_.block_style_.margin_left_ +
                     context_.border_style_.border_width_ +
                     context_.block_style_.padding_left_;
    }
    if (context_.list_level_ > 0 && type == LISTITEM &&
        !context_.list_index_stack_.empty()) {
      context_.list_index_stack_.back()++;
      int list_type = context_.para_stack_[context_.para_stack_.size() - 2];
      context_.block_style_ = style_.list_item_.block_;
      if (list_type == UL) {
        float indent =
            style_.unordered_list_.indent_.indent_ >= 0
                ? style_.unordered_list_.indent_.indent_
                : style_.unordered_list_.unordered_list_.mark_size_ +
                      style_.unordered_list_.unordered_list_.mark_margin_right_;
        context_.block_style_.margin_left_ +=
            (context_.list_level_ - 1) * indent;
        AppendUnorderedListMark();
        context_.indent_ += (context_.list_level_ - 1) * indent;
        context_.have_normal_text_ = true;
        if (context_.list_index_stack_.back() == context_.list_start_index_ &&
            context_.list_level_ == 1) {
          // if is the first paragraph in list, add list margin top to
          // paragraph.
          context_.block_style_.margin_top_ +=
              style_.unordered_list_.block_.margin_top_;
          context_.block_style_.padding_top_ +=
              style_.unordered_list_.block_.padding_top_;
        }
      } else if (list_type == OL) {
        float indent =
            style_.ordered_list_.indent_.indent_ >= 0
                ? style_.ordered_list_.indent_.indent_
                : style_.ordered_list_number_.block_.margin_left_ +
                      style_.ordered_list_number_.block_.margin_left_ +
                      style_.ordered_list_.ordered_list_.number_font_size_;
        context_.block_style_.margin_left_ +=
            (context_.list_level_ - 1) * indent;
        AppendOrderedListNumber();
        context_.indent_ += (context_.list_level_ - 1) * indent;
        context_.have_normal_text_ = true;
        if (context_.list_index_stack_.back() == context_.list_start_index_ &&
            context_.list_level_ == 1) {
          context_.block_style_.margin_top_ +=
              style_.ordered_list_.block_.margin_top_;
          context_.block_style_.padding_top_ +=
              style_.ordered_list_.block_.padding_top_;
        }
      }
    } else if (type == CODE) {
      context_.border_type_ = MarkdownBorder::kRect;
      context_.block_style_ = style_.code_block_.block_;
      context_.border_style_ = style_.code_block_.border_;
      context_.block_style_.margin_left_ += quote_indent;
    } else if (type == TABLE) {
      context_.block_style_ = style_.table_.block_;
      context_.border_style_ = style_.table_.border_;
      context_.border_type_ = MarkdownBorder::kNone;
      context_.block_style_.margin_left_ += quote_indent;
    } else if (type == HDR) {
      context_.block_style_ = GetHNBlockStyle(context_.hn_);
      context_.block_style_.margin_left_ += quote_indent;
    } else {
      auto parent_type = context_.para_stack_[context_.para_stack_.size() - 2];
      if (parent_type == SOURCE) {
        // not in other block node
        context_.block_style_ = style_.normal_text_.block_;
      } else if (context_.list_level_ > 0 &&
                 (context_.current_paragraph_ == nullptr ||
                  context_.current_paragraph_->GetRunCount() == 0)) {
        context_.block_style_.margin_left_ += context_.indent_;
      }
    }
  }
  if (type == OL || type == UL) {
    context_.list_level_++;
    context_.list_level_ += context_.list_extra_level_;
    context_.list_level_stack_.emplace_back(context_.list_level_);
    context_.list_index_stack_.emplace_back(context_.list_start_index_ - 1);
  }
  if (type == QUOTE) {
    context_.quote_level_++;
    // TODO(zhouchaoying): temporarily fix quote border, will be removed next
    // commit
    if (context_.quote_level_ == 1) {
      context_.quote_start_para_ = document_->GetParagraphs().size();
    }
  }
}

std::string MarkdownParserEmbed::MarkdownNumberTypeToString(
    lynx::markdown::MarkdownNumberType type, int index) {
  if (type == MarkdownNumberType::kNumber) {
    return std::to_string(index);
  } else if (type == MarkdownNumberType::kAlphabet) {
    std::string str;
    index -= 1;
    if (index < 26) {
      str += static_cast<char>(static_cast<int>('a') + index);
    } else {
      int high = index / 26;
      int low = index % 26;
      str += static_cast<char>(static_cast<int>('a') + high);
      str += static_cast<char>(static_cast<int>('a') + low);
    }
    return str;
  } else if (type == MarkdownNumberType::kRomanNumerals) {
    static const char* level_1[] = {"",  "I",  "II",  "III",  "IV",
                                    "V", "VI", "VII", "VIII", "IX"};
    static const char* level_2[] = {"",  "X",  "XX",  "XXX",  "XL",
                                    "L", "LX", "LXX", "LXXX", "XC"};
    static const char* level_3[] = {"",  "C",  "CC",  "CCC",  "CD",
                                    "D", "DC", "DCC", "DCCC", "CM"};
    std::string str;
    str += level_3[(index / 100) % 10];
    str += level_2[(index / 10) % 10];
    str += level_1[(index) % 10];
    return str;
  } else {
    return std::to_string(index);
  }
}

void MarkdownParserEmbed::OnParagraphEnd() {
  int type = context_.para_stack_.back();
  context_.para_stack_.pop_back();
  if (type == QUOTE) {
    // TODO(zhouchaoying): temporarily fix quote border, will be removed next
    // commit
    if (context_.quote_level_ == 1) {
      if (context_.quote_start_para_ >= 0 &&
          context_.quote_start_para_ <
              static_cast<int>(document_->GetParagraphs().size())) {
        document_->AddQuoteRange(
            Range{context_.quote_start_para_,
                  static_cast<int>(document_->GetParagraphs().size())});
        document_->GetParagraphs().back()->SetSpaceAfter(0);
      }
    }
    context_.quote_level_--;
  } else if ((type == OL || type == UL) &&
             !context_.list_index_stack_.empty()) {
    if (context_.list_index_stack_.back() != context_.list_start_index_ - 1 &&
        context_.list_level_ == 1) {
      // if list has at least one paragraph, add list margin bottom to the last
      // paragraph.
      MarkdownBlockStylePart* list_block_style = nullptr;
      if (type == OL) {
        list_block_style = &style_.ordered_list_.block_;
      } else if (type == UL) {
        list_block_style = &style_.unordered_list_.block_;
      }
      if (list_block_style != nullptr && !document_->GetParagraphs().empty()) {
        auto& last_para = document_->GetParagraphs().back();
        auto block = last_para->GetBlockStyle();
        block.margin_bottom_ += list_block_style->margin_bottom_;
        block.padding_bottom_ += list_block_style->padding_bottom_;
        last_para->SetBlockStyle(block);
      }
    }
    context_.list_level_--;
    context_.list_level_stack_.pop_back();
    context_.list_index_stack_.pop_back();
    context_.list_level_ = context_.list_level_stack_.empty()
                               ? 0
                               : context_.list_level_stack_.back();
    context_.list_start_index_ = 1;
    context_.indent_ = 0;
  }
  if (type == HR) {
    if (context_.processed_markdown_length_ >= context_.markdown_start_ &&
        context_.processed_markdown_length_ < context_.markdown_end_) {
      MarkdownStyleInitializer::ResetBlockStyle(&context_.block_style_);
      MarkdownStyleInitializer::ResetBorderStyle(&context_.border_style_);
      context_.border_style_ = style_.split_.border_;
      context_.block_style_ = style_.split_.block_;
      context_.border_type_ = MarkdownBorder::kTop;
      auto para = std::make_shared<MarkdownElement>(MarkdownElementType::kNone);
      para->SetSpaceAfter(style_.normal_text_.base_.paragraph_space_);
      GenerateElement(para.get());
      document_->AddParagraph(std::move(para));
    }
  } else if (context_.current_paragraph_ != nullptr &&
             context_.current_paragraph_->GetCharCount() > 0) {
    auto para = std::make_shared<MarkdownParagraphElement>();
    GenerateParagraph(type, para.get());
    para->SetMarkdownSourceType(
        ParagraphTypeToSyntaxType(static_cast<ParagraphType>(type)));
    para->SetMarkdownSourceRange(
        Range{.start_ = MarkdownSourceByteIndexToCharIndex(
                  context_.markdown_source_range_.start_),
              .end_ = MarkdownSourceByteIndexToCharIndex(
                  context_.markdown_source_range_.end_)});
    document_->AddParagraph(std::move(para));
  } else if (context_.current_table_ != nullptr &&
             !context_.current_table_->Empty()) {
    auto table = std::make_shared<MarkdownTableElement>();
    GenerateTable(table.get());
    document_->AddParagraph(std::move(table));
  }
  if (type == HDR) {
    context_.hn_ = -1;
  }
  context_.current_paragraph_ = nullptr;
  context_.current_table_ = nullptr;
}

void MarkdownParserEmbed::SetParagraphStyle(
    const lynx::markdown::MarkdownBaseStylePart& base_style_part,
    tttext::ParagraphStyle* style, MarkdownElement* element) {
  if (base_style_part.line_height_ > 0) {
    style->SetLineHeightInPx(base_style_part.line_height_,
                             context_.line_height_rule_);
  }
  if (base_style_part.line_space_ >= 0) {
    style->SetLineSpaceAfterPx(base_style_part.line_space_);
  }
  style->AllowBreakAroundPunctuation(
      document_->GetAllowBreakAroundPunctuation());
  if (base_style_part.text_overflow_ == MarkdownTextOverflow::kEllipsis) {
    if (!document_->GetTruncationText().empty()) {
      style->SetEllipsis(std::u16string(document_->GetTruncationText()));
    } else if (document_->GetTruncationDelegate() != nullptr) {
      style->SetEllipsis(document_->GetTruncationDelegate());
    }
  }
  if (base_style_part.text_maxline_ > 0) {
    style->SetMaxLines(base_style_part.text_maxline_);
  }
  if (element != nullptr) {
    element->SetTextOverflow(base_style_part.text_overflow_);
    if (base_style_part.paragraph_space_ >= 0) {
      element->SetSpaceAfter(base_style_part.paragraph_space_);
    }
    if (base_style_part.last_line_alignment_ != MarkdownTextAlign::kUndefined) {
      element->SetLastLineAlign(base_style_part.last_line_alignment_);
    }
  }
  if (base_style_part.direction_ != MarkdownDirection::kNormal) {
    style->SetWriteDirection(ConvertWriteDirection(base_style_part.direction_));
  }
  if (base_style_part.text_align_ != MarkdownTextAlign::kUndefined) {
    style->SetHorizontalAlign(ConvertTextAlign(base_style_part.text_align_));
  }
  if (base_style_part.text_indent_ > 0) {
    style->SetFirstLineIndentInPx(base_style_part.text_indent_);
  }
  tttext::Style default_style = style->GetDefaultStyle();
  SetTTStyleByMarkdownBaseStyle(base_style_part, &default_style);
  style->SetDefaultStyle(default_style);
}

void MarkdownParserEmbed::SetTTStyleByMarkdownBaseStyle(
    MarkdownDocument* document, const MarkdownBaseStylePart& base_style_part,
    tttext::Style* style) {
  auto loader = document->GetResourceLoader();
  if (loader != nullptr &&
      (!base_style_part.font_.empty() ||
       (base_style_part.font_weight_ != MarkdownFontWeight::kNormal &&
        base_style_part.font_weight_ != MarkdownFontWeight::kBold))) {
    auto font = loader->LoadFont(base_style_part.font_.c_str(),
                                 base_style_part.font_weight_);
    style->SetFontDescriptor(
        {{}, tttext::FontStyle::Normal(), reinterpret_cast<uint64_t>(font)});
  }
  if (base_style_part.font_style_ != MarkdownFontStyle::kUndefined) {
    style->SetItalic(base_style_part.font_style_ == MarkdownFontStyle::kItalic);
  }
  if (base_style_part.font_size_ >= 0) {
    style->SetTextSize(base_style_part.font_size_);
  }
  if (base_style_part.color_ != MarkdownStyleInitializer::COLOR_UNDEFINED) {
    style->SetForegroundColor(tttext::TTColor(base_style_part.color_));
  }
  if (base_style_part.background_color_ !=
      MarkdownStyleInitializer::COLOR_UNDEFINED) {
    style->SetBackgroundColor(
        tttext::TTColor(base_style_part.background_color_));
  }
  if (base_style_part.word_break_ == MarkdownWordBreak::kBreakAll) {
    style->SetWordBreak(tttext::WordBreakType::kBreakAll);
  }
  if (base_style_part.font_weight_ == MarkdownFontWeight::kBold) {
    style->SetBold(true);
  }
}

void MarkdownParserEmbed::SetTTStyleByMarkdownBaseStyle(
    const lynx::markdown::MarkdownBaseStylePart& base_style_part,
    tttext::Style* style) {
  SetTTStyleByMarkdownBaseStyle(document_, base_style_part, style);
}

void MarkdownParserEmbed::SetDecorationStyle(
    const lynx::markdown::MarkdownDecorationStylePart& decoration_style_part,
    tttext::Style* style) {
  if (decoration_style_part.text_decoration_line_ ==
          MarkdownTextDecorationLine::kNone ||
      decoration_style_part.text_decoration_style_ ==
          MarkdownTextDecorationStyle::kNone) {
    return;
  }
  style->SetDecorationColor(
      tttext::TTColor(decoration_style_part.text_decoration_color_));
  style->SetDecorationThicknessMultiplier(
      decoration_style_part.text_decoration_thickness_);
  style->SetDecorationStyle(
      ConvertDecorationStyle(decoration_style_part.text_decoration_style_));
  style->SetDecorationType(
      ConvertDecorationLine(decoration_style_part.text_decoration_line_));
}

tttext::DecorationType MarkdownParserEmbed::ConvertDecorationLine(
    lynx::markdown::MarkdownTextDecorationLine line) {
  switch (line) {
    case MarkdownTextDecorationLine::kUnderline:
      return ttoffice::tttext::DecorationType::kUnderLine;
    case MarkdownTextDecorationLine::kOverline:
      return ttoffice::tttext::DecorationType::kOverline;
    case MarkdownTextDecorationLine::kLineThrough:
      return ttoffice::tttext::DecorationType::kLineThrough;
    default:
      return ttoffice::tttext::DecorationType::kNone;
  }
}

tttext::LineType MarkdownParserEmbed::ConvertDecorationStyle(
    lynx::markdown::MarkdownTextDecorationStyle type) {
  switch (type) {
    case MarkdownTextDecorationStyle::kSolid:
      return ttoffice::tttext::LineType::kSolid;
    case MarkdownTextDecorationStyle::kDouble:
      return ttoffice::tttext::LineType::kDouble;
    case MarkdownTextDecorationStyle::kDotted:
      return ttoffice::tttext::LineType::kDotted;
    case MarkdownTextDecorationStyle::kDashed:
      return ttoffice::tttext::LineType::kDashed;
    case MarkdownTextDecorationStyle::kWavy:
      return ttoffice::tttext::LineType::kWavy;
    default:
      return ttoffice::tttext::LineType::kNone;
  }
}

tttext::CharacterVerticalAlignment MarkdownParserEmbed::ConvertVerticalAlign(
    MarkdownVerticalAlign align) {
  switch (align) {
    case MarkdownVerticalAlign::kBaseline:
      return tttext::CharacterVerticalAlignment::kBaseLine;
    case MarkdownVerticalAlign::kTop:
      return tttext::CharacterVerticalAlignment::kTop;
    case MarkdownVerticalAlign::kBottom:
      return tttext::CharacterVerticalAlignment::kBottom;
    case MarkdownVerticalAlign::kCenter:
      return tttext::CharacterVerticalAlignment::kMiddle;
    default:
      return tttext::CharacterVerticalAlignment::kBaseLine;
  }
}

tttext::WriteDirection MarkdownParserEmbed::ConvertWriteDirection(
    MarkdownDirection direction) {
  switch (direction) {
    case MarkdownDirection::kLtr:
      return tttext::WriteDirection::kLTR;
    case MarkdownDirection::kRtl:
      return tttext::WriteDirection::kRTL;
    default:
      return tttext::WriteDirection::kAuto;
  }
}

tttext::ParagraphHorizontalAlignment MarkdownParserEmbed::ConvertTextAlign(
    MarkdownTextAlign align) {
  switch (align) {
    default:
    case MarkdownTextAlign::kLeft:
      return tttext::ParagraphHorizontalAlignment::kLeft;
    case MarkdownTextAlign::kCenter:
      return tttext::ParagraphHorizontalAlignment::kCenter;
    case MarkdownTextAlign::kRight:
      return tttext::ParagraphHorizontalAlignment::kRight;
    case MarkdownTextAlign::kJustify:
      return tttext::ParagraphHorizontalAlignment::kJustify;
  }
}

std::pair<uint32_t, uint32_t>
MarkdownParserEmbed::GetTextLineByteRangeByMarkdownRange(uint32_t line_offset,
                                                         uint32_t line_length) {
  uint32_t line_start = line_offset;
  uint32_t line_end = line_offset + line_length;
  if (line_end == line_start)
    return {-1, -1};
  if (line_start >= context_.markdown_end_) {
    return {0, 0};
  }
  if (line_end < context_.markdown_start_) {
    return {0, 0};
  }
  return {std::max(line_start, context_.markdown_start_) - line_start,
          std::min(line_end, context_.markdown_end_) - line_start};
}

void MarkdownParserEmbed::OnParagraphText(line* text_line) {
  if (text_line == nullptr) {
    return;
  }
  auto* first_line = text_line;
  auto* last_line = text_line;
  while (last_line->next != nullptr)
    last_line = last_line->next;
  context_.processed_markdown_length_ =
      static_cast<uint32_t>(last_line->markdown_offset + last_line->text.size);
  context_.markdown_source_range_ =
      Range{.start_ = first_line->markdown_offset,
            .end_ = last_line->markdown_offset + last_line->text.size};
  line* text_end = nullptr;
  if (context_.para_stack_.back() == CODE) {
    if (strncmp(text_line->text.text + text_line->dle, "```", 3) == 0) {
      auto end = text_line;
      while (end->next != nullptr) {
        end = end->next;
      }
      if (end != text_line &&
          strncmp(end->text.text + end->dle, "```", 3) == 0) {
        text_end = end;
      }
      text_line = text_line->next;
    }
  }
  if (context_.para_stack_.back() == CODE) {
    context_.have_normal_text_ = true;
    while (text_line != text_end) {
      auto [line_start, line_end] = GetTextLineByteRangeByMarkdownRange(
          static_cast<uint32_t>(text_line->markdown_offset),
          static_cast<uint32_t>(text_line->text.size));
      if (line_end <= line_start) {
        text_line = text_line->next;
        continue;
      }
      char* content = text_line->text.text + line_start;
      int len = line_end - line_start;
      tttext::Style run_style;
      SetTTStyleByMarkdownBaseStyle(style_.code_block_.base_, &run_style);
      int32_t char_start =
          context_.current_paragraph_->GetCharCount() + context_.char_offset_;
      context_.current_paragraph_->AddTextRun(&run_style, content, len);
      int32_t char_end =
          context_.current_paragraph_->GetCharCount() + context_.char_offset_;
      auto markdown_start = text_line->markdown_offset + line_start;
      auto markdown_end = text_line->markdown_offset + line_end;
      document_->markdown_index_to_char_index_.emplace_back(
          Range{MarkdownSourceByteIndexToCharIndex(markdown_start),
                MarkdownSourceByteIndexToCharIndex(markdown_end)},
          Range{char_start, char_end});
      text_line = text_line->next;
      if (text_line != text_end) {
        context_.current_paragraph_->AddTextRun(&run_style, "\n");
      }
    }
  } else if (context_.para_stack_.back() == TABLE) {
    if (static_cast<uint32_t>(text_line->markdown_offset) >=
            context_.markdown_start_ &&
        context_.markdown_end_ >
            static_cast<uint32_t>(text_line->markdown_offset)) {
      context_.enable_split_render_ = false;
      HandleTableLines(text_line);
      context_.enable_split_render_ = true;
    }
  } else {
    tttext::Style run_style;
    if (context_.hn_ <= 0 || context_.hn_ > 6) {
      if (context_.quote_level_ > 0) {
        SetTTStyleByMarkdownBaseStyle(style_.quote_.base_, &run_style);
      } else if (context_.list_level_ > 0) {
        int list_type =
            context_.para_stack_.size() < 3
                ? -1
                : context_.para_stack_[context_.para_stack_.size() - 3];
        if (list_type == OL || list_type == UL) {
          if ((context_.markdown_start_ >
                   static_cast<uint32_t>(text_line->markdown_offset) ||
               context_.markdown_end_ <=
                   static_cast<uint32_t>(text_line->markdown_offset)) &&
              context_.current_paragraph_ != nullptr) {
            // remove list mark/number but apply list indent
            auto para = std::move(context_.current_paragraph_);
            context_.current_paragraph_ = tttext::Paragraph::Create();
            auto para_style = para->GetParagraphStyle();
            para_style.SetFirstLineIndentInPx(
                para_style.GetHangingIndentInPx());
            context_.current_paragraph_->SetParagraphStyle(&para_style);
          }
        }
        if (list_type == OL) {
          SetTTStyleByMarkdownBaseStyle(style_.ordered_list_.base_, &run_style);
        } else if (list_type == UL) {
          SetTTStyleByMarkdownBaseStyle(style_.unordered_list_.base_,
                                        &run_style);
        }
      } else {
        SetTTStyleByMarkdownBaseStyle(style_.normal_text_.base_, &run_style);
      }
    } else {
      SetTTStyleByMarkdownBaseStyle(GetHNStyle(context_.hn_), &run_style);
    }
    std::string content = "";
    int32_t markdown_offset = text_line->markdown_offset;
    context_.lines_offset_.emplace_back(0);
    int32_t offset{0};
    while (text_line != text_end) {
      auto [line_start, line_end] = GetTextLineByteRangeByMarkdownRange(
          static_cast<uint32_t>(text_line->markdown_offset),
          static_cast<uint32_t>(text_line->text.size));
      if (line_start >= line_end) {
        text_line = text_line->next;
        continue;
      }
      content += std::string_view(text_line->text.text, text_line->text.size);
      int32_t text_line_end_offset =
          text_line->markdown_offset + text_line->text.size;
      text_line = text_line->next;
      if (text_line != text_end) {
        if (!content.empty()) {
          content += "\n";
        }
        offset += text_line->markdown_offset - text_line_end_offset - 1;
        context_.lines_offset_.emplace_back(offset);
      }
    }
    ParseInlineSyntax(content, context_.current_paragraph_.get(), run_style,
                      &context_.have_normal_text_, context_.char_offset_,
                      markdown_offset, true);
    context_.lines_offset_.clear();
    context_.line_index_ = 0;
  }
}

void MarkdownParserEmbed::GenerateElement(
    lynx::markdown::MarkdownElement* element) {
  element->SetBlockStyle(context_.block_style_);
  element->SetBorderStyle(context_.border_style_);
  element->SetBorderType(context_.border_type_);
  element->SetCharStart(context_.char_offset_);
}

void MarkdownParserEmbed::GenerateParagraph(
    int type, lynx::markdown::MarkdownParagraphElement* para) {
  if (context_.quote_level_ > 0) {
    SetParagraphStyle(style_.quote_.base_,
                      &context_.current_paragraph_->GetParagraphStyle(), para);
  } else if (context_.list_level_ > 0) {
    int list_type = context_.para_stack_[context_.para_stack_.size() - 2];
    if (list_type == UL) {
      SetParagraphStyle(style_.unordered_list_.base_,
                        &context_.current_paragraph_->GetParagraphStyle(),
                        para);
    } else if (list_type == OL) {
      SetParagraphStyle(style_.ordered_list_.base_,
                        &context_.current_paragraph_->GetParagraphStyle(),
                        para);
    }
  } else if (type == CODE) {
    SetParagraphStyle(style_.code_block_.base_,
                      &context_.current_paragraph_->GetParagraphStyle(), para);
  } else if (context_.hn_ > 0) {
    SetParagraphStyle(GetHNStyle(context_.hn_),
                      &context_.current_paragraph_->GetParagraphStyle(), para);
  } else {
    SetParagraphStyle(style_.normal_text_.base_,
                      &context_.current_paragraph_->GetParagraphStyle(), para);
  }
  if (!context_.extra_class_.empty()) {
    const auto style = style_.span_styles_.find(context_.extra_class_);
    if (style != style_.span_styles_.end()) {
      SetParagraphStyle(style->second.base_,
                        &context_.current_paragraph_->GetParagraphStyle(),
                        para);
      const auto& block = style->second.block_;
      context_.block_style_.margin_top_ = block.margin_top_;
      context_.block_style_.padding_top_ = block.padding_top_;
      context_.block_style_.margin_bottom_ = block.margin_bottom_;
      context_.block_style_.padding_bottom_ = block.padding_bottom_;
    }
    context_.extra_class_ = {};
  }
  if (!context_.have_normal_text_) {
    context_.current_paragraph_->GetParagraphStyle().SetHorizontalAlign(
        tttext::ParagraphHorizontalAlignment::kCenter);
    context_.current_paragraph_->GetParagraphStyle().SetVerticalAlign(
        tttext::ParagraphVerticalAlignment::kCenter);
  }
  GenerateElement(para);
  auto char_count = context_.current_paragraph_->GetCharCount();
  para->SetCharCount(char_count);
  context_.char_offset_ += char_count;
  para->SetParagraph(std::move(context_.current_paragraph_));
  if (type == CODE) {
    para->SetScrollX(style_.code_block_.scroll_.scroll_x_);
  }
}

void MarkdownParserEmbed::GenerateTable(
    lynx::markdown::MarkdownTableElement* table) {
  GenerateElement(table);
  context_.current_table_->SetCellStyle(style_.table_cell_.block_);
  context_.current_table_->SetCellBackground(
      style_.table_cell_.base_.background_color_);
  context_.current_table_->SetHeaderStyle(style_.table_header_.block_);
  context_.current_table_->SetHeaderBackground(
      style_.table_header_.base_.background_color_);
  context_.current_table_->SetTableStyle(style_.table_.table_);
  table->SetCharCount(context_.current_table_->GetCharCount());
  context_.char_offset_ += context_.current_table_->GetCharCount();
  table->SetTable(std::move(context_.current_table_));
  table->SetSpaceAfter(0);
  table->SetTextOverflow(style_.table_cell_.base_.text_overflow_);
  table->SetScrollX(style_.table_.scroll_.scroll_x_);
}

void MarkdownParserEmbed::AppendUnorderedListMark() {
  MarkdownMarkType mark_type = style_.unordered_list_marker_.marker_.mark_type_;
  if (mark_type == MarkdownMarkType::kMixed) {
    mark_type = static_cast<MarkdownMarkType>(
        (context_.list_level_ - 1) %
        (static_cast<int>(MarkdownMarkType::kMixed)));
  }
  auto mark = std::make_unique<MarkdownUnorderedListMarkDelegate>(
      mark_type, style_.unordered_list_marker_);
  context_.indent_ = mark->GetAdvance();
  context_.current_paragraph_->GetParagraphStyle().SetHangingIndentInPx(
      mark->GetAdvance());
  tttext::Style style;
  style.SetVerticalAlignment(ConvertVerticalAlign(
      style_.unordered_list_marker_.align_.vertical_align_));
  context_.current_paragraph_->AddShapeRun(&style, std::move(mark), false);
}

void MarkdownParserEmbed::AppendOrderedListNumber() {
  MarkdownNumberType number_type =
      style_.ordered_list_.ordered_list_.number_type_;
  if (number_type == MarkdownNumberType::kMixed) {
    number_type = static_cast<MarkdownNumberType>(
        (context_.list_level_ - 1) %
        (static_cast<int>(MarkdownNumberType::kMixed)));
  }
  tttext::Style number_style;
  SetTTStyleByMarkdownBaseStyle(style_.ordered_list_number_.base_,
                                &number_style);
  auto number_str = MarkdownNumberTypeToString(
                        number_type, context_.list_index_stack_.back()) +
                    ".";
  auto tmp_para = tttext::Paragraph::Create();
  if (style_.ordered_list_number_.block_.margin_left_ > 0) {
    context_.current_paragraph_->AddGhostShapeRun(
        nullptr, std::make_unique<MarkdownEmptySpaceDelegate>(
                     style_.ordered_list_number_.block_.margin_left_));
  }
  context_.current_paragraph_->AddTextRun(&number_style, number_str.c_str(),
                                          number_str.length());
  if (style_.ordered_list_number_.block_.margin_right_ > 0) {
    context_.current_paragraph_->AddGhostShapeRun(
        nullptr, std::make_unique<MarkdownEmptySpaceDelegate>(
                     style_.ordered_list_number_.block_.margin_right_));
  }
  tmp_para->AddTextRun(&number_style, number_str.c_str(), number_str.length());
  auto [width, _] = MarkdownLayout::MeasureParagraph(
      tmp_para.get(), std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(), -1);
  auto indent = width + style_.ordered_list_number_.block_.margin_left_ +
                style_.ordered_list_number_.block_.margin_right_;
  context_.indent_ = indent;
  context_.current_paragraph_->GetParagraphStyle().SetHangingIndentInPx(indent);
}

void MarkdownParserEmbed::HandleTableLines(line* text_line) {
  if (context_.current_table_ == nullptr) {
    return;
  }
  int line_count = 0;
  for (line* tmp = text_line; tmp != nullptr; tmp = tmp->next) {
    line_count++;
  }
  // remove align line
  line_count -= 1;

  auto cell_base = style_.table_cell_.base_;
  tttext::ParagraphStyle paragraph_style;
  SetParagraphStyle(cell_base, &paragraph_style, nullptr);
  tttext::Style run_style;
  SetTTStyleByMarkdownBaseStyle(cell_base, &run_style);
  // header
  auto* header_line = text_line;
  auto header_string =
      TrimSpace({text_line->text.text, (size_t)(text_line->text.size)});
  auto split = Split(header_string, '|');
  if (line_count > 0 && !split.empty()) {
    context_.current_table_->Resize(line_count, split.size());
  }
  size_t col_count = split.size();
  text_line = text_line->next;
  // align
  auto align =
      HandleTableAlign({text_line->text.text, (size_t)(text_line->text.size)});
  if (align.size() < split.size()) {
    align.resize(split.size(), tttext::ParagraphHorizontalAlignment::kLeft);
  }
  text_line = text_line->next;
  // content
  uint32_t char_offset = 0;
  for (size_t col = 0; col < col_count; col++) {
    auto str = TrimSpace(split[col]);
    if (str.empty()) {
      continue;
    }
    auto para = tttext::Paragraph::Create();
    auto header_style = run_style;
    auto header_base = style_.table_header_.base_;
    header_base.background_color_ = 0;
    SetTTStyleByMarkdownBaseStyle(header_base, &header_style);
    para->SetParagraphStyle(&paragraph_style);
    para->GetParagraphStyle().SetHorizontalAlign(align[col]);
    ParseInlineSyntax(
        std::string(str), para.get(), header_style, nullptr,
        char_offset + context_.char_offset_,
        str.data() - header_line->text.text + header_line->markdown_offset,
        false);
    uint32_t char_count = para->GetCharCount();
    context_.current_table_->SetCell(
        0, col,
        MarkdownTableCell{
            .paragraph_ = std::move(para),
            .alignment_ = align[col],
            .vertical_alignment_ = style_.table_header_.align_.vertical_align_,
            .char_start_ = char_offset,
            .char_count_ = char_count});
    char_offset += char_count;
  }
  int row_index = 1;
  while (text_line != nullptr) {
    auto line_string =
        TrimSpace({text_line->text.text, (size_t)(text_line->text.size)});
    split = Split(line_string, '|');
    for (size_t col = 0; col < col_count; col++) {
      if (col >= split.size())
        continue;
      auto str = TrimSpace(split[col]);
      if (str.empty())
        continue;
      auto para = tttext::Paragraph::Create();
      para->SetParagraphStyle(&paragraph_style);
      para->GetParagraphStyle().SetHorizontalAlign(align[col]);
      ParseInlineSyntax(
          std::string(str), para.get(), run_style, nullptr,
          char_offset + context_.char_offset_,
          str.data() - text_line->text.text + text_line->markdown_offset,
          false);
      uint32_t char_count = para->GetCharCount();
      context_.current_table_->SetCell(
          row_index, col,
          MarkdownTableCell{
              .paragraph_ = std::move(para),
              .alignment_ = align[col],
              .vertical_alignment_ = style_.table_cell_.align_.vertical_align_,
              .char_start_ = char_offset,
              .char_count_ = char_count});
      char_offset += char_count;
    }
    row_index++;
    text_line = text_line->next;
  }
  context_.current_table_->char_count_ = char_offset;
}

const MarkdownBaseStylePart& MarkdownParserEmbed::GetHNStyle(int hn) {
  switch (hn) {
    case 1:
      return style_.h1_.base_;
    case 2:
      return style_.h2_.base_;
    case 3:
      return style_.h3_.base_;
    case 4:
      return style_.h4_.base_;
    case 5:
      return style_.h5_.base_;
    case 6:
      return style_.h6_.base_;
    default:
      return style_.normal_text_.base_;
  }
}

const MarkdownBlockStylePart& MarkdownParserEmbed::GetHNBlockStyle(int hn) {
  switch (hn) {
    case 1:
      return style_.h1_.block_;
    case 2:
      return style_.h2_.block_;
    case 3:
      return style_.h3_.block_;
    case 4:
      return style_.h4_.block_;
    case 5:
      return style_.h5_.block_;
    case 6:
      return style_.h6_.block_;
    default:
      return style_.normal_text_.block_;
  }
}

std::vector<std::string_view> MarkdownParserEmbed::Split(
    const std::string_view& content, char split) {
  if (content.empty()) {
    return {};
  }
  size_t start = content[0] == split ? 1 : 0;
  std::vector<std::string_view> strs;
  for (size_t end = start; end < content.size(); end++) {
    if (content[end] == '\\') {
      end++;
    } else if (content[end] == split) {
      strs.emplace_back(content.substr(start, end - start));
      start = end + 1;
    }
  }
  if (start < content.size()) {
    strs.emplace_back(content.substr(start, content.size() - start));
  }
  return strs;
}

std::string_view MarkdownParserEmbed::TrimSpace(std::string_view origin) {
  size_t start = 0;
  for (; start < origin.size(); start++) {
    if (origin[start] != ' ' && origin[start] != '\t') {
      break;
    }
  }
  int end = origin.size() - 1;
  for (; end >= 0; end--) {
    if (origin[end] != ' ' && origin[end] != '\t') {
      break;
    }
  }
  return origin.substr(start, end - start + 1);
}

std::vector<tttext::ParagraphHorizontalAlignment>
MarkdownParserEmbed::HandleTableAlign(const std::string_view& content) {
  auto split = Split(content, '|');
  std::vector<tttext::ParagraphHorizontalAlignment> aligns(split.size());
  for (size_t i = 0; i < split.size(); i++) {
    auto str = TrimSpace(split[i]);
    if (str.size() < 2) {
      aligns[i] = tttext::ParagraphHorizontalAlignment::kLeft;
    } else {
      bool align_left = str.front() == ':';
      bool align_right = str.back() == ':';
      if (align_left && align_right) {
        aligns[i] = tttext::ParagraphHorizontalAlignment::kCenter;
      } else if (align_right) {
        aligns[i] = tttext::ParagraphHorizontalAlignment::kRight;
      } else {
        aligns[i] = tttext::ParagraphHorizontalAlignment::kLeft;
      }
    }
  }
  return aligns;
}

void MarkdownParserEmbed::OnListCheck(int checked) {
  context_.list_checked_ = checked;
}

void MarkdownParserEmbed::OnParagraphAlign(int align_type) {}

void MarkdownParserEmbed::OnHeaderNumber(int hn) {
  context_.hn_ = hn;
}

void MarkdownParserEmbed::OnListIndex(int index) {
  context_.list_start_index_ = index;
}

void MarkdownParserEmbed::OnListExtraLevel(int list_extra_level) {
  context_.list_extra_level_ = list_extra_level;
}

void MarkdownParserEmbed::ParseInlineSyntax(
    const std::string& content, tttext::Paragraph* para,
    const tttext::Style& base_style, bool* have_normal_text,
    uint32_t char_offset, uint32_t markdown_offset, bool check_paragraph_tag) {
  auto result = MarkdownInlineSyntaxParser::Parse(content);
  if (have_normal_text != nullptr) {
    for (auto& child : result->Children()) {
      if (child->GetSyntax() != MarkdownInlineSyntax::kImg) {
        *have_normal_text = true;
        break;
      }
    }
  }
  if (check_paragraph_tag && result->Children().size() == 1 &&
      result->Children().front()->GetSyntax() ==
          MarkdownInlineSyntax::kInlineHtml) {
    if (const auto* node = static_cast<MarkdownInlineHtmlTag*>(
            result->Children().front().get());
        node->GetTag() == "p" && !node->GetClass().empty()) {
      context_.extra_class_ = node->GetClass();
    }
  }
  AppendNodeToParagraph(result.get(), para, base_style, char_offset,
                        markdown_offset);
}

void MarkdownParserEmbed::AppendLinkToParagraph(MarkdownLinkNode* node,
                                                tttext::Paragraph* para,
                                                const tttext::Style& base_style,
                                                uint32_t char_offset,
                                                uint32_t markdown_offset) {
  tttext::Style new_style = base_style;
  SetTTStyleByMarkdownBaseStyle(style_.link_.base_, &new_style);
  uint32_t count_before = para->GetCharCount();
  AppendChildrenToParagraph(node, para, new_style, char_offset,
                            markdown_offset);
  uint32_t count_after = para->GetCharCount();
  tttext::Style decoration_style;
  SetDecorationStyle(style_.link_.decoration_, &decoration_style);
  para->ApplyStyleInRange(decoration_style, count_before,
                          count_after - count_before);
  if (char_offset != static_cast<uint32_t>(-1)) {
    auto link_content =
        para->GetContentString(count_before, count_after - count_before);
    document_->links_.emplace_back(
        MarkdownLink{.url_ = std::string(node->GetLink()),
                     .content_ = link_content,
                     .char_start_ = char_offset + count_before,
                     .char_count_ = count_after - count_before,
                     .attached_paragraph_ = para,
                     .char_start_in_paragraph_ = count_before});
  }
}

void MarkdownParserEmbed::AppendImgToParagraph(MarkdownImageNode* node,
                                               tttext::Paragraph* para,
                                               const tttext::Style& base_style,
                                               uint32_t char_offset,
                                               uint32_t markdown_offset) {
  std::string url(node->GetUrl());
  float max_height = -1;
  bool is_inline_view = base::BeginsWith(url, kInlineViewSchema);
  bool is_block_view = base::BeginsWith(url, kBlockViewSchema);
  float indent = context_.indent_ + context_.block_style_.margin_left_ +
                 context_.block_style_.padding_left_ +
                 context_.border_style_.border_width_ * 2 +
                 context_.block_style_.margin_right_ +
                 context_.block_style_.padding_right_;
  float max_width =
      is_block_view ? context_.max_width_ : context_.max_width_ - indent;
  if (is_inline_view || is_block_view) {
    // inline view or block view
    context_.have_normal_text_ = true;
    std::string inline_view_id = is_inline_view
                                     ? url.substr(strlen(kInlineViewSchema))
                                     : url.substr(strlen(kBlockViewSchema));
    if (!inline_view_id.empty() && loader_ != nullptr) {
      auto delegate = loader_->LoadInlineView(inline_view_id.c_str(), max_width,
                                              max_height);
      if (delegate != nullptr) {
        document_->AddInlineView(MarkdownInlineView{
            .id_ = inline_view_id,
            .char_index_ =
                static_cast<int32_t>(char_offset + para->GetCharCount()),
            .is_block_view_ = is_block_view,
            .view_ = delegate.get(),
        });
        if (is_block_view) {
          delegate = std::make_unique<BlockViewWrapper>(
              context_.max_width_, indent, std::move(delegate));
        }
        document_->SetShapeRunAltString(char_offset + para->GetCharCount(),
                                        node->GetAltText());
        para->AddShapeRun(&base_style, std::move(delegate), false);
      }
    }
  } else {
    // img
    float width = style_.image_.size_.width_;
    float height = style_.image_.size_.height_;
    if (node->GetWidth() > 0) {
      width = node->GetWidth();
    }
    if (node->GetHeight() > 0) {
      height = node->GetHeight();
    }
    bool need_alt_text = false;
    if (loader_ != nullptr) {
      auto delegate =
          loader_->LoadImage(url.c_str(), width, height, max_width, max_height,
                             style_.image_.image_.radius_);
      if (delegate == nullptr && !(style_.image_.image_.alt_image_.empty())) {
        delegate = loader_->LoadImage(style_.image_.image_.alt_image_.c_str(),
                                      width, height, max_width, max_height,
                                      style_.image_.image_.radius_);
      }
      if (delegate != nullptr) {
        document_->AddImage(MarkdownImage{
            .url_ = url,
            .char_index_ =
                static_cast<int32_t>(para->GetCharCount() + char_offset),
            .image_ = delegate.get(),
        });
        if (!node->GetCaption().empty()) {
          auto caption = tttext::Paragraph::Create();
          auto style = base_style;
          SetTTStyleByMarkdownBaseStyle(style_.image_caption_.base_, &style);
          SetParagraphStyle(style_.image_caption_.base_,
                            &(caption->GetParagraphStyle()), nullptr);
          caption->AddTextRun(&style, node->GetCaption().data(),
                              node->GetCaption().length());
          delegate = std::make_unique<ImageWithCaption>(
              std::move(delegate), std::move(caption), max_width,
              style_.image_caption_.image_caption_.caption_position_,
              style_.image_caption_.base_.text_align_);
        }
        document_->SetShapeRunAltString(char_offset + para->GetCharCount(),
                                        node->GetAltText());
        para->AddShapeRun(&base_style, std::move(delegate), false);
      } else {
        need_alt_text = style_.image_.image_.enable_alt_text_;
      }
    } else {
      need_alt_text = style_.image_.image_.enable_alt_text_;
    }

    if (need_alt_text && !node->GetAltText().empty()) {
      if (max_width > 0 && width > max_width) {
        height *= max_width / width;
        width = max_width;
      }
      if (max_height > 0 && height > max_height) {
        width *= max_height / height;
        height = max_height;
      }
      auto alt_text = tttext::Paragraph::Create();
      tttext::ParagraphStyle paragraph_style = alt_text->GetParagraphStyle();
      paragraph_style.SetHorizontalAlign(
          tttext::ParagraphHorizontalAlignment::kCenter);
      alt_text->SetParagraphStyle(&paragraph_style);
      alt_text->AddTextRun(&base_style, "\ufffd");
      context_.enable_split_render_ = false;
      AppendChildrenToParagraph(node, alt_text.get(), base_style, char_offset,
                                markdown_offset);
      context_.enable_split_render_ = true;
      document_->SetShapeRunAltString(char_offset + para->GetCharCount(),
                                      node->GetAltText());
      para->AddShapeRun(&base_style,
                        std::make_unique<MarkdownTextDelegate>(
                            std::move(alt_text), width, height),
                        false);
    }
  }
  context_.line_height_rule_ = tttext::RulerType::kAtLeast;
}

void MarkdownParserEmbed::AppendInlineBorderLeft(
    const MarkdownBlockStylePart& block, const MarkdownBorderStylePart& border,
    MarkdownBackgroundStylePart* background, tttext::Paragraph* para,
    tttext::Style* style) const {
  float left_empty =
      block.margin_left_ + block.padding_left_ + border.border_width_;
  if (left_empty != 0) {
    auto left_delegate =
        std::make_unique<MarkdownEmptySpaceDelegate>(left_empty);
    para->AddGhostShapeRun(nullptr, std::move(left_delegate));
  }
  if (block.padding_left_ > 0 || block.padding_right_ > 0 ||
      (background != nullptr && !background->background_image_.empty())) {
    style->SetBackgroundColor(tttext::TTColor());
  }
}

void MarkdownParserEmbed::AppendInlineBorderRight(
    const MarkdownBaseStylePart& base, const MarkdownBlockStylePart& block,
    const MarkdownBorderStylePart& border,
    MarkdownBackgroundStylePart* background, tttext::Paragraph* para,
    uint32_t char_offset, uint32_t char_offset_end) const {
  float right_empty =
      block.margin_right_ + block.padding_right_ + border.border_width_;
  if (right_empty != 0) {
    auto right_delegate =
        std::make_unique<MarkdownEmptySpaceDelegate>(right_empty);
    para->AddGhostShapeRun(nullptr, std::move(right_delegate));
  }
  if (char_offset_end <= char_offset) {
    return;
  }
  if (block.padding_left_ > 0 || block.padding_right_ > 0 ||
      (background != nullptr && !background->background_image_.empty())) {
    auto attachment = std::make_unique<MarkdownTextAttachment>();
    attachment->start_index_ = char_offset;
    attachment->end_index_ = char_offset_end;
    attachment->rect_.left_ = std::make_unique<MarkdownLengthValue>(
        -block.padding_left_, StyleValuePattern::kPx);
    attachment->rect_.right_ = std::make_unique<MarkdownCalculateValue>(
        std::make_unique<MarkdownLengthValue>(100, StyleValuePattern::kPercent),
        OperatorType::kAdd,
        std::make_unique<MarkdownLengthValue>(block.padding_right_,
                                              StyleValuePattern::kPx));
    if (background != nullptr && !background->background_image_.empty() &&
        loader_ != nullptr) {
      attachment->rect_.gradient_ = loader_->LoadGradient(
          background->background_image_.c_str(), base.font_size_,
          style_.normal_text_.base_.font_size_);
    }
    if (attachment->rect_.gradient_ == nullptr) {
      attachment->rect_.color_ = base.background_color_;
    }
    attachment->rect_.radius_ = std::make_unique<MarkdownLengthValue>(
        border.border_radius_, StyleValuePattern::kPx);
    if (border.border_width_ > 0) {
      attachment->rect_.stroke_color_ = border.border_color_;
      attachment->rect_.stroke_width_ = std::make_unique<MarkdownLengthValue>(
          border.border_width_, StyleValuePattern::kPx);
    }
    document_->border_attachments_.emplace_back(std::move(attachment));
  }
}

void MarkdownParserEmbed::AppendInlineCode(MarkdownInlineNode* node,
                                           tttext::Paragraph* para,
                                           const tttext::Style& base_style,
                                           uint32_t char_offset,
                                           uint32_t markdown_offset) {
  if (node->Children().empty())
    return;
  auto new_style = base_style;
  SetTTStyleByMarkdownBaseStyle(style_.inline_code_.base_, &new_style);
  AppendInlineBorderLeft(style_.inline_code_.block_,
                         style_.inline_code_.border_, nullptr, para,
                         &new_style);
  auto char_start = char_offset + para->GetCharCount();
  AppendChildrenToParagraph(node, para, new_style, char_offset,
                            markdown_offset);
  auto char_end = char_offset + para->GetCharCount();
  AppendInlineBorderRight(style_.inline_code_.base_, style_.inline_code_.block_,
                          style_.inline_code_.border_, nullptr, para,
                          char_start, char_end);
}

void MarkdownParserEmbed::AppendRawText(MarkdownInlineNode* node,
                                        tttext::Paragraph* para,
                                        const tttext::Style& base_style,
                                        uint32_t char_offset,
                                        uint32_t markdown_offset) {
  if (node->GetSyntax() == MarkdownInlineSyntax::kBreakLine) {
    para->AddTextRun(&base_style, "\n");
    context_.line_index_++;
    return;
  }
  int32_t piece_start = 0;
  int32_t piece_end = node->GetText().length();
  if (context_.enable_split_render_) {
    auto range = GetTextLineByteRangeByMarkdownRange(
        markdown_offset + piece_start, piece_end - piece_start);
    if (range.first < range.second) {
      piece_end = range.second + piece_start;
      piece_start = range.first + piece_start;
    } else {
      piece_start = 0;
      piece_end = 0;
    }
  }
  if (piece_start == piece_end)
    return;
  std::string piece_str;
  if (node->GetSyntax() == MarkdownInlineSyntax::kHtmlEntity) {
    piece_str = static_cast<MarkdownHtmlEntityNode*>(node)->GetEntity();
    piece_end = piece_start + 1;
  } else {
    piece_str = std::string(
        node->GetText().substr(piece_start, piece_end - piece_start));
  }
  uint32_t char_start = para->GetCharCount();
  para->AddTextRun(&base_style, piece_str.c_str());
  uint32_t char_end = para->GetCharCount();
  int32_t markdown_start = markdown_offset + piece_start;
  int32_t markdown_end = markdown_offset + piece_end;
  if (context_.line_index_ > 0 &&
      context_.line_index_ < context_.lines_offset_.size()) {
    const auto offset = context_.lines_offset_[context_.line_index_];
    markdown_start += offset;
    markdown_end += offset;
  }
  document_->markdown_index_to_char_index_.emplace_back(
      Range{MarkdownSourceByteIndexToCharIndex(markdown_start),
            MarkdownSourceByteIndexToCharIndex(markdown_end)},
      Range{static_cast<int32_t>(char_start + char_offset),
            static_cast<int32_t>(char_end + char_offset)});
}

void MarkdownParserEmbed::AppendInlineHtml(MarkdownInlineHtmlTag* node,
                                           tttext::Paragraph* para,
                                           const tttext::Style& base_style,
                                           uint32_t char_offset,
                                           uint32_t markdown_offset) {
  auto new_style = base_style;
  if (node->GetTag() == "br") {
    para->AddTextRun(&base_style, "\n");
  } else if (node->GetTag() == "mark") {
    SetTTStyleByMarkdownBaseStyle(style_.mark_.base_, &new_style);
    AppendInlineBorderLeft(style_.mark_.block_, style_.mark_.border_,
                           &style_.mark_.background_, para, &new_style);
    auto start = char_offset + para->GetCharCount();
    AppendChildrenToParagraph(node, para, new_style, char_offset,
                              markdown_offset);
    auto end = char_offset + para->GetCharCount();
    AppendInlineBorderRight(style_.mark_.base_, style_.mark_.block_,
                            style_.mark_.border_, &style_.mark_.background_,
                            para, start, end);
  } else if (node->GetTag() == "span") {
    const auto cls = std::string(node->GetClass());
    const auto iter = style_.span_styles_.find(cls);
    if (iter == style_.span_styles_.end()) {
      AppendChildrenToParagraph(node, para, new_style, char_offset,
                                markdown_offset);
    } else {
      auto& span_style = iter->second;
      SetTTStyleByMarkdownBaseStyle(span_style.base_, &new_style);
      uint32_t count_before = para->GetCharCount();
      AppendInlineBorderLeft(span_style.block_, span_style.border_,
                             &span_style.background_, para, &new_style);
      auto start = char_offset + para->GetCharCount();
      AppendChildrenToParagraph(node, para, new_style, char_offset,
                                markdown_offset);
      auto end = char_offset + para->GetCharCount();
      AppendInlineBorderRight(span_style.base_, span_style.block_,
                              span_style.border_, &span_style.background_, para,
                              start, end);
      uint32_t count_after = para->GetCharCount();
      tttext::Style decoration_style;
      SetDecorationStyle(span_style.decoration_, &decoration_style);
      para->ApplyStyleInRange(decoration_style, count_before,
                              count_after - count_before);
    }
  } else {
    AppendChildrenToParagraph(node, para, new_style, char_offset,
                              markdown_offset);
  }
}

void MarkdownParserEmbed::AppendDoubleBraces(MarkdownInlineNode* node,
                                             tttext::Paragraph* para,
                                             const tttext::Style& base_style,
                                             uint32_t char_offset,
                                             uint32_t markdown_offset) {
  auto new_style = base_style;
  context_.block_style_.margin_top_ += style_.double_braces_.block_.margin_top_;
  context_.block_style_.margin_bottom_ +=
      style_.double_braces_.block_.margin_bottom_;
  SetTTStyleByMarkdownBaseStyle(style_.double_braces_.base_, &new_style);
  AppendInlineBorderLeft(style_.double_braces_.block_,
                         style_.double_braces_.border_,
                         &style_.double_braces_.background_, para, &new_style);
  auto start = para->GetCharCount() + char_offset;
  AppendChildrenToParagraph(node, para, new_style, char_offset,
                            markdown_offset);
  auto end = char_offset + para->GetCharCount();
  AppendInlineBorderRight(style_.double_braces_.base_,
                          style_.double_braces_.block_,
                          style_.double_braces_.border_,
                          &style_.double_braces_.background_, para, start, end);
}

void MarkdownParserEmbed::AppendDoubleSquareBracket(
    MarkdownInlineNode* node, tttext::Paragraph* para,
    const tttext::Style& base_style, uint32_t char_offset,
    uint32_t markdown_offset) {
  auto new_style = base_style;
  const auto& ref_style = style_.ref_;
  SetTTStyleByMarkdownBaseStyle(ref_style.base_, &new_style);
  new_style.SetBackgroundColor(tttext::TTColor(0));
  auto new_para = tttext::Paragraph::Create();
  AppendChildrenToParagraph(node, new_para.get(), new_style, char_offset,
                            markdown_offset);
  auto delegate = std::make_unique<MarkdownRefDelegate>(
      std::move(new_para), style_.ref_, base_style.GetTextSize());
  para->AddGhostShapeRun(&new_style, std::move(delegate));
}

void MarkdownParserEmbed::AppendNodeToParagraph(MarkdownInlineNode* node,
                                                tttext::Paragraph* para,
                                                const tttext::Style& base_style,
                                                uint32_t char_offset,
                                                uint32_t markdown_offset) {
  if (context_.enable_split_render_) {
    if (markdown_offset >= context_.markdown_end_ ||
        markdown_offset + node->GetText().length() < context_.markdown_start_) {
      return;
    }
  }
  if (node->GetSyntax() == MarkdownInlineSyntax::kRawText ||
      node->GetSyntax() == MarkdownInlineSyntax::kHtmlEntity ||
      node->GetSyntax() == MarkdownInlineSyntax::kBreakLine) {
    AppendRawText(node, para, base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kImg) {
    AppendImgToParagraph(reinterpret_cast<MarkdownImageNode*>(node), para,
                         base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kLink) {
    AppendLinkToParagraph(reinterpret_cast<MarkdownLinkNode*>(node), para,
                          base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kInlineHtml) {
    AppendInlineHtml(reinterpret_cast<MarkdownInlineHtmlTag*>(node), para,
                     base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kInlineCode) {
    AppendInlineCode(node, para, base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kDoubleBraces) {
    AppendDoubleBraces(node, para, base_style, char_offset, markdown_offset);
  } else if (node->GetSyntax() == MarkdownInlineSyntax::kDoubleSquareBrackets) {
    AppendDoubleSquareBracket(node, para, base_style, char_offset,
                              markdown_offset);
  } else {
    tttext::Style new_style = base_style;
    if (node->GetSyntax() == MarkdownInlineSyntax::kBold) {
      SetTTStyleByMarkdownBaseStyle(style_.bold_.base_, &new_style);
    } else if (node->GetSyntax() == MarkdownInlineSyntax::kBoldItalic) {
      SetTTStyleByMarkdownBaseStyle(style_.bold_.base_, &new_style);
      SetTTStyleByMarkdownBaseStyle(style_.italic_.base_, &new_style);
    } else if (node->GetSyntax() == MarkdownInlineSyntax::kItalic) {
      SetTTStyleByMarkdownBaseStyle(style_.italic_.base_, &new_style);
    } else if (node->GetSyntax() == MarkdownInlineSyntax::kDelete) {
      new_style.SetDecorationType(tttext::DecorationType::kLineThrough);
      new_style.SetDecorationStyle(tttext::LineType::kSolid);
      new_style.SetDecorationColor(new_style.GetForegroundColor());
      new_style.SetDecorationThicknessMultiplier(1);
    }
    AppendChildrenToParagraph(node, para, new_style, char_offset,
                              markdown_offset);
  }
}

void MarkdownParserEmbed::AppendChildrenToParagraph(
    lynx::markdown::MarkdownInlineNode* node, tttext::Paragraph* para,
    const tttext::Style& base_style, uint32_t char_offset,
    uint32_t markdown_offset) {
  for (auto& child : node->Children()) {
    AppendNodeToParagraph(
        child.get(), para, base_style, char_offset,
        markdown_offset + child->GetText().data() - node->GetText().data());
  }
}

int32_t MarkdownParserEmbed::MarkdownSourceByteIndexToCharIndex(
    int32_t byte_index) const {
  if (byte_index <= 0)
    return 0;
  const auto& map = context_.byte_index_to_char_index_;
  if (static_cast<size_t>(byte_index) >= map.size())
    return map.back();
  return map[byte_index];
}

std::vector<int32_t> MarkdownParserEmbed::CalculateByteIndexToCharIndexMap(
    const std::string_view string) {
  int32_t char_index = 0;
  std::vector<int32_t> result;
  result.reserve(string.size() + 1);
  for (const auto c : string) {
    result.emplace_back(char_index);
    if (IsUtf8StartByte(c)) {
      char_index++;
    }
  }
  result.emplace_back(char_index);
  return result;
}
std::vector<std::string_view> SplitLines(std::string_view content) {
  std::vector<std::string_view> result;
  uint32_t last_index = 0;
  auto find = content.find('\n', last_index);
  while (find != std::string_view::npos) {
    result.emplace_back(content.substr(last_index, find - last_index + 1));
    last_index = find + 1;
    find = content.find('\n', last_index);
  }
  return result;
}

void MarkdownParserEmbed::ParsePlainText(const char* src, int size) {
  auto content = std::string_view(src, size);
  document_->ClearForParse();
  document_->UpdateTruncation(document_->GetMaxWidth());
  auto lines = SplitLines(content);
  uint32_t char_count = 0;
  for (auto line : lines) {
    auto para = tttext::Paragraph::Create();
    auto& style = document_->GetStyle().normal_text_;
    tttext::ParagraphStyle paragraph_style;
    tttext::Style run_style;
    SetParagraphStyle(style.base_, &paragraph_style, nullptr);
    SetTTStyleByMarkdownBaseStyle(style.base_, &run_style);
    paragraph_style.SetDefaultStyle(run_style);
    para->SetParagraphStyle(&paragraph_style);
    para->AddTextRun(&run_style, line.data(), line.length());
    auto para_element = std::make_unique<MarkdownParagraphElement>();
    para_element->SetBlockStyle(style.block_);
    para_element->SetCharStart(char_count);
    auto char_count_after = char_count + para->GetCharCount();
    para_element->SetCharCount(char_count_after);
    para_element->SetMarkdownSourceRange(
        {static_cast<int32_t>(char_count),
         static_cast<int32_t>(char_count_after)});
    char_count = char_count_after;
    para_element->SetParagraph(std::move(para));
    document_->AddParagraph(std::move(para_element));
  }
}

}  // namespace lynx::markdown
