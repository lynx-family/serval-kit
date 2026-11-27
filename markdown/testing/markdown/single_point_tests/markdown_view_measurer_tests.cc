// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "../mock_platform/markdown_tests_platform.h"
#include "../mock_platform/mock_markdown_canvas.h"
#include "../mock_platform/mock_markdown_resource_loader.h"
#include "markdown/draw/markdown_drawer.h"
#include "markdown/element/markdown_paragraph.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/utils/markdown_string_utils.h"
#include "markdown/view/markdown_view_measurer.h"
#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"

namespace serval::markdown {

TEST(MarkdownViewMeasurerTest, HtmlCompletionCanBeToggledOnExistingContent) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetContent("word <span>tail");
  for (bool enabled : {false, true, false}) {
    measurer.SetEnableSyntaxCompletion(enabled);
    measurer.Measure({});
    EXPECT_EQ(measurer.GetDocument()->GetContentByCharPos(0, 100),
              enabled ? "word tail\n" : "word <span>tail\n");
  }
}

TEST(MarkdownViewMeasurerTest, DashedDecorationScalesWithDensity) {
  const float original = MarkdownScreenMetrics::GetDensity();
  for (float density : {1.f, 3.3125f}) {
    MarkdownScreenMetrics::SetDensity(density);
    auto context = testing::CreateTestMarkdownSharedContext();
    testing::MockMarkdownResourceLoader loader;
    MarkdownViewMeasurer measurer(context, &loader);
    auto config = MarkdownStyleReader::ReadStyle({}, &loader, context.get());
    config.link_.decoration_.text_decoration_style_ =
        MarkdownTextDecorationStyle::kDashed;
    config.link_.decoration_.text_decoration_line_ =
        MarkdownTextDecorationLine::kUnderline;
    measurer.SetStyle(config);
    measurer.SetContent("[dashed underline](url)");
    measurer.Measure({});
    testing::MockMarkdownCanvas canvas(&loader);
    MarkdownDrawer drawer(&canvas, context.get());
    canvas.StartPaint();
    drawer.DrawPage(*measurer.GetDocument()->GetPage());
    canvas.EndPaint();
    int count = 0;
    float painted_end = 0;
    for (const auto& op : canvas.GetJson()[0].GetArray()) {
      if (std::string_view(op["op"].GetString()) != "line")
        continue;
      float start = op["p1"]["x"].GetFloat();
      float end = op["p2"]["x"].GetFloat();
      float cap = op["painter"]["stroke_width"].GetFloat() / 2;
      EXPECT_NEAR(end - start, 2.5f * density, 0.001f);
      if (count > 0)
        EXPECT_GT(start - cap, painted_end);
      painted_end = end + cap;
      ++count;
    }
    EXPECT_GT(count, 1);
  }
  MarkdownScreenMetrics::SetDensity(original);
}

TEST(MarkdownViewMeasurerTest, PendingAndReadyFormulaSourceMapping) {
  class Loader : public testing::MockMarkdownResourceLoader {
   public:
    std::shared_ptr<MarkdownDrawable> LoadFormula(std::string_view formula,
                                                  float font_size,
                                                  uint32_t color, float width,
                                                  bool display) override {
      EXPECT_EQ(formula, "\nx_1\n");
      EXPECT_TRUE(display);
      EXPECT_GT(font_size, 0);
      EXPECT_GT(width, 0);
      return ready ? std::make_shared<testing::MockImage>("math", 40, 20, width,
                                                          -1, 0)
                   : nullptr;
    }
    bool ready{false};
  } loader;
  testing::MockMarkdownResourceLoader default_loader;
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext(),
                                &loader);
  measurer.SetContent("> $$\n> x_1\n> $$Z");
  measurer.Measure({});
  EXPECT_EQ(measurer.GetDocument()->GetContentByCharPos(0, 100),
            "$$\nx_1\n$$Z\n");
  loader.ready = true;
  measurer.NeedsMeasure();
  measurer.Measure({});
  auto document = measurer.GetDocument();
  EXPECT_EQ(document->GetContentByCharPos(0, 1), "$$\nx_1\n$$");
  EXPECT_EQ(document->CharOffsetToMarkdownOffset(0), 2);
  EXPECT_EQ(document->CharOffsetToMarkdownOffset(1), 15);

  measurer.SetResourceLoader(&default_loader);
  measurer.Measure({});
  EXPECT_EQ(measurer.GetDocument()->GetContentByCharPos(0, 1), "$");
  EXPECT_EQ(measurer.GetDocument()->GetContentByCharPos(0, 100),
            "$$\nx_1\n$$Z\n");
}

TEST(MarkdownViewMeasurerTest, MeasurePlainTextBasic) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetSourceType(SourceType::kPlainText);
  measurer.SetContent("hello\n");
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;
  auto size = measurer.Measure(spec);
  EXPECT_GT(size.width_, 0);
  EXPECT_GT(size.height_, 0);
  EXPECT_LE(size.width_, 200);
}

TEST(MarkdownViewMeasurerTest, SettersPreservePreviouslyMeasuredDocument) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetContent("original content");
  measurer.SetContentID("original-id");
  measurer.SetContentRange({0, 8});
  MeasureSpec spec{.width_ = 200, .width_mode_ = tttext::LayoutMode::kDefinite};
  measurer.Measure(spec);
  const auto original_document = measurer.GetDocument();
  ASSERT_NE(original_document, nullptr);

  measurer.SetContentID("updated-id");

  EXPECT_EQ(original_document->GetContentID(), "original-id");
  measurer.Measure(spec);
  const auto updated_id_document = measurer.GetDocument();
  EXPECT_NE(updated_id_document, original_document);
  EXPECT_EQ(updated_id_document->GetContentID(), "updated-id");
  EXPECT_EQ(original_document->GetContentID(), "original-id");

  measurer.SetContent("updated content");
  measurer.SetContentRange({1, 7});

  EXPECT_EQ(updated_id_document->GetMarkdownContent(), "original content");
  EXPECT_EQ(updated_id_document->GetMarkdownContentRange().start_, 0);
  EXPECT_EQ(updated_id_document->GetMarkdownContentRange().end_, 8);

  measurer.Measure(spec);
  const auto updated_document = measurer.GetDocument();
  EXPECT_NE(updated_document, updated_id_document);
  EXPECT_EQ(updated_document->GetMarkdownContent(), "updated content");
  EXPECT_EQ(updated_document->GetContentID(), "updated-id");
  EXPECT_EQ(updated_document->GetMarkdownContentRange().start_, 1);
  EXPECT_EQ(updated_document->GetMarkdownContentRange().end_, 7);
  EXPECT_EQ(updated_id_document->GetMarkdownContent(), "original content");
  EXPECT_EQ(updated_id_document->GetMarkdownContentRange().start_, 0);
  EXPECT_EQ(updated_id_document->GetMarkdownContentRange().end_, 8);
}

TEST(MarkdownViewMeasurerTest, ContentRangeAffectsHeight) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetSourceType(SourceType::kMarkdown);
  const std::string content = "line1\n\nline2";
  measurer.SetContent(content);
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;
  auto full = measurer.Measure(spec);

  measurer.SetContentRange({0, 5});
  auto partial = measurer.Measure(spec);

  EXPECT_GT(full.height_, 0);
  EXPECT_GT(partial.height_, 0);
  EXPECT_LE(partial.height_, full.height_);
}

TEST(MarkdownViewMeasurerTest, AlignCustomTruncationViewAtLastLineEnd) {
  auto context = testing::CreateTestMarkdownSharedContext();
  testing::MockMarkdownMainView main_view(context);
  testing::MockMarkdownResourceLoader loader;
  loader.SetMainView(&main_view);
  MarkdownViewMeasurer measurer(context, &loader);
  auto style = MarkdownStyleReader::ReadStyle({}, &loader, context.get());
  style.truncation_.truncation_.truncation_type_ =
      MarkdownTruncationType::kView;
  style.truncation_.truncation_.content_ = "truncation";
  measurer.SetStyle(style);
  measurer.SetContent("first paragraph\n\nsecond paragraph\n\nthird paragraph");
  measurer.SetTextMaxLines(2);
  MeasureSpec spec;
  spec.width_ = 300;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;
  measurer.Measure(spec);

  const auto document = measurer.GetDocument();
  ASSERT_NE(document, nullptr);
  const auto truncation = std::static_pointer_cast<testing::MockInlineView>(
      document->GetTruncationDelegate());
  ASSERT_NE(truncation, nullptr);
  const auto expected = document->GetInlineViewOrigin("truncation");
  ASSERT_GT(expected.first, 0);
  ASSERT_GT(expected.second, 0);

  measurer.Align();

  EXPECT_FLOAT_EQ(truncation->GetAlignedPosition().x_, expected.first);
  EXPECT_FLOAT_EQ(truncation->GetAlignedPosition().y_, expected.second);
}

TEST(MarkdownViewMeasurerTest, ReportsLineEndIndicesAndContents) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetContent("**hello**\n\nworld");
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kAtMost;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;

  measurer.Measure(spec);
  auto document = measurer.GetDocument();
  auto line_ends = document->GetLineEndCharIndices();
  auto line_texts = document->GetLineTexts();

  ASSERT_EQ(line_ends.size(), 2u);
  ASSERT_EQ(line_texts.size(), 2u);
  EXPECT_EQ(line_texts[0], "hello");
  EXPECT_EQ(line_texts[1], "world");
  EXPECT_NE(document->GetContentByCharPos(0, line_ends[0]).find("hello"),
            std::string::npos);
  EXPECT_NE(
      document->GetContentByCharPos(line_ends[0], line_ends[1]).find("world"),
      std::string::npos);
}

TEST(MarkdownViewMeasurerTest, LineEndIndicesRespectMaxLines) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetContent("first\n\nsecond");
  measurer.SetTextMaxLines(1);
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kAtMost;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;

  measurer.Measure(spec);
  auto document = measurer.GetDocument();
  auto line_ends = document->GetLineEndCharIndices();
  auto line_texts = document->GetLineTexts();

  ASSERT_EQ(line_ends.size(), 1u);
  ASSERT_EQ(line_texts.size(), 1u);
  EXPECT_EQ(line_texts[0], "first");
  EXPECT_NE(document->GetContentByCharPos(0, line_ends[0]).find("first"),
            std::string::npos);
}

TEST(MarkdownViewMeasurerTest, CharHitPastWrappedLineEndStaysOnCurrentLine) {
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetSourceType(SourceType::kPlainText);
  measurer.SetContent("abcdefghij");
  MeasureSpec spec;
  spec.width_ = 50;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;

  measurer.Measure(spec);
  auto page = measurer.GetDocument()->GetPage();
  ASSERT_NE(page, nullptr);
  auto* page_region =
      static_cast<MarkdownPageParagraphRegion*>(page->GetRegion(0));
  ASSERT_NE(page_region, nullptr);
  ASSERT_NE(page_region->region_, nullptr);
  ASSERT_GT(page_region->region_->GetLineCount(), 1);
  auto* first_line = page_region->region_->GetLine(0);
  ASSERT_NE(first_line, nullptr);
  ASSERT_GT(first_line->GetCharCount(), 0u);

  PointF point{
      page_region->rect_.GetLeft() + first_line->GetLineRight() + 1.f,
      page_region->rect_.GetTop() +
          (first_line->GetLineTop() + first_line->GetLineBottom()) / 2.f};
  auto range = MarkdownSelection::GetCharRangeByPoint(
      page.get(), point, MarkdownSelection::CharRangeType::kChar);
  auto line_end = static_cast<int32_t>(page_region->element_->GetCharStart() +
                                       first_line->GetEndCharPos());

  EXPECT_EQ(range.start_, line_end - 1);
  EXPECT_EQ(range.end_, line_end);
}

TEST(MarkdownViewMeasurerTest, CodeBlockWithBlankLine) {
  auto entry = testing::MarkdownCaseBuilder::LoadSingleCase(
      "markdown/testing/markdown/cases/code_block_with_blank_line");
  ASSERT_FALSE(entry.attributes.markdown.empty());
  MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
  measurer.SetContent(entry.attributes.markdown);
  measurer.Measure({.width_ = entry.attributes.width,
                    .width_mode_ = tttext::LayoutMode::kAtMost,
                    .height_ = entry.attributes.height,
                    .height_mode_ = tttext::LayoutMode::kAtMost});

  const std::vector<std::string> expected = {
      R"(\textbf{加粗文字} \textit{斜体文字} \underline{下划线文字})",
      R"(% 无序列表
\begin{itemize}
  \item 第一项
  \item 第二项
\end{itemize}

% 有序列表
\begin{enumerate}
  \item 步骤一
  \item 步骤二
\end{enumerate})",
      R"(\includegraphics[width=0.8\textwidth]{你的图片路径.jpg})"};
  auto document = measurer.GetDocument();
  const auto ranges = document->GetSyntaxSourceRanges("codeBlock");
  ASSERT_EQ(ranges.size(), expected.size());
  std::vector<std::string> actual;
  for (const auto& element : document->GetParagraphs()) {
    if (element->GetMarkdownSourceType() != MarkdownSyntaxType::kCodeBlock) {
      continue;
    }
    ASSERT_EQ(element->GetType(), MarkdownElementType::kParagraph);
    auto* paragraph =
        static_cast<MarkdownParagraphElement*>(element.get())->GetParagraph();
    actual.emplace_back(
        paragraph->GetContentString(0, paragraph->GetCharCount()));
  }
  EXPECT_EQ(actual, expected);
  const auto& source = entry.attributes.markdown;
  for (size_t i = 0; i < ranges.size(); ++i) {
    const auto start =
        UTF8IndexToCIndex(source.c_str(), source.size(), ranges[i].start_);
    const auto end =
        UTF8IndexToCIndex(source.c_str(), source.size(), ranges[i].end_);
    EXPECT_EQ(source.substr(start, end - start),
              "```latex\n" + expected[i] + "\n```");
  }
}

TEST(MarkdownViewMeasurerTest, CodeBlockPreservesBlankLines) {
  const std::vector<std::string> contents = {"first\n\nsecond",
                                             "\nfirst\n\n\nsecond\n"};
  for (const auto& content : contents) {
    for (const auto& suffix : {"```", ""}) {
      SCOPED_TRACE(content + suffix);
      MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
      measurer.SetContent("```\n" + content + "\n" + suffix);
      measurer.Measure({.width_ = 320,
                        .width_mode_ = tttext::LayoutMode::kAtMost,
                        .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
                        .height_mode_ = tttext::LayoutMode::kIndefinite});
      const auto& elements = measurer.GetDocument()->GetParagraphs();
      ASSERT_EQ(elements.size(), 1u);
      EXPECT_EQ(elements[0]->GetMarkdownSourceType(),
                MarkdownSyntaxType::kCodeBlock);
      auto* paragraph =
          static_cast<MarkdownParagraphElement*>(elements[0].get())
              ->GetParagraph();
      EXPECT_EQ(paragraph->GetContentString(0, paragraph->GetCharCount()),
                content);
    }
  }
}

TEST(MarkdownViewMeasurerTest, CodeBlockBlankLinesRespectContentRange) {
  const std::string source = "```\nfirst\n\nsecond\n```";
  for (const std::string& content : {"\n", "second"}) {
    SCOPED_TRACE(content);
    MarkdownViewMeasurer measurer(testing::CreateTestMarkdownSharedContext());
    measurer.SetContent(source);
    const auto start =
        content == "\n" ? source.find("\n\n") + 1 : source.find(content);
    measurer.SetContentRange({static_cast<int32_t>(start),
                              static_cast<int32_t>(start + content.size())});
    measurer.Measure({.width_ = 320,
                      .width_mode_ = tttext::LayoutMode::kAtMost,
                      .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
                      .height_mode_ = tttext::LayoutMode::kIndefinite});
    const auto& elements = measurer.GetDocument()->GetParagraphs();
    ASSERT_EQ(elements.size(), 1u);
    auto* paragraph = static_cast<MarkdownParagraphElement*>(elements[0].get())
                          ->GetParagraph();
    EXPECT_EQ(paragraph->GetContentString(0, paragraph->GetCharCount()),
              content);
  }
}

}  // namespace serval::markdown
