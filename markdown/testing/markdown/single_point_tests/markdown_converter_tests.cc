// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/element/markdown_table.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/parser/markdown_dom_node.h"
#include "markdown/style/markdown_style_initializer.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"

namespace serval::markdown::testing {
namespace {

class TestMarkdownParserImpl final : public MarkdownParserImpl {
 public:
  using MarkdownParserImpl::ConvertDomTree;
};

class RecordingResourceLoader final : public MockMarkdownResourceLoader {
 public:
  MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                              float max_width,
                                              float max_height) override {
    replacement_data_.emplace_back(ud);
    replacement_ids_.emplace_back(id);
    replacement_max_widths_.emplace_back(max_width);
    return {
        .view_ = std::make_shared<MockImage>("replacement", 10, 10, max_width,
                                             max_height, 0),
        .alt_text_ = "replacement",
    };
  }

  std::vector<void*> replacement_data_;
  std::vector<int32_t> replacement_ids_;
  std::vector<float> replacement_max_widths_;
};

class RecordingImageResourceLoader final : public MockMarkdownResourceLoader {
 public:
  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float radius) override {
    desire_width_ = desire_width;
    desire_height_ = desire_height;
    return MockMarkdownResourceLoader::LoadImage(
        src, desire_width, desire_height, max_width, max_height, radius);
  }

  float desire_width_{0};
  float desire_height_{0};
};

class RecordingMeasureDrawable final : public MarkdownDrawable {
 public:
  explicit RecordingMeasureDrawable(SizeF size) : size_(size) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {}

  MeasureSpec last_spec_{};

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    last_spec_ = spec;
    return {.width_ = size_.width_,
            .height_ = size_.height_,
            .baseline_ = size_.height_};
  }

 private:
  SizeF size_;
};

MarkdownStyle MakeMarkdownStyle() {
  MarkdownStyle style{};
  MarkdownStyleInitializer::InitialNormalTextStyle(&style);
  MarkdownStyleInitializer::InitialOtherStyleByNormalTextStyle(&style);
  MarkdownStyleInitializer::InitialBoldStyle(&style);
  MarkdownStyleInitializer::InitialItalicStyle(&style);
  return style;
}

std::unique_ptr<MarkdownDocument> MakeDocument(MarkdownResourceLoader* loader) {
  auto document = std::make_unique<MarkdownDocument>(
      CreateTestMarkdownSharedContext(), loader);
  document->SetMaxSize(400, 400);
  document->SetStyle(MakeMarkdownStyle());
  return document;
}

void AppendText(MarkdownDomNode* parent, MarkdownDomRawText* text,
                const char* content) {
  text->SetText(content);
  parent->AppendChild(text);
}

TEST(MarkdownReplacementViewWrapperTest, UsesFontAdjustedBaseline) {
  auto view = std::make_shared<RecordingMeasureDrawable>(SizeF{10, 10});
  MarkdownReplacementViewWrapper wrapper(view, 120, 80, 20);

  const auto result = wrapper.Measure(MeasureSpec{});

  EXPECT_FLOAT_EQ(result.width_, 10);
  EXPECT_FLOAT_EQ(result.height_, 10);
  EXPECT_FLOAT_EQ(result.baseline_, (10 + 0.6f * 20) / 2);
  EXPECT_FLOAT_EQ(view->last_spec_.width_, 120);
  EXPECT_FLOAT_EQ(view->last_spec_.height_, 80);
}

TEST(MarkdownConverterTest, UsesPerCellHeaderAndAlignmentMetadata) {
  MockMarkdownResourceLoader loader;
  auto document = MakeDocument(&loader);
  auto style = document->GetStyle();
  style.table_header_.align_.vertical_align_ = MarkdownVerticalAlign::kTop;
  style.table_cell_.align_.vertical_align_ = MarkdownVerticalAlign::kBottom;
  style.table_header_.base_.font_size_ = 31;
  style.table_cell_.base_.font_size_ = 17;
  document->SetStyle(style);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomTable table;
  table.SetAligns({MarkdownTextAlign::kLeft});
  MarkdownDomNode first_row(MarkdownDomType::kTableRow);
  MarkdownDomTableCell first_cell;
  MarkdownDomRawText first_text(MarkdownDomType::kRawText);
  first_cell.SetHeader(false);
  first_cell.SetAlign(MarkdownTextAlign::kRight);
  AppendText(&first_cell, &first_text, "body");
  first_row.AppendChild(&first_cell);

  MarkdownDomNode second_row(MarkdownDomType::kTableRow);
  MarkdownDomTableCell second_cell;
  MarkdownDomRawText second_text(MarkdownDomType::kRawText);
  second_cell.SetHeader(true);
  second_cell.SetAlign(MarkdownTextAlign::kCenter);
  AppendText(&second_cell, &second_text, "header");
  second_row.AppendChild(&second_cell);

  table.AppendChild(&first_row);
  table.AppendChild(&second_row);
  root.AppendChild(&table);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  const auto& elements = document->GetParagraphs();
  ASSERT_EQ(elements.size(), 1u);
  ASSERT_EQ(elements[0]->GetType(), MarkdownElementType::kTable);
  const auto* converted_table =
      static_cast<MarkdownTableElement*>(elements[0].get())->GetTable();
  ASSERT_NE(converted_table, nullptr);
  ASSERT_EQ(converted_table->GetRowCount(), 2);
  ASSERT_EQ(converted_table->GetColumnCount(), 1);

  const auto& body_cell = converted_table->GetCell(0, 0);
  EXPECT_EQ(body_cell.alignment_, tttext::ParagraphHorizontalAlignment::kRight);
  EXPECT_EQ(body_cell.vertical_alignment_, MarkdownVerticalAlign::kBottom);
  EXPECT_FLOAT_EQ(
      body_cell.paragraph_->GetParagraphStyle().GetDefaultStyle().GetTextSize(),
      17);

  const auto& header_cell = converted_table->GetCell(1, 0);
  EXPECT_EQ(header_cell.alignment_,
            tttext::ParagraphHorizontalAlignment::kCenter);
  EXPECT_EQ(header_cell.vertical_alignment_, MarkdownVerticalAlign::kTop);
  EXPECT_FLOAT_EQ(header_cell.paragraph_->GetParagraphStyle()
                      .GetDefaultStyle()
                      .GetTextSize(),
                  31);
}

TEST(MarkdownConverterTest, AppliesListBlockStyleToEveryListItem) {
  MockMarkdownResourceLoader loader;
  auto document = MakeDocument(&loader);
  auto style = document->GetStyle();
  style.unordered_list_.block_.margin_top_ = 13;
  style.unordered_list_.base_.font_size_ = 23;
  style.unordered_list_.base_.paragraph_space_ = 29;
  document->SetStyle(style);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomList list(MarkdownDomType::kUnorderedList);
  MarkdownDomListItem first_item;
  MarkdownDomNode first_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText first_text(MarkdownDomType::kRawText);
  AppendText(&first_paragraph, &first_text, "first");
  first_item.AppendChild(&first_paragraph);

  MarkdownDomListItem second_item;
  MarkdownDomNode second_container(MarkdownDomType::kUndefined);
  MarkdownDomNode second_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText second_text(MarkdownDomType::kRawText);
  AppendText(&second_paragraph, &second_text, "second");
  second_container.AppendChild(&second_paragraph);
  second_item.AppendChild(&second_container);

  list.AppendChild(&first_item);
  list.AppendChild(&second_item);
  root.AppendChild(&list);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  const auto& elements = document->GetParagraphs();
  ASSERT_EQ(elements.size(), 2u);
  EXPECT_FLOAT_EQ(elements[0]->GetBlockStyle().margin_top_, 13);
  EXPECT_FLOAT_EQ(elements[1]->GetBlockStyle().margin_top_, 13);
  EXPECT_FLOAT_EQ(elements[0]->GetSpaceAfter(), 29);
  EXPECT_FLOAT_EQ(elements[1]->GetSpaceAfter(), 29);
  auto* second_paragraph_element =
      static_cast<MarkdownParagraphElement*>(elements[1].get());
  EXPECT_FLOAT_EQ(second_paragraph_element->GetParagraph()
                      ->GetParagraphStyle()
                      .GetDefaultStyle()
                      .GetTextSize(),
                  23);
}

TEST(MarkdownConverterTest, AppliesParagraphSpaceToParagraphsAndSplit) {
  MockMarkdownResourceLoader loader;
  auto document = MakeDocument(&loader);
  auto style = document->GetStyle();
  style.normal_text_.base_.paragraph_space_ = 17;
  document->SetStyle(style);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomNode first_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText first_text(MarkdownDomType::kRawText);
  AppendText(&first_paragraph, &first_text, "first");
  MarkdownDomNode second_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText second_text(MarkdownDomType::kRawText);
  AppendText(&second_paragraph, &second_text, "second");
  MarkdownDomNode split(MarkdownDomType::kSplit);
  root.AppendChild(&first_paragraph);
  root.AppendChild(&second_paragraph);
  root.AppendChild(&split);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  const auto& elements = document->GetParagraphs();
  ASSERT_EQ(elements.size(), 3u);
  EXPECT_FLOAT_EQ(elements[0]->GetSpaceAfter(), 17);
  EXPECT_FLOAT_EQ(elements[1]->GetSpaceAfter(), 17);
  EXPECT_FLOAT_EQ(elements[2]->GetSpaceAfter(), 17);
}

TEST(MarkdownConverterTest, ClearsLastQuoteParagraphSpace) {
  MockMarkdownResourceLoader loader;
  auto document = MakeDocument(&loader);
  auto style = document->GetStyle();
  style.quote_.base_.paragraph_space_ = 23;
  document->SetStyle(style);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomNode quote(MarkdownDomType::kQuote);
  MarkdownDomNode first_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText first_text(MarkdownDomType::kRawText);
  AppendText(&first_paragraph, &first_text, "first");
  MarkdownDomNode second_paragraph(MarkdownDomType::kParagraph);
  MarkdownDomRawText second_text(MarkdownDomType::kRawText);
  AppendText(&second_paragraph, &second_text, "second");
  quote.AppendChild(&first_paragraph);
  quote.AppendChild(&second_paragraph);
  root.AppendChild(&quote);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  const auto& elements = document->GetParagraphs();
  ASSERT_EQ(elements.size(), 2u);
  EXPECT_FLOAT_EQ(elements[0]->GetSpaceAfter(), 23);
  EXPECT_FLOAT_EQ(elements[1]->GetSpaceAfter(), 0);
}

TEST(MarkdownConverterTest, UsesUndefinedImageSizeAndFallsBackToAltText) {
  RecordingImageResourceLoader loader;
  auto document = MakeDocument(&loader);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomNode paragraph(MarkdownDomType::kParagraph);
  MarkdownDomImage image;
  image.SetUrl("invalid");
  image.SetAltText("fallback");
  paragraph.AppendChild(&image);
  root.AppendChild(&paragraph);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  EXPECT_FLOAT_EQ(loader.desire_width_, -1);
  EXPECT_FLOAT_EQ(loader.desire_height_, -1);
  const auto& elements = document->GetParagraphs();
  ASSERT_EQ(elements.size(), 1u);
  auto* paragraph_element =
      static_cast<MarkdownParagraphElement*>(elements[0].get());
  ASSERT_NE(paragraph_element->GetParagraph(), nullptr);
  EXPECT_EQ(paragraph_element->GetParagraph()->GetContentString(0, 8),
            "fallback");
}

TEST(MarkdownConverterTest, UsesStableConverterNodeIdForReplacement) {
  RecordingResourceLoader loader;
  auto document = MakeDocument(&loader);
  int block_source = 0;
  int inline_source = 0;

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomPlaceHolder block_placeholder;
  block_placeholder.SetData(&block_source);

  MarkdownDomNode paragraph(MarkdownDomType::kParagraph);
  MarkdownDomPlaceHolder inline_placeholder;
  inline_placeholder.SetData(&inline_source);
  paragraph.AppendChild(&inline_placeholder);

  root.AppendChild(&block_placeholder);
  root.AppendChild(&paragraph);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  ASSERT_EQ(loader.replacement_data_.size(), 2u);
  EXPECT_EQ(loader.replacement_data_[0], &block_source);
  EXPECT_EQ(loader.replacement_data_[1], &inline_source);
  EXPECT_EQ(loader.replacement_ids_, (std::vector<int32_t>{2, 4}));
  const auto first_conversion_ids = loader.replacement_ids_;

  loader.replacement_data_.clear();
  loader.replacement_ids_.clear();
  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  ASSERT_EQ(loader.replacement_data_.size(), 2u);
  EXPECT_EQ(loader.replacement_data_[0], &block_source);
  EXPECT_EQ(loader.replacement_data_[1], &inline_source);
  EXPECT_EQ(loader.replacement_ids_, first_conversion_ids);

  const auto& inline_views = document->GetInlineViews();
  ASSERT_EQ(inline_views.size(), 2u);
  EXPECT_EQ(inline_views[0].id_, "2");
  EXPECT_EQ(inline_views[1].id_, "4");
}

TEST(MarkdownConverterTest, TableKeepsReplacementMaxWidthStackBalanced) {
  RecordingResourceLoader loader;
  auto document = MakeDocument(&loader);
  auto style = document->GetStyle();
  style.normal_text_.block_.margin_left_ = 10;
  style.normal_text_.block_.margin_right_ = 20;
  style.normal_text_.block_.padding_left_ = 5;
  style.normal_text_.block_.padding_right_ = 5;
  style.normal_text_.block_.max_width_ = 500;
  style.table_.block_.max_width_ = 300;
  document->SetStyle(style);

  MarkdownDomNode root(MarkdownDomType::kSource);
  MarkdownDomTable table;
  table.SetAligns({MarkdownTextAlign::kLeft});
  MarkdownDomNode row(MarkdownDomType::kTableRow);
  MarkdownDomTableCell cell;
  MarkdownDomPlaceHolder cell_replacement;
  cell.AppendChild(&cell_replacement);
  row.AppendChild(&cell);
  table.AppendChild(&row);

  MarkdownDomPlaceHolder replacement;
  root.AppendChild(&table);
  root.AppendChild(&replacement);

  TestMarkdownParserImpl::ConvertDomTree(document.get(), &root);

  ASSERT_EQ(loader.replacement_max_widths_.size(), 2u);
  EXPECT_FLOAT_EQ(loader.replacement_max_widths_[0], 300);
  EXPECT_FLOAT_EQ(loader.replacement_max_widths_[1], 360);
}

}  // namespace
}  // namespace serval::markdown::testing
