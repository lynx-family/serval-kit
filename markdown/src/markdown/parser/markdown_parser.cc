// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/markdown_parser.h"

#include <unordered_map>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/discount/markdown_parser_discount.h"
namespace lynx::markdown {

class ParserMap {
 public:
  ParserMap() { default_parser_ = std::make_unique<MarkdownParserDiscount>(); }
  MarkdownParser* GetParser(const std::string& name) {
    if (name.empty()) {
      return default_parser_.get();
    }
    const auto iter = parser_map_.find(name);
    if (iter == parser_map_.end() || iter->second == nullptr) {
      return default_parser_.get();
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
  std::unique_ptr<MarkdownParser> default_parser_;
};

ParserMap& GetParserMap() {
  static ParserMap parser_map;
  return parser_map;
}

void MarkdownParser::RegisterParser(const std::string& name,
                                    MarkdownParser* parser) {
  GetParserMap().RegisterParser(name, parser);
}

void MarkdownParser::Parse(const std::string& parser_name,
                           MarkdownDocument* document, void* ud) {
  const auto parser = GetParserMap().GetParser(parser_name);
  if (parser == nullptr)
    return;
  parser->Parse(document, ud);
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

void MarkdownParser::ParsePlainText(MarkdownDocument* document) {
  auto content = document->GetMarkdownContent();
  document->ClearForParse();
  document->UpdateTruncation(document->GetMaxWidth());
  auto lines = SplitLines(content);
  uint32_t char_count = 0;
  for (auto line : lines) {
    auto para = tttext::Paragraph::Create();
    auto& style = document->GetStyle().normal_text_;
    tttext::ParagraphStyle paragraph_style;
    tttext::Style run_style;
    SetParagraphStyle(document, style.base_, &paragraph_style, nullptr,
                      tttext::RulerType::kExact);
    SetTTStyleByMarkdownBaseStyle(document, style.base_, &run_style);
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
    document->AddParagraph(std::move(para_element));
  }
}

void MarkdownParser::SetParagraphStyle(
    MarkdownDocument* document, const MarkdownBaseStylePart& base_style_part,
    tttext::ParagraphStyle* style, MarkdownElement* element,
    tttext::RulerType line_height_rule) {
  if (base_style_part.line_height_ > 0) {
    style->SetLineHeightInPx(base_style_part.line_height_, line_height_rule);
  }
  if (base_style_part.line_space_ >= 0) {
    style->SetLineSpaceAfterPx(base_style_part.line_space_);
  }
  style->AllowBreakAroundPunctuation(
      document->GetAllowBreakAroundPunctuation());
  if (base_style_part.text_overflow_ == MarkdownTextOverflow::kEllipsis) {
    if (!document->GetTruncationText().empty()) {
      style->SetEllipsis(std::u16string(document->GetTruncationText()));
    } else if (document->GetTruncationDelegate() != nullptr) {
      style->SetEllipsis(document->GetTruncationDelegate());
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
  SetTTStyleByMarkdownBaseStyle(document, base_style_part, &default_style);
  style->SetDefaultStyle(default_style);
}

void MarkdownParser::SetTTStyleByMarkdownBaseStyle(
    MarkdownDocument* document,
    const lynx::markdown::MarkdownBaseStylePart& base_style_part,
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

void MarkdownParser::SetDecorationStyle(
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

tttext::DecorationType MarkdownParser::ConvertDecorationLine(
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

tttext::LineType MarkdownParser::ConvertDecorationStyle(
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

tttext::CharacterVerticalAlignment MarkdownParser::ConvertVerticalAlign(
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

std::string MarkdownParser::MarkdownNumberTypeToString(MarkdownNumberType type,
                                                       int index) {
  if (type == MarkdownNumberType::kNumber) {
    return std::to_string(index);
  }
  if (type == MarkdownNumberType::kAlphabet) {
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
  }
  if (type == MarkdownNumberType::kRomanNumerals) {
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
  }
  return std::to_string(index);
}

void MarkdownParser::AppendInlineBorderLeft(
    const MarkdownBlockStylePart& block, const MarkdownBorderStylePart& border,
    MarkdownBackgroundStylePart* background, tttext::Paragraph* para,
    tttext::Style* style) {
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

void MarkdownParser::AppendInlineBorderRight(
    MarkdownDocument* document, const MarkdownBaseStylePart& base,
    const MarkdownBlockStylePart& block, const MarkdownBorderStylePart& border,
    MarkdownBackgroundStylePart* background, tttext::Paragraph* para,
    uint32_t char_offset, uint32_t char_offset_end) {
  auto* loader = document->GetResourceLoader();
  auto& style = document->GetStyle();
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
        loader != nullptr) {
      attachment->rect_.gradient_ = loader->LoadGradient(
          background->background_image_.c_str(), base.font_size_,
          style.normal_text_.base_.font_size_);
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
    document->AddInlineBorder(std::move(attachment));
  }
}

const MarkdownBaseStylePart& MarkdownParser::GetHNStyle(
    const MarkdownStyle& style, int hn) {
  switch (hn) {
    case 1:
      return style.h1_.base_;
    case 2:
      return style.h2_.base_;
    case 3:
      return style.h3_.base_;
    case 4:
      return style.h4_.base_;
    case 5:
      return style.h5_.base_;
    case 6:
      return style.h6_.base_;
    default:
      return style.normal_text_.base_;
  }
}

const MarkdownBlockStylePart& MarkdownParser::GetHNBlockStyle(
    const MarkdownStyle& style, int hn) {
  switch (hn) {
    case 1:
      return style.h1_.block_;
    case 2:
      return style.h2_.block_;
    case 3:
      return style.h3_.block_;
    case 4:
      return style.h4_.block_;
    case 5:
      return style.h5_.block_;
    case 6:
      return style.h6_.block_;
    default:
      return style.normal_text_.block_;
  }
}
tttext::WriteDirection MarkdownParser::ConvertWriteDirection(
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
tttext::ParagraphHorizontalAlignment MarkdownParser::ConvertTextAlign(
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
}  // namespace lynx::markdown
