// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/parser/markdown_parser.h"

#include <unordered_map>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_element.h"
#include "markdown/element/markdown_list_item.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/element/markdown_table.h"
#include "markdown/parser/embed/markdown_parser_embed.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/parser/markdown_dom_node.h"
namespace lynx::markdown {

class ParserProviderMap {
 public:
  ParserProviderMap() = default;
  MarkdownParserProvider* GetParserProvider(const std::string& name) {
    const auto iter = parser_map_.find(name);
    if (iter == parser_map_.end() || iter->second == nullptr) {
      return nullptr;
    }
    return iter->second;
  }
  void RegisterParser(const std::string& name, MarkdownParserProvider* parser) {
    if (parser == nullptr || name.empty()) {
      return;
    }
    parser_map_[name] = parser;
  }
  std::unordered_map<std::string, MarkdownParserProvider*> parser_map_;
};

ParserProviderMap& GetParserMap() {
  static ParserProviderMap parser_map;
  return parser_map;
}

void MarkdownParserProvider::RegisterParserProvider(
    const std::string& name, MarkdownParserProvider* parser) {
  GetParserMap().RegisterParser(name, parser);
}

void MarkdownParserImpl::ParseMarkdown(const std::string& parser_name,
                                       MarkdownDocument* document, void* ud) {
  if (!parser_name.empty()) {
    if (const auto provider = GetParserMap().GetParserProvider(parser_name);
        provider != nullptr) {
      const auto parser = provider->CreateParser();
      auto* dom = parser->Parse(document->GetMarkdownContent(), ud);
      ConvertDomTree(document, dom);
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

enum class ConvertState : uint8_t { kBlock, kParagraph, kTable };
struct MarkdownConverterState {
  MarkdownElement* element_;
  tttext::Style run_style_;
  tttext::ParagraphStyle paragraph_style_;
  MarkdownDomType node_type_;
};
struct MarkdownConverterContext {
  std::vector<MarkdownConverterState> state_stack_;
  std::vector<int32_t> list_index_stack_;
  std::vector<float> max_width_stack_;
  int32_t node_id_{0};
  int32_t list_level_{0};
  std::shared_ptr<tttext::RunDelegate> marker_;

  int32_t hn_{0};

  std::vector<MarkdownTextAlign> table_alignment_{};
  int32_t row_index_{0};
  int32_t col_index_{0};

  uint32_t char_offset_{0};
  uint32_t table_offset_{0};

  MarkdownConverterState& GetCurrentState() { return state_stack_.back(); }
  MarkdownElement* GetElement() const { return state_stack_.back().element_; }
  MarkdownBlockElement* GetBlock() const {
    return static_cast<MarkdownBlockElement*>(GetElement());
  }
  MarkdownParagraphElement* GetParagraphElement() const {
    return static_cast<MarkdownParagraphElement*>(GetElement());
  }
  tttext::Paragraph* GetParagraph() const {
    auto type = GetElement()->GetType();
    if (type == MarkdownElementType::kTable) {
      return GetTable()->GetCell(row_index_, col_index_).paragraph_.get();
    }
    return GetParagraphElement()->GetParagraph();
  }
  MarkdownTableElement* GetTableElement() const {
    return static_cast<MarkdownTableElement*>(GetElement());
  }
  MarkdownTable* GetTable() const { return GetTableElement()->GetTable(); }
  MarkdownListItem* GetListItem() const {
    return static_cast<MarkdownListItem*>(GetElement());
  }
};

class MarkdownConverter {
 public:
  MarkdownConverter() = default;
  ~MarkdownConverter() = default;
  void Convert(MarkdownDocument* document, MarkdownDomNode* dom_node);

 protected:
  std::unique_ptr<MarkdownElement> ConvertNode(MarkdownDomNode* node);
  static void MakeElementPlain(std::unique_ptr<MarkdownElement> block_element,
                               MarkdownDocument* target, float left = 0,
                               float top = 0, float right = 0,
                               float bottom = 0);
  void ConvertBlockChild(MarkdownDomNode* node);
  void ConvertInlineChild(MarkdownDomNode* node);
  void ConvertInlineNode(MarkdownDomNode* node);

  void ConvertParagraph(MarkdownDomNode* node);
  void ConvertBlock(MarkdownDomNode* node);
  void ConvertTable(MarkdownDomNode* node);
  void ConvertTableRow(MarkdownDomNode* node);
  void ConvertTableCell(MarkdownDomNode* node);
  std::unique_ptr<MarkdownElement> ReplaceBlockNode(MarkdownDomNode* node);
  void ReplaceInlineNode(MarkdownDomNode* node);

  void ConvertImage(MarkdownDomNode* node);

  void EnterInlineHtml(MarkdownDomNode* node);

  void UpdateBlockNodeStyle();
  void UpdateBaseStyle();
  void UpdateBlockStyle();
  void UpdateDecorationStyle(uint32_t char_start, uint32_t char_end);
  void UpdateInlineBorderLeft(MarkdownDomType type);
  void UpdateInlineBorderRight(MarkdownDomType type, uint32_t char_start,
                               uint32_t char_end);
  void UpdateListItemMarker(MarkdownDomNode* node);

  static std::unique_ptr<MarkdownElement> GenerateElement(MarkdownDomType type);
  void PushState(MarkdownElement* element, MarkdownDomType type);
  void PushInlineState(MarkdownDomType type);
  void PopState();
  void PushMaxWidth(MarkdownElement* element);
  void PopMaxWidth();
  const MarkdownBaseStylePart* GetBaseStyleByType(MarkdownDomType type);
  const MarkdownBlockStylePart* GetBlockStyleByType(MarkdownDomType type);
  const MarkdownBorderStylePart* GetBorderStyleByType(MarkdownDomType type);
  const MarkdownDecorationStylePart* GetDecorationStyle(MarkdownDomType type);
  const MarkdownBackgroundStylePart* GetBackgroundStyleByType(
      MarkdownDomType type);
  const MarkdownScrollStylePart* GetScrollStyleByType(MarkdownDomType type);
  static MarkdownBorder GetBorderTypeByType(MarkdownDomType type);
  static MarkdownSyntaxType GetSyntaxType(MarkdownDomType type);
  static tttext::ParagraphHorizontalAlignment ConvertHorizontalAlignment(
      MarkdownTextAlign align);

  MarkdownDocument* document_{nullptr};

  MarkdownDomNode* root_{};

  std::unique_ptr<MarkdownElement> root_element_;
  MarkdownConverterContext context_;
};

void MarkdownConverter::Convert(MarkdownDocument* document,
                                MarkdownDomNode* dom_node) {
  if (document == nullptr || dom_node == nullptr)
    return;
  document_ = document;
  root_ = dom_node;
  document_->ClearForParse();
  document_->UpdateTruncation(document_->GetMaxWidth());
  context_.max_width_stack_.emplace_back(document_->GetMaxWidth());
  root_element_ = ConvertNode(root_);
  MakeElementPlain(std::move(root_element_), document_);
}

std::unique_ptr<MarkdownElement> MarkdownConverter::ReplaceBlockNode(
    MarkdownDomNode* node) {
  auto id = std::to_string(reinterpret_cast<uint64_t>(node));
  float max_width = context_.max_width_stack_.back();
  auto delegate = document_->GetResourceLoader()->LoadReplacementView(
      node, context_.node_id_, max_width, document_->GetMaxHeight());
  if (delegate != nullptr) {
    document_->AddInlineView(
        MarkdownInlineView{id, static_cast<int32_t>(context_.char_offset_),
                           false, delegate.get()});
    auto element = std::make_unique<MarkdownParagraphElement>();
    auto para = tttext::Paragraph::Create();
    tttext::Style style;
    style.SetVerticalAlignment(tttext::CharacterVerticalAlignment::kTop);
    style.SetForegroundColor(tttext::TTColor(1));
    // make view cannot be selected
    para->AddTextRun(&style, "V");
    para->AddGhostShapeRun(&style, std::move(delegate));
    para->GetParagraphStyle().SetLineHeightInPxAtLeast(
        para->GetParagraphStyle().GetLineHeightInPx());
    element->SetParagraph(std::move(para));
    element->SetCharStart(context_.char_offset_);
    element->SetCharCount(1);
    context_.char_offset_++;
    return element;
  }
  return nullptr;
}

void MarkdownConverter::ConvertParagraph(MarkdownDomNode* node) {
  const auto type = node->GetType();
  auto para_element = context_.GetParagraphElement();
  auto para = tttext::Paragraph::Create();
  para->SetParagraphStyle(&(context_.GetCurrentState().paragraph_style_));
  para_element->SetParagraph(std::move(para));
  para_element->SetCharStart(context_.char_offset_);
  if (type == MarkdownDomType::kParagraph || type == MarkdownDomType::kHeader) {
    if (context_.marker_ != nullptr) {
      auto run_style = context_.GetCurrentState().run_style_;
      run_style.SetVerticalAlignment(
          tttext::CharacterVerticalAlignment::kMiddle);
      context_.GetParagraph()->AddGhostShapeRun(&run_style, context_.marker_);
      context_.marker_ = nullptr;
    }
  }
  ConvertInlineChild(node);
  para_element->SetCharCount(context_.GetParagraph()->GetCharCount());
  context_.char_offset_ += para_element->GetCharCount();
}

void MarkdownConverter::ConvertBlock(MarkdownDomNode* node) {
  const auto type = node->GetType();
  if (type == MarkdownDomType::kListItem) {
    UpdateListItemMarker(node);
    context_.list_index_stack_.back()++;
  } else if (type == MarkdownDomType::kOrderedList ||
             type == MarkdownDomType::kUnorderedList) {
    auto* list_node = static_cast<MarkdownDomList*>(node);
    auto start_index = list_node->GetStart();
    context_.list_level_++;
    context_.list_index_stack_.emplace_back(start_index);
  }
  ConvertBlockChild(node);
  if (type == MarkdownDomType::kOrderedList ||
      type == MarkdownDomType::kUnorderedList) {
    context_.list_level_--;
    context_.list_index_stack_.pop_back();
  }
}

void MarkdownConverter::ConvertTable(MarkdownDomNode* node) {
  auto* table_element = context_.GetTableElement();
  table_element->SetTable(std::make_unique<MarkdownTable>());
  auto* table = context_.GetTable();
  table->SetTableStyle(document_->GetStyle().table_.table_);
  table->SetHeaderStyle(document_->GetStyle().table_header_.block_);
  table->SetCellStyle(document_->GetStyle().table_cell_.block_);
  table_element->SetCharStart(context_.char_offset_);
  int32_t row_count = 1;
  auto* start = node->GetFirstChild();
  while (start != node->GetLastChild()) {
    start = start->GetNext();
    row_count++;
  }
  auto* table_node = static_cast<MarkdownDomTable*>(node);
  context_.table_alignment_ = table_node->GetAligns();
  table->Resize(row_count,
                static_cast<int32_t>(context_.table_alignment_.size()));
  context_.row_index_ = 0;
  context_.table_offset_ = context_.char_offset_;
  for (auto* child = node->GetFirstChild(); child != nullptr;
       child = child->GetNext()) {
    ConvertTableRow(static_cast<MarkdownDomNode*>(child));
    context_.row_index_++;
  }
  context_.table_alignment_.clear();
  context_.row_index_ = 0;
  context_.table_offset_ = 0;
  table_element->SetCharCount(context_.char_offset_ -
                              table_element->GetCharStart());
}

void MarkdownConverter::ConvertTableRow(MarkdownDomNode* node) {
  context_.col_index_ = 0;
  for (auto* child = node->GetFirstChild(); child != nullptr;
       child = child->GetNext()) {
    ConvertTableCell(static_cast<MarkdownDomNode*>(child));
    context_.col_index_++;
  }
  context_.col_index_ = 0;
}

void MarkdownConverter::ConvertTableCell(MarkdownDomNode* node) {
  const bool is_header = context_.row_index_ == 0;
  const auto& style = document_->GetStyle();
  const auto type = node->GetType();
  PushInlineState(type);
  auto para = tttext::Paragraph::Create();
  para->SetParagraphStyle(&(context_.GetCurrentState().paragraph_style_));
  context_.GetTable()->SetCell(
      context_.row_index_, context_.col_index_,
      MarkdownTableCell{
          .paragraph_ = std::move(para),
          .alignment_ = ConvertHorizontalAlignment(
              context_.table_alignment_[context_.col_index_]),
          .vertical_alignment_ =
              is_header ? style.table_header_.align_.vertical_align_
                        : style.table_cell_.align_.vertical_align_,
          .char_start_ = context_.char_offset_ - context_.table_offset_,
      });
  ConvertInlineChild(node);
  auto& cell =
      context_.GetTable()->GetCell(context_.row_index_, context_.col_index_);
  cell.char_count_ = cell.paragraph_->GetCharCount();
  context_.char_offset_ += cell.char_count_;
  PopState();
}

tttext::ParagraphHorizontalAlignment
MarkdownConverter::ConvertHorizontalAlignment(MarkdownTextAlign align) {
  switch (align) {
    case MarkdownTextAlign::kLeft:
    default:
      return tttext::ParagraphHorizontalAlignment::kLeft;
    case MarkdownTextAlign::kCenter:
      return tttext::ParagraphHorizontalAlignment::kCenter;
    case MarkdownTextAlign::kRight:
      return tttext::ParagraphHorizontalAlignment::kRight;
  }
}

std::unique_ptr<MarkdownElement> MarkdownConverter::ConvertNode(
    MarkdownDomNode* node) {
  context_.node_id_++;
  const auto type = node->GetType();
  if (type == MarkdownDomType::kPlaceHolder) {
    return ReplaceBlockNode(node);
  }
  std::unique_ptr<MarkdownElement> element = GenerateElement(type);
  if (type == MarkdownDomType::kHeader) {
    context_.hn_ = static_cast<MarkdownDomHeader*>(node)->GetHN();
  }
  PushState(element.get(), type);
  PushMaxWidth(element.get());
  if (type == MarkdownDomType::kParagraph ||
      type == MarkdownDomType::kCodeBlock || type == MarkdownDomType::kHeader) {
    ConvertParagraph(node);
  } else if (type == MarkdownDomType::kTable) {
    ConvertTable(node);
  } else {
    ConvertBlock(node);
  }
  PopMaxWidth();
  PopState();
  return element;
}

void MarkdownConverter::ConvertBlockChild(MarkdownDomNode* node) {
  auto* child = node->GetFirstChild();
  while (child != nullptr) {
    auto child_element = ConvertNode(static_cast<MarkdownDomNode*>(child));
    if (child_element != nullptr) {
      context_.GetBlock()->AddChild(std::move(child_element));
    }
    child = child->GetNext();
  }
}

void MarkdownConverter::ConvertInlineChild(MarkdownDomNode* node) {
  auto* child = node->GetFirstChild();
  while (child != nullptr) {
    ConvertInlineNode(static_cast<MarkdownDomNode*>(child));
    child = child->GetNext();
  }
}

void MarkdownConverter::ConvertImage(MarkdownDomNode* node) {
  auto* image_node = static_cast<MarkdownDomImage*>(node);
  std::string url(image_node->GetUrl());
  auto max_width = context_.max_width_stack_.back();
  auto image = document_->GetResourceLoader()->LoadImage(
      url.c_str(), image_node->GetWidth(), image_node->GetHeight(), max_width,
      document_->GetMaxHeight(), document_->GetStyle().image_.image_.radius_);
  if (image != nullptr) {
    auto para = context_.GetParagraph();
    document_->AddImage(MarkdownImage{
        url, static_cast<int32_t>(para->GetCharCount() + context_.char_offset_),
        image.get()});
    para->AddShapeRun(&(context_.GetCurrentState().run_style_),
                      std::move(image), false);
    para->GetParagraphStyle().SetLineHeightInPxAtLeast(
        para->GetParagraphStyle().GetLineSpaceAfterPx());
  }
}

void MarkdownConverter::ReplaceInlineNode(MarkdownDomNode* node) {
  auto id = std::to_string(reinterpret_cast<uint64_t>(node));
  float max_width = context_.max_width_stack_.back();
  auto view = document_->GetResourceLoader()->LoadReplacementView(
      node, context_.node_id_, max_width, document_->GetMaxHeight());
  if (view != nullptr) {
    tttext::Style style = context_.GetCurrentState().run_style_;
    auto* current_para = context_.GetParagraph();
    document_->AddInlineView(
        MarkdownInlineView{id,
                           static_cast<int32_t>(context_.char_offset_ +
                                                current_para->GetCharCount()),
                           false, view.get()});
    style.SetVerticalAlignment(tttext::CharacterVerticalAlignment::kMiddle);
    current_para->AddShapeRun(&style, std::move(view), false);
    current_para->GetParagraphStyle().SetLineHeightInPxAtLeast(
        current_para->GetParagraphStyle().GetLineSpaceAfterPx());
  }
}

void MarkdownConverter::ConvertInlineNode(MarkdownDomNode* node) {
  context_.node_id_++;
  const auto type = node->GetType();
  if (type == MarkdownDomType::kPlaceHolder) {
    ReplaceInlineNode(node);
    return;
  }
  PushInlineState(node->GetType());
  if (type == MarkdownDomType::kInlineHtml) {
    EnterInlineHtml(node);
  }
  UpdateInlineBorderLeft(type);
  const auto char_start = context_.GetParagraph()->GetCharCount();
  const char* link_url = nullptr;
  if (type == MarkdownDomType::kRawText) {
    auto* raw_text = static_cast<MarkdownDomRawText*>(node);
    if (auto& content = raw_text->GetText(); !content.empty()) {
      context_.GetParagraph()->AddTextRun(
          &(context_.GetCurrentState().run_style_), content.c_str());
    }
  } else if (type == MarkdownDomType::kBreakLine) {
    context_.GetParagraph()->AddTextRun(
        &(context_.GetCurrentState().run_style_), "\n");
  } else if (type == MarkdownDomType::kImage) {
    ConvertImage(node);
  } else {
    if (type == MarkdownDomType::kBold) {
      context_.GetCurrentState().run_style_.SetBold(true);
    } else if (type == MarkdownDomType::kItalic) {
      context_.GetCurrentState().run_style_.SetItalic(true);
    } else if (type == MarkdownDomType::kBoldItalic) {
      context_.GetCurrentState().run_style_.SetBold(true);
      context_.GetCurrentState().run_style_.SetItalic(true);
    } else if (type == MarkdownDomType::kDelete) {
      auto& style = context_.GetCurrentState().run_style_;
      style.SetDecorationType(tttext::DecorationType::kLineThrough);
      style.SetDecorationStyle(tttext::LineType::kSolid);
      style.SetDecorationColor(style.GetForegroundColor());
      style.SetDecorationThicknessMultiplier(1);
    } else if (type == MarkdownDomType::kLink) {
      link_url = static_cast<MarkdownDomLink*>(node)->GetUrl().c_str();
    }
    ConvertInlineChild(node);
  }
  const auto char_end = context_.GetParagraph()->GetCharCount();
  UpdateDecorationStyle(char_start, char_end);
  UpdateInlineBorderRight(type, context_.char_offset_ + char_start,
                          context_.char_offset_ + char_end);
  if (link_url != nullptr) {
    const auto* para = context_.GetParagraph();
    const auto link_content =
        para->GetContentString(char_start, char_end - char_start);
    document_->AddLink(MarkdownLink{
        .url_ = link_url,
        .content_ = link_content,
        .char_start_ = context_.char_offset_ + char_start,
        .char_count_ = char_end - char_start,
        .attached_paragraph_ = context_.GetParagraph(),
        .char_start_in_paragraph_ = char_start,
    });
  }
  PopState();
}

void MarkdownConverter::UpdateInlineBorderLeft(MarkdownDomType type) {
  auto* block = GetBlockStyleByType(type);
  if (block == nullptr)
    return;
  auto* border = GetBorderStyleByType(type);
  if (border == nullptr)
    return;
  MarkdownParserEmbed::AppendInlineBorderLeft(
      *block, *border, nullptr, context_.GetParagraph(),
      &(context_.GetCurrentState().run_style_));
}

void MarkdownConverter::UpdateInlineBorderRight(MarkdownDomType type,
                                                uint32_t char_start,
                                                uint32_t char_end) {
  auto* block = GetBlockStyleByType(type);
  if (block == nullptr)
    return;
  auto* border = GetBorderStyleByType(type);
  if (border == nullptr)
    return;
  auto* base = GetBaseStyleByType(type);
  if (base == nullptr)
    return;
  MarkdownParserEmbed::AppendInlineBorderRight(
      document_, *base, *block, *border, nullptr, context_.GetParagraph(),
      char_start, char_end);
}

std::vector<std::string_view> Split(std::string_view content, char split) {
  if (content.empty()) {
    return {};
  }
  size_t start = content[0] == split ? 1 : 0;
  std::vector<std::string_view> strs;
  for (size_t end = start; end < content.size(); end++) {
    if (content[end] == split) {
      strs.emplace_back(content.substr(start, end - start));
      start = end + 1;
    }
  }
  if (start < content.size()) {
    strs.emplace_back(content.substr(start, content.size() - start));
  }
  return strs;
}

void MarkdownConverter::EnterInlineHtml(MarkdownDomNode* node) {
  auto* html_node = static_cast<MarkdownDomHtmlNode*>(node);
  auto& tag = html_node->GetTag();
  if (tag != "span") {
    return;
  }
  auto& clazz = html_node->GetClass();
  if (clazz.empty()) {
    return;
  }
  const auto classes = Split(clazz, ' ');
  for (auto& value : classes) {
    const auto style_iter =
        document_->GetStyle().span_styles_.find(std::string(value));
    if (style_iter == document_->GetStyle().span_styles_.end()) {
      continue;
    }
    auto& style = style_iter->second;
    auto& [element, run_style, paragraph_style, node_type] =
        context_.GetCurrentState();
    MarkdownParserEmbed::SetTTStyleByMarkdownBaseStyle(document_, style.base_,
                                                       &run_style);
  }
}

std::unique_ptr<MarkdownElement> MarkdownConverter::GenerateElement(
    MarkdownDomType type) {
  if (type == MarkdownDomType::kParagraph ||
      type == MarkdownDomType::kCodeBlock || type == MarkdownDomType::kHeader) {
    return std::make_unique<MarkdownParagraphElement>();
  }
  if (type == MarkdownDomType::kTable) {
    return std::make_unique<MarkdownTableElement>();
  }
  if (type == MarkdownDomType::kListItem) {
    return std::make_unique<MarkdownListItem>();
  }
  return std::make_unique<MarkdownBlockElement>();
}
void MarkdownConverter::PushInlineState(MarkdownDomType type) {
  auto back = context_.state_stack_.back();
  back.node_type_ = type;
  context_.state_stack_.emplace_back(back);
  UpdateBaseStyle();
}

void MarkdownConverter::PushState(MarkdownElement* element,
                                  MarkdownDomType type) {
  if (context_.state_stack_.empty()) {
    context_.state_stack_.emplace_back(
        MarkdownConverterState{.element_ = element, .node_type_ = type});
  } else {
    auto back = context_.state_stack_.back();
    back.element_ = element;
    back.node_type_ = type;
    context_.state_stack_.emplace_back(back);
  }
  UpdateBlockNodeStyle();
  auto syntax = GetSyntaxType(type);
  element->SetMarkdownSourceType(syntax);
}
void MarkdownConverter::PopState() {
  context_.state_stack_.pop_back();
}
void MarkdownConverter::PushMaxWidth(MarkdownElement* element) {
  auto& block = element->GetBlockStyle();
  if (element->GetType() == MarkdownElementType::kTable) {
    auto& cell_block = document_->GetStyle().table_cell_.block_;
    if (cell_block.max_width_ > 0) {
      context_.max_width_stack_.emplace_back(cell_block.max_width_);
    }
    return;
  }
  if (block.max_width_ > 0) {
    context_.max_width_stack_.emplace_back(block.max_width_);
  } else {
    float width = context_.max_width_stack_.back();
    width -= block.margin_left_ + block.margin_right_ + block.padding_left_ +
             block.padding_right_;
    context_.max_width_stack_.emplace_back(width);
  }
}
void MarkdownConverter::PopMaxWidth() {
  context_.max_width_stack_.pop_back();
}

void MarkdownConverter::UpdateBlockNodeStyle() {
  UpdateBaseStyle();
  UpdateBlockStyle();
}

void MarkdownConverter::UpdateBaseStyle() {
  auto& [element, run_style, paragraph_style, node_type] =
      context_.GetCurrentState();
  if (auto base_style = GetBaseStyleByType(node_type); base_style != nullptr) {
    MarkdownParserEmbed::SetTTStyleByMarkdownBaseStyle(document_, *base_style,
                                                       &run_style);
    paragraph_style.SetDefaultStyle(run_style);
    MarkdownParserEmbed::SetParagraphStyle(document_, *base_style,
                                           &paragraph_style, element,
                                           tttext::RulerType::kExact);
  }
}

void MarkdownConverter::UpdateBlockStyle() {
  auto& [element, run_style, paragraph_style, node_type] =
      context_.GetCurrentState();
  if (auto* block_style = GetBlockStyleByType(node_type);
      block_style != nullptr) {
    element->SetBlockStyle(*block_style);
  }
  if (auto* border_style = GetBorderStyleByType(node_type);
      border_style != nullptr) {
    element->SetBorderStyle(*border_style);
    auto border_type = GetBorderTypeByType(node_type);
    element->SetBorderType(border_type);
  }
  if (auto* scroll_style = GetScrollStyleByType(node_type);
      scroll_style != nullptr) {
    element->SetScrollX(scroll_style->scroll_x_);
  }
}

void MarkdownConverter::UpdateDecorationStyle(uint32_t char_start,
                                              uint32_t char_end) {
  auto& [element, run_style, paragraph_style, node_type] =
      context_.GetCurrentState();
  if (auto* decoration_style = GetDecorationStyle(node_type);
      decoration_style != nullptr) {
    auto new_style = run_style;
    MarkdownParserEmbed::SetDecorationStyle(*decoration_style, &new_style);
    context_.GetParagraph()->ApplyStyleInRange(new_style, char_start,
                                               char_end - char_start);
  }
}

void MarkdownConverter::UpdateListItemMarker(MarkdownDomNode* node) {
  auto* parent = static_cast<MarkdownDomNode*>(node->GetParent());
  if (parent == nullptr) {
    return;
  }
  auto type = parent->GetType();
  auto& style = document_->GetStyle();
  if (type == MarkdownDomType::kOrderedList) {
    MarkdownNumberType number_type =
        style.ordered_list_.ordered_list_.number_type_;
    if (number_type == MarkdownNumberType::kMixed) {
      number_type = static_cast<MarkdownNumberType>(
          (context_.list_level_ - 1) %
          (static_cast<int>(MarkdownNumberType::kMixed)));
    }
    auto number_str = MarkdownParserEmbed::MarkdownNumberTypeToString(
                          number_type, context_.list_index_stack_.back()) +
                      ".";
    auto para = tttext::Paragraph::Create();
    auto new_style = context_.GetCurrentState().run_style_;
    MarkdownParserEmbed::SetTTStyleByMarkdownBaseStyle(
        document_, style.ordered_list_number_.base_, &new_style);
    para->SetParagraphStyle(&(context_.GetCurrentState().paragraph_style_));
    para->AddTextRun(&new_style, number_str.c_str(), number_str.length());
    auto delegate = std::make_shared<MarkdownTextDelegate>(
        std::move(para), style.ordered_list_number_.block_,
        document_->GetMaxWidth(), document_->GetMaxHeight());
    delegate->Layout();
    context_.GetListItem()->SetMarker(delegate);
    context_.marker_ = delegate;
  } else if (type == MarkdownDomType::kUnorderedList) {
    MarkdownMarkType mark_type =
        style.unordered_list_marker_.marker_.mark_type_;
    if (mark_type == MarkdownMarkType::kMixed) {
      mark_type = static_cast<MarkdownMarkType>(
          (context_.list_level_ - 1) %
          (static_cast<int>(MarkdownMarkType::kMixed)));
    }
    auto mark = std::make_shared<MarkdownUnorderedListMarkDelegate>(
        mark_type, style.unordered_list_marker_);
    context_.GetListItem()->SetMarker(mark);
    context_.marker_ = mark;
  }
  if (context_.marker_ != nullptr) {
    float width = context_.marker_->GetAdvance();
    context_.max_width_stack_.back() -= width;
  }
}

const MarkdownDecorationStylePart* MarkdownConverter::GetDecorationStyle(
    MarkdownDomType type) {
  if (type == MarkdownDomType::kLink) {
    return &(document_->GetStyle().link_.decoration_);
  }
  return nullptr;
}

const MarkdownBaseStylePart* MarkdownConverter::GetBaseStyleByType(
    MarkdownDomType type) {
  auto& style = document_->GetStyle();
  if (type == MarkdownDomType::kSource || type == MarkdownDomType::kUndefined) {
    return &style.normal_text_.base_;
  }
  if (type == MarkdownDomType::kQuote) {
    return &style.quote_.base_;
  }
  if (type == MarkdownDomType::kOrderedList) {
    return &(style.ordered_list_.base_);
  }
  if (type == MarkdownDomType::kUnorderedList) {
    return &(style.unordered_list_.base_);
  }
  if (type == MarkdownDomType::kCodeBlock) {
    return &(style.code_block_.base_);
  }
  if (type == MarkdownDomType::kHeader) {
    return &(MarkdownParserEmbed::GetHNStyle(style, context_.hn_));
  }
  if (type == MarkdownDomType::kInlineCode) {
    return &(style.inline_code_.base_);
  }
  if (type == MarkdownDomType::kLink) {
    return &(style.link_.base_);
  }
  if (type == MarkdownDomType::kTableCell) {
    if (context_.row_index_ == 0) {
      return &(style.table_header_.base_);
    } else {
      return &(style.table_cell_.base_);
    }
  }
  return nullptr;
}
const MarkdownBlockStylePart* MarkdownConverter::GetBlockStyleByType(
    MarkdownDomType type) {
  auto& style = document_->GetStyle();
  if (type == MarkdownDomType::kSource) {
    return &style.normal_text_.block_;
  }
  if (type == MarkdownDomType::kQuote) {
    return &style.quote_.block_;
  }
  if (type == MarkdownDomType::kOrderedList) {
    return &(style.ordered_list_.block_);
  }
  if (type == MarkdownDomType::kUnorderedList) {
    return &(style.unordered_list_.block_);
  }
  if (type == MarkdownDomType::kCodeBlock) {
    return &(style.code_block_.block_);
  }
  if (type == MarkdownDomType::kHeader) {
    return &(MarkdownParserEmbed::GetHNBlockStyle(style, context_.hn_));
  }
  if (type == MarkdownDomType::kInlineCode) {
    return &(style.inline_code_.block_);
  }
  if (type == MarkdownDomType::kTable) {
    return &(style.table_.block_);
  }
  if (type == MarkdownDomType::kTableCell) {
    return &(style.table_cell_.block_);
  }
  if (type == MarkdownDomType::kSplit) {
    return &(style.split_.block_);
  }
  return nullptr;
}
const MarkdownBorderStylePart* MarkdownConverter::GetBorderStyleByType(
    MarkdownDomType type) {
  auto& style = document_->GetStyle();
  if (type == MarkdownDomType::kQuote) {
    return &style.quote_.border_;
  }
  if (type == MarkdownDomType::kCodeBlock) {
    return &(style.code_block_.border_);
  }
  if (type == MarkdownDomType::kInlineCode) {
    return &(style.inline_code_.border_);
  }
  if (type == MarkdownDomType::kTable) {
    return &(style.table_.border_);
  }
  if (type == MarkdownDomType::kSplit) {
    return &(style.split_.border_);
  }
  return nullptr;
}
const MarkdownBackgroundStylePart* MarkdownConverter::GetBackgroundStyleByType(
    MarkdownDomType type) {
  return nullptr;
}
const MarkdownScrollStylePart* MarkdownConverter::GetScrollStyleByType(
    MarkdownDomType type) {
  auto& style = document_->GetStyle();
  if (type == MarkdownDomType::kTable) {
    return &(style.table_.scroll_);
  }
  if (type == MarkdownDomType::kCodeBlock) {
    return &(style.code_block_.scroll_);
  }
  return nullptr;
}

MarkdownBorder MarkdownConverter::GetBorderTypeByType(MarkdownDomType type) {
  if (type == MarkdownDomType::kQuote) {
    return MarkdownBorder::kLeft;
  }
  if (type == MarkdownDomType::kCodeBlock) {
    return MarkdownBorder::kRect;
  }
  if (type == MarkdownDomType::kInlineCode) {
    return MarkdownBorder::kRect;
  }
  if (type == MarkdownDomType::kTable) {
    return MarkdownBorder::kRect;
  }
  if (type == MarkdownDomType::kSplit) {
    return MarkdownBorder::kTop;
  }
  return MarkdownBorder::kNone;
}

MarkdownSyntaxType MarkdownConverter::GetSyntaxType(MarkdownDomType type) {
  if (type == MarkdownDomType::kTable) {
    return MarkdownSyntaxType::kTable;
  }
  switch (type) {
    case MarkdownDomType::kParagraph:
    case MarkdownDomType::kHeader:
      return MarkdownSyntaxType::kParagraph;
    case MarkdownDomType::kOrderedList:
      return MarkdownSyntaxType::kOrderedList;
    case MarkdownDomType::kUnorderedList:
      return MarkdownSyntaxType::kUnorderedList;
    case MarkdownDomType::kCodeBlock:
      return MarkdownSyntaxType::kCodeBlock;
    case MarkdownDomType::kQuote:
      return MarkdownSyntaxType::kQuote;
    case MarkdownDomType::kSplit:
      return MarkdownSyntaxType::kSplit;
    default:
      return MarkdownSyntaxType::kUndefined;
  }
}

void MarkdownConverter::MakeElementPlain(
    std::unique_ptr<MarkdownElement> element, MarkdownDocument* target,
    float left, float top, float right, float bottom) {
  auto block_style = element->GetBlockStyle();
  block_style.margin_left_ += left;
  block_style.margin_right_ += right;
  block_style.margin_top_ += top;
  block_style.margin_bottom_ += bottom;
  float total_left = block_style.margin_left_ + block_style.padding_left_;
  float total_right = block_style.margin_right_ + block_style.padding_right_;
  float total_top = block_style.margin_top_ + block_style.padding_top_;
  float total_bottom = block_style.margin_bottom_ + block_style.padding_bottom_;
  if (element->GetType() == MarkdownElementType::kParagraph) {
    auto* para =
        static_cast<MarkdownParagraphElement*>(element.get())->GetParagraph();
    if (para->GetCharCount() > 0) {
      element->SetBlockStyle(block_style);
      target->AddParagraph(std::move(element));
    }
  } else if (element->GetType() == MarkdownElementType::kTable ||
             (element->GetMarkdownSourceType() == MarkdownSyntaxType::kSplit &&
              element->GetBorderType() == MarkdownBorder::kTop)) {
    element->SetBlockStyle(block_style);
    target->AddParagraph(std::move(element));
  } else if (element->GetType() == MarkdownElementType::kBlock ||
             element->GetType() == MarkdownElementType::kListItem) {
    float none_first_space = 0;
    uint32_t para_start = 0;
    if (element->GetType() == MarkdownElementType::kListItem) {
      auto* list_item = static_cast<MarkdownListItem*>(element.get());
      auto marker = list_item->GetMarker();
      marker->Layout();
      auto width = marker->GetAdvance();
      none_first_space = width;
    }
    if (element->GetMarkdownSourceType() == MarkdownSyntaxType::kQuote &&
        element->GetBorderType() == MarkdownBorder::kLeft) {
      para_start = target->GetParagraphs().size();
    }
    auto* block = static_cast<MarkdownBlockElement*>(element.get());
    for (auto& child : block->GetChildren()) {
      if (child.get() == block->GetChildren().front().get()) {
        MakeElementPlain(std::move(child), target, total_left, total_top,
                         total_right, 0);
      } else if (child.get() == block->GetChildren().back().get()) {
        MakeElementPlain(std::move(child), target,
                         total_left + none_first_space, 0, total_right,
                         total_bottom);
      } else {
        MakeElementPlain(std::move(child), target,
                         total_left + none_first_space, 0, total_right, 0);
      }
    }
    if (element->GetMarkdownSourceType() == MarkdownSyntaxType::kQuote &&
        element->GetBorderType() == MarkdownBorder::kLeft) {
      uint32_t para_end = target->GetParagraphs().size();
      target->AddQuoteRange(Range{static_cast<int32_t>(para_start),
                                  static_cast<int32_t>(para_end)});
    }
  }
}

void MarkdownParserImpl::ConvertDomTree(MarkdownDocument* document,
                                        MarkdownDomNode* root) {
  MarkdownConverter converter;
  converter.Convert(document, root);
}

}  // namespace lynx::markdown
