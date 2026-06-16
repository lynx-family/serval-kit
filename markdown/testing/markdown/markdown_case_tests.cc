// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <fstream>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "markdown/draw/markdown_drawer.h"
#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_view.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "testing/markdown/markdown_case_builder.h"
#include "testing/markdown/markdown_tests_platform.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_frame_driver.h"
#include "testing/markdown/mock_markdown_platform_view.h"
#include "testing/markdown/mock_markdown_resource_loader.h"
#define GROUND_TRUTH_PATH "markdown/testing/markdown/ground_truth"
namespace serval::markdown {
namespace testing {
namespace fs = std::filesystem;
const fs::path CASES_PATH = "markdown/testing/markdown/cases";
using ValuePtr = MarkdownCaseValuePtr;

std::string SerializeJson(const rapidjson::Value& value) {
  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
  writer.SetIndent(' ', 2);
  value.Accept(writer);
  return s.GetString();
}

class MarkdownCaseUnittest : public MarkdownCaseConfig {
 public:
  MarkdownCaseUnittest() {
    resource_loader_ = std::make_unique<MockMarkdownResourceLoader>();
    document_ = std::make_unique<MarkdownDocument>(
        CreateTestMarkdownSharedContext(), resource_loader_.get());
    canvas_ = std::make_unique<MockMarkdownCanvas>(resource_loader_.get(),
                                                   document_.get());

    const auto default_attributes = CASES_PATH / "template" / "attributes.json";
    EXPECT_TRUE(fs::is_regular_file(default_attributes));
    if (!fs::is_regular_file(default_attributes)) {
      return;
    }
    ApplyAttributes(MarkdownCaseBuilder::ReadJsonFileToValue(
        default_attributes));
  }

  void ParseLayoutAndDraw() const {
    Parse();
    Layout();
    Draw();
  }
  void Parse() const {
    if (!markdown_.empty()) {
      auto style =
          MarkdownStyleReader::ReadStyle(style_map_, resource_loader_.get());
      document_->SetStyle(style);
      document_->SetMarkdownContent(markdown_);
      document_->SetMarkdownContentRange(
          Range{0, std::numeric_limits<int32_t>::max()});
      document_->SetMaxSize(width_, height_);
      document_->SetMaxLines(-1);
      MarkdownParserImpl::ParseMarkdown("", document_.get(), nullptr);
    }
  }
  void Layout() const {
    MarkdownLayout layout(document_.get());
    layout.Layout(width_, height_, max_lines_);
    if (attachments_ != nullptr) {
      auto attachments = MarkdownStyleReader::ReadTextAttachments(
          attachments_.get(), document_.get());
      document_->GetPage()->ClearAttachments();
      document_->GetPage()->AddTextAttachments(std::move(attachments));
    }
  }
  void Draw() const {
    if (animation_type_ != MarkdownAnimationType::kNone && !region_view_) {
      auto cursor = document_->GetStyle()
                        .typewriter_cursor_.typewriter_cursor_.custom_cursor_;
      auto cursor_delegate = cursor.empty() ? nullptr
                                            : resource_loader_->LoadInlineView(
                                                  cursor.c_str(), 10, 10);
      MarkdownCharTypewriterDrawer drawer(
          document_->GetContextPtr(), canvas_.get(), animation_step_,
          resource_loader_.get(), document_->GetStyle().typewriter_cursor_,
          draw_cursor_if_complete_, cursor_delegate.get());
      drawer.DrawPage(*document_->GetPage());
    } else if (region_view_) {
      const auto page = document_->GetPage();
      float top = 0;
      float bottom = height_;
      if (!region_rect_.IsEmpty()) {
        top = region_rect_.GetTop();
        bottom = region_rect_.GetBottom();
      }
      const auto region_range = document_->GetShowedRegions(top, bottom);
      for (auto i = region_range.start_; i < region_range.end_; i++) {
        auto region_rect = document_->GetPage()->GetRegionRect(i);
        canvas_->Save();
        canvas_->Translate(-region_rect.GetLeft(), -region_rect.GetTop());
        if (animation_type_ != MarkdownAnimationType::kNone) {
          auto cursor =
              document_->GetStyle()
                  .typewriter_cursor_.typewriter_cursor_.custom_cursor_;
          auto cursor_delegate =
              cursor.empty()
                  ? nullptr
                  : resource_loader_->LoadInlineView(cursor.c_str(), 10, 10);
          MarkdownCharTypewriterDrawer drawer(
              document_->GetContextPtr(), canvas_.get(), animation_step_,
              resource_loader_.get(), document_->GetStyle().typewriter_cursor_,
              draw_cursor_if_complete_, cursor_delegate.get());
          drawer.DrawRegion(*document_->GetPage(), i);
        } else {
          MarkdownDrawer drawer(canvas_.get(), document_->GetContextPtr());
          drawer.DrawRegion(*document_->GetPage(), i);
        }
        canvas_->Restore();
      }
      const auto extra_range = document_->GetShowedExtraContents(top, bottom);
      for (auto i = extra_range.start_; i < extra_range.end_; i++) {
        MarkdownDrawer drawer(canvas_.get(), document_->GetContextPtr());
        drawer.DrawQuoteBorder(*page, i);
      }
    } else {
      MarkdownDrawer drawer(canvas_.get(), document_->GetContextPtr());
      drawer.DrawPage(*document_->GetPage());
    }
  }

  void ApplyAttributes(const ValuePtr& attributes) {
    MarkdownCaseBuilder builder(*this);
    builder.ApplyAttributes(attributes.get());
  }

  bool LoadCaseInDirectory(const fs::path& path) {
    MarkdownCaseBuilder builder(*this);
    return builder.LoadCaseInDirectory(path);
  }

  std::vector<MockMarkdownFrameStep> BuildFrameSteps() const {
    const auto case_steps = MarkdownCaseBuilder::BuildFrameSteps(*this);
    std::vector<MockMarkdownFrameStep> steps;
    steps.reserve(case_steps.size());
    for (const auto& case_step : case_steps) {
      MockMarkdownFrameStep step;
      step.interval_ms_ = case_step.interval_ms_;
      step.has_visible_rect_ = case_step.has_visible_rect_;
      step.visible_rect_ = case_step.visible_rect_;
      steps.push_back(step);
    }
    return steps;
  }

  void ConfigureView(MarkdownView* view, bool attach_marks = true) {
    view->SetResourceLoader(resource_loader_.get());
    view->SetSourceType(source_type_);
    view->SetStyle(style_map_);
    view->SetContent(markdown_);
    view->SetContentComplete(content_complete_);
    if (max_lines_ >= 0) {
      view->SetTextMaxLines(max_lines_);
    }
    if (attach_marks && attachments_ != nullptr) {
      view->SetTextAttachments(std::move(attachments_));
    }
    view->SetAnimationType(animation_type_);
    view->SetInitialAnimationStep(animation_step_);
    view->SetAnimationVelocity(static_cast<float>(animation_velocity_));
  }

  MeasureSpec BuildMeasureSpec() const {
    MeasureSpec spec;
    spec.width_ = width_;
    spec.width_mode_ = tttext::LayoutMode::kDefinite;
    spec.height_ = height_;
    spec.height_mode_ = tttext::LayoutMode::kIndefinite;
    return spec;
  }

  rapidjson::Document RunViewCase() {
    auto main_view = std::make_unique<MockMarkdownMainView>();
    resource_loader_->SetMainView(main_view.get());
    ConfigureView(main_view->GetMarkdownView());

    MockMarkdownFrameDriver driver(main_view.get(), canvas_.get());
    driver.SetMeasureSpec(BuildMeasureSpec());
    driver.SetDefaultVisibleRect(RectF::MakeLTRB(0, 0, width_, height_));
    auto result = driver.Run(BuildFrameSteps());
    resource_loader_->SetMainView(nullptr);
    return result;
  }

  void RunCaseInDirectory(const fs::path& path) {
    auto case_name = path.filename();
    printf("running case:%s\n", case_name.c_str());

    const auto ground_truth_path = path / "ground_truth.json";
    if (!LoadCaseInDirectory(path)) {
      return;
    }
    const auto frames = RunViewCase();
    const auto result = MarkdownCaseBuilder::ConvertJson(frames);
    if (generate_ground_truth_) {
      std::ofstream output(ground_truth_path);
      output << SerializeJson(frames);
      output.flush();
      output.close();
    } else if (fs::is_regular_file(ground_truth_path)) {
      const auto ground_truth =
          MarkdownCaseBuilder::ReadJsonFileToValue(ground_truth_path);
      ExpectValue(result, ground_truth);
    } else {
      return;
    }
    printf("end case:%s\n", case_name.c_str());
  }

  void ExpectCanvas(const ValuePtr& ground_truth) const {
    const auto result = MarkdownCaseBuilder::ConvertJson(canvas_->GetJson());
    ExpectValue(result, ground_truth);
  }

  static void ExpectMap(const ValueMap& result, const ValueMap& truth) {
    EXPECT_EQ(result.size(), truth.size());
    if (result.size() != truth.size())
      return;
    for (const auto& [key, value] : result) {
      auto v2 = truth.find(key);
      EXPECT_NE(v2, truth.end());
      if (v2 == truth.end())
        return;
      ExpectValue(value, v2->second);
    }
  }
  static void ExpectArray(const ValueArray& result, const ValueArray& truth) {
    EXPECT_EQ(result.size(), truth.size());
    if (result.size() != truth.size())
      return;
    for (size_t i = 0; i < result.size(); i++) {
      ExpectValue(result[i], truth[i]);
    }
  }
  static void ExpectValue(const ValuePtr& result, const ValuePtr& truth) {
    EXPECT_EQ(result->GetType(), truth->GetType());
    if (result->GetType() != truth->GetType())
      return;
    switch (result->GetType()) {
      case ValueType::kNull:
        break;
      case ValueType::kMap:
        ExpectMap(result->AsMap(), truth->AsMap());
        break;
      case ValueType::kArray:
        ExpectArray(result->AsArray(), truth->AsArray());
        break;
      case ValueType::kBool:
        EXPECT_EQ(result->AsBool(), truth->AsBool());
        break;
      case ValueType::kInt:
        EXPECT_EQ(result->AsInt(), truth->AsInt());
        break;
      case ValueType::kLong:
        EXPECT_EQ(result->AsLong(), truth->AsLong());
        break;
      case ValueType::kDouble:
        EXPECT_NEAR(result->AsDouble(), truth->AsDouble(), 1e-3);
        break;
      case ValueType::kString:
        EXPECT_EQ(result->AsString(), truth->AsString());
        break;
    }
  }

  std::unique_ptr<MockMarkdownResourceLoader> resource_loader_;
  std::unique_ptr<MarkdownDocument> document_;
  std::unique_ptr<MockMarkdownCanvas> canvas_;
};
TEST(MarkdownCaseUnittest, Cases) {
  for (const auto& test_case : fs::directory_iterator(CASES_PATH)) {
    if (test_case.is_directory()) {
      MarkdownCaseUnittest unittest;
      unittest.RunCaseInDirectory(test_case);
    }
  }
}

TEST(MarkdownCaseUnittest, SimpleTextTest) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = "simple text";
  unittest.width_ = 100;
  unittest.Parse();
  auto& paras = unittest.document_->GetParagraphs();
  EXPECT_EQ(paras.size(), 1ull);
  auto element = paras.front();
  EXPECT_EQ(element->GetType(), MarkdownElementType::kParagraph);
  auto* para =
      static_cast<MarkdownParagraphElement*>(element.get())->GetParagraph();
  auto parsed_content = para->GetContentString(0, para->GetCharCount());
  EXPECT_EQ(unittest.markdown_, parsed_content);
}

TEST(MarkdownCaseUnittest, GetContentByCharPosLineBreakTest) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
text
text
text
)";
  unittest.width_ = 500;
  unittest.height_ = 500;
  unittest.ParseLayoutAndDraw();
  auto content = unittest.document_->GetContentByCharPos(0, 100);
  EXPECT_EQ(content, "text\ntext\ntext\n");
}
TEST(MarkdownCaseUnittest, ContentRangeCrash) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
- 111
- 222
333 444 555
)";
  auto style = MarkdownStyleReader::ReadStyle(unittest.style_map_,
                                              unittest.resource_loader_.get());
  unittest.document_->SetStyle(style);
  unittest.document_->SetMarkdownContent(unittest.markdown_);
  unittest.document_->SetMarkdownContentRange({15, 25});
  unittest.document_->SetMaxSize(500, 1e5);
  MarkdownParserImpl::ParseMarkdown("", unittest.document_.get(), nullptr);
}
TEST(MarkdownCaseUnittest, GetLinkByTouchPosition) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
This text has a [link](url://link) in the middle.
)";
  unittest.style_map_["normalText"]->AsMap()["fontSize"] =
      Value::MakeDouble(20);
  unittest.ParseLayoutAndDraw();
  const auto& links = unittest.document_->GetLinks();
  ASSERT_EQ(links.size(), 1ull);
  const auto& link_info = links.front();
  auto rects = MarkdownSelection::GetSelectionRectByCharPos(
      unittest.document_->GetPage().get(), link_info.char_start_,
      link_info.char_start_ + link_info.char_count_);
  ASSERT_FALSE(rects.empty());
  auto left = rects.front().GetLeft();
  auto top = rects.front().GetTop();
  auto right = rects.back().GetRight();
  auto inside = unittest.document_->GetLinkByTouchPosition({left + 1, top + 1});
  EXPECT_EQ(inside->url_, "url://link");
  auto outside_left =
      unittest.document_->GetLinkByTouchPosition({left - 5, top + 1});
  EXPECT_EQ(outside_left, nullptr);
  auto outside_right =
      unittest.document_->GetLinkByTouchPosition({right + 5, top + 1});
  EXPECT_EQ(outside_right, nullptr);
}
TEST(MarkdownCaseUnittest, TypewriterDynamicHeight) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
|header|header|header|
|-|-|
|body|body|body|
|body|body|body|
)";
  unittest.ParseLayoutAndDraw();
  MarkdownCharTypewriterDrawer typewriter_drawer(
      unittest.document_->GetContextPtr(), unittest.canvas_.get(), 1000,
      unittest.resource_loader_.get(),
      unittest.document_->GetStyle().typewriter_cursor_, false, nullptr);
  typewriter_drawer.DrawPage(*unittest.document_->GetPage());
  EXPECT_EQ(typewriter_drawer.GetMaxDrawHeight(), 120);
}

TEST(MarkdownCaseUnittest, TableSelectionRegionTracksRowBottom) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
|header|header|header|
|-|-|-|
|body|body|body|
|body|body|body|
)";
  unittest.ParseLayoutAndDraw();
  auto page = unittest.document_->GetPage();
  ASSERT_NE(page, nullptr);
  ASSERT_GT(page->GetRegionCount(), 0u);
  ASSERT_EQ(page->GetRegion(0)->element_->GetType(),
            MarkdownElementType::kTable);
  auto* table_region =
      static_cast<MarkdownPageTableRegion*>(page->GetRegion(0));
  ASSERT_NE(table_region, nullptr);

  const int32_t first_row_end = unittest.document_->GetCharIndexByLineIndex(0);
  const float expected_height =
      page->GetRegion(0)->rect_.GetTop() +
      table_region->table_->GetCell(0, 0).cell_rect_.GetBottom();
  const auto selection_regions =
      MarkdownSelection::GetSelectionRegionsByCharRange(
          page.get(), first_row_end - 1, first_row_end);
  ASSERT_FALSE(selection_regions.empty());
  EXPECT_EQ(selection_regions.back().row_bottom_, expected_height);
}
TEST(MarkdownCaseUnittest, SelectionGetContent) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
A very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text, a very long text.
)";
  unittest.style_map_["normalText"]->AsMap()["fontSize"] =
      Value::MakeDouble(25);
  unittest.max_lines_ = 1;
  unittest.ParseLayoutAndDraw();
  auto content = unittest.document_->GetContentByCharPos(0, 1000);
  EXPECT_EQ(content, "A very long text, a ");
  unittest.max_lines_ = 2;
  unittest.ParseLayoutAndDraw();
  content = unittest.document_->GetContentByCharPos(0, 1000);
  EXPECT_EQ(content, "A very long text, a very long text, a ve");
}
TEST(MarkdownCaseUnittest, EmptyContent) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(
- ####
)";
  unittest.ParseLayoutAndDraw();
}

TEST(MarkdownCaseUnittest, LineIndexEmptyPage) {
  MarkdownPage page;
  EXPECT_EQ(MarkdownSelection::GetLineCount(&page), 0);
  EXPECT_EQ(MarkdownSelection::GetCharIndexByLineIndex(&page, -1), 0);
  EXPECT_EQ(MarkdownSelection::GetCharIndexByLineIndex(&page, 0), 0);
  EXPECT_EQ(MarkdownSelection::GetLineIndexByCharIndex(&page, -1), 0);
  EXPECT_EQ(MarkdownSelection::GetLineIndexByCharIndex(&page, 0), 0);
}

TEST(MarkdownCaseUnittest, LineIndexCharIndexConvert) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ =
      "aa\n\n"
      "bb\n\n"
      "cc\n\n"
      "|h1|h2|\n"
      "|-|-|\n"
      "|r1c1|r1c2|\n"
      "|r2c1|r2c2|\n";
  unittest.width_ = 1000;
  unittest.height_ = 1000;
  unittest.ParseLayoutAndDraw();

  auto page = unittest.document_->GetPage();
  ASSERT_NE(page, nullptr);

  const int line_count = MarkdownSelection::GetLineCount(page.get());
  EXPECT_EQ(line_count, unittest.document_->GetLineCount());

  const int total_char_count = MarkdownSelection::GetPageCharCount(page.get());
  EXPECT_GT(total_char_count, 0);

  EXPECT_EQ(MarkdownSelection::GetCharIndexByLineIndex(page.get(), -1), 0);
  EXPECT_EQ(MarkdownSelection::GetCharIndexByLineIndex(page.get(), line_count),
            total_char_count);
  EXPECT_EQ(MarkdownSelection::GetLineIndexByCharIndex(page.get(), -1), 0);
  EXPECT_EQ(
      MarkdownSelection::GetLineIndexByCharIndex(page.get(), total_char_count),
      line_count);

  const std::vector<int> expected_line_end_char_indices{
      2, 4, 6, 10, 18, 26,
  };
  const auto line_end_char_indices =
      unittest.document_->GetLineEndCharIndices();
  ASSERT_EQ(static_cast<size_t>(line_count),
            expected_line_end_char_indices.size());
  EXPECT_EQ(total_char_count, expected_line_end_char_indices.back());
  ASSERT_EQ(line_end_char_indices.size(),
            expected_line_end_char_indices.size());

  for (int i = 0; i < line_count; ++i) {
    const int char_index =
        MarkdownSelection::GetCharIndexByLineIndex(page.get(), i);
    EXPECT_EQ(char_index, expected_line_end_char_indices[i]);
    EXPECT_EQ(line_end_char_indices[static_cast<size_t>(i)],
              expected_line_end_char_indices[static_cast<size_t>(i)]);
    EXPECT_EQ(unittest.document_->GetCharIndexByLineIndex(i), char_index);
    if (char_index > 0) {
      EXPECT_EQ(MarkdownSelection::GetLineIndexByCharIndex(page.get(),
                                                           char_index - 1),
                i);
      EXPECT_EQ(unittest.document_->GetLineIndexByCharIndex(char_index - 1), i);
    }
  }
}

TEST(MarkdownCaseUnittest, OffsetConvert) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = R"(  ### Below Are Headings

## Level 2 Heading
### Level 3 Heading
#### Level 4 Heading
##### Level 5 Heading

### Below Is Normal Text:

This is a normal paragraph that includes **bold**, *italic*, ***bold+italic***, ~~strikethrough~~, `inline code`, <mark>these inline styles</mark>, and an emoji: 😄

> This is a paragraph in a quote
> It has a cross-paragraph span tag: <span>content from the previous line
> content from the next line</span> span parses correctly.
)";
  unittest.Parse();
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(0), 0);
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(5), 0);
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(7), 1);
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(15), 9);
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(80), 58);
  ASSERT_EQ(unittest.document_->MarkdownOffsetToCharOffset(204), 158);
}

TEST(MarkdownCaseUnittest, DrawAttachment) {
  MarkdownCaseUnittest unittest;
  EXPECT_TRUE(unittest.LoadCaseInDirectory(CASES_PATH / "template"));
  unittest.ParseLayoutAndDraw();
  MarkdownTextAttachment attachment{
      .start_index_ = 20,
      .end_index_ = 50,
      .index_type_ = CharIndexType::kParsedContent,
      .attachment_layer_ = AttachmentLayer::kBackground,
      .id_ = "attachment",
      .clickable_ = false,
      .rect_ =
          {
              .gradient_ = nullptr,
          },
      .border_top_ = {.line_type_ = MarkdownLineType::kSolid},
      .border_bottom_ = {.line_type_ = MarkdownLineType::kDashed},
  };
  const auto rects = MarkdownSelection::GetSelectionRectByCharPos(
      unittest.document_->GetPage().get(), attachment.start_index_,
      attachment.end_index_);
  attachment.DrawOnMultiLines(unittest.canvas_.get(), rects);
}

TEST(MarkdownCaseUnittest, MarkAttachments) {
  MarkdownCaseUnittest unittest;
  if (unittest.LoadCaseInDirectory(CASES_PATH / "text_attachments_mark")) {
    unittest.ParseLayoutAndDraw();
  }
}

TEST(MarkdownCaseUnittest, SetArrayPropAddsMarkAttachments) {
  MarkdownCaseUnittest unittest;
  ASSERT_TRUE(
      unittest.LoadCaseInDirectory(CASES_PATH / "text_attachments_mark"));
  ASSERT_NE(unittest.attachments_, nullptr);

  MarkdownCaseUnittest expected;
  ASSERT_TRUE(
      expected.LoadCaseInDirectory(CASES_PATH / "text_attachments_mark"));
  expected.ParseLayoutAndDraw();

  auto main_view = std::make_unique<MockMarkdownMainView>();
  auto* view_ptr = main_view->GetMarkdownView();
  unittest.resource_loader_->SetMainView(main_view.get());

  unittest.ConfigureView(view_ptr, false);

  main_view->ResetRequestCount();
  view_ptr->SetArrayProp(MarkdownProps::kTextMarkAttachments,
                         unittest.attachments_->AsArray());
  EXPECT_GT(main_view->GetRequestMeasureCount(), 0);

  MockMarkdownFrameDriver driver(main_view.get(), unittest.canvas_.get());
  driver.SetMeasureSpec(unittest.BuildMeasureSpec());
  driver.SetDefaultVisibleRect(
      RectF::MakeLTRB(0, 0, unittest.width_, unittest.height_));
  const auto frames = driver.Run(unittest.BuildFrameSteps());
  unittest.resource_loader_->SetMainView(nullptr);

  ASSERT_EQ(frames.Size(), 1u);
  const auto actual_ops = MarkdownCaseBuilder::ConvertJson(frames[0]["ops"]);
  const auto expected_ops =
      MarkdownCaseBuilder::ConvertJson(expected.canvas_->GetJson());
  MarkdownCaseUnittest::ExpectValue(actual_ops, expected_ops);
}

TEST(MarkdownCaseUnittest, SetMapPropAddsMarkdownEffect) {
  MarkdownCaseUnittest unittest;
  ASSERT_TRUE(unittest.LoadCaseInDirectory(CASES_PATH / "template"));

  auto main_view = std::make_unique<MockMarkdownMainView>();
  auto* view_ptr = main_view->GetMarkdownView();
  unittest.resource_loader_->SetMainView(main_view.get());
  unittest.ConfigureView(view_ptr, false);

  ValueMap effect;
  effect.emplace("rangeStart", Value::MakeInt(5));
  effect.emplace("rangeEnd", Value::MakeInt(16));
  effect.emplace("color", Value::MakeString("#ff0000"));

  main_view->ResetRequestCount();
  view_ptr->SetMapProp(MarkdownProps::kMarkdownEffect, effect);
  EXPECT_GT(main_view->GetRequestMeasureCount(), 0);

  MockMarkdownFrameDriver driver(main_view.get(), unittest.canvas_.get());
  driver.SetMeasureSpec(unittest.BuildMeasureSpec());
  driver.SetDefaultVisibleRect(
      RectF::MakeLTRB(0, 0, unittest.width_, unittest.height_));
  const auto frames = driver.Run(unittest.BuildFrameSteps());
  unittest.resource_loader_->SetMainView(nullptr);

  ASSERT_EQ(frames.Size(), 1u);
  const auto& ops = frames[0]["ops"];
  ASSERT_TRUE(ops.IsArray());
  bool has_effect_rect = false;
  for (const auto& op : ops.GetArray()) {
    if (op.IsObject() && op.HasMember("op") && op["op"].IsString() &&
        std::string_view(op["op"].GetString()) == "rect" &&
        op.HasMember("painter") && op["painter"].IsObject() &&
        op["painter"].HasMember("fill_color") &&
        op["painter"]["fill_color"].GetUint() == 0xffff0000) {
      has_effect_rect = true;
      break;
    }
  }
  EXPECT_TRUE(has_effect_rect);
}

TEST(MarkdownCaseUnittest, LongPressDragAdjustsEndHandle) {
  MarkdownCaseUnittest unittest;
  unittest.markdown_ = "abcdefg";
  unittest.width_ = 300;
  unittest.height_ = 200;

  auto main_view = std::make_unique<MockMarkdownMainView>();
  auto* view = main_view->GetMarkdownView();
  unittest.resource_loader_->SetMainView(main_view.get());
  unittest.ConfigureView(view, false);
  view->SetEnableSelection(true);

  MockMarkdownFrameDriver driver(main_view.get(), unittest.canvas_.get());
  driver.SetMeasureSpec(unittest.BuildMeasureSpec());
  driver.SetDefaultVisibleRect(
      RectF::MakeLTRB(0, 0, unittest.width_, unittest.height_));
  (void)driver.Run(unittest.BuildFrameSteps());
  unittest.resource_loader_->SetMainView(nullptr);

  const auto first_char_rect = view->GetTextBoundingRect({0, 1});
  const PointF long_press_position{
      (first_char_rect.GetLeft() + first_char_rect.GetRight()) * 0.5f,
      (first_char_rect.GetTop() + first_char_rect.GetBottom()) * 0.5f};
  ASSERT_TRUE(view->OnLongPress(long_press_position, GestureEventType::kDown));
  EXPECT_EQ(view->GetSelectedRange().start_, 0);
  EXPECT_EQ(view->GetSelectedRange().end_, 1);

  const auto target_char_rect = view->GetTextBoundingRect({3, 4});
  const PointF drag_position{
      (target_char_rect.GetLeft() + target_char_rect.GetRight()) * 0.5f,
      (target_char_rect.GetTop() + target_char_rect.GetBottom()) * 0.5f};
  const auto motion = drag_position - long_press_position;
  ASSERT_TRUE(view->ShouldBeginPan(drag_position, motion));
  ASSERT_TRUE(view->OnPan(drag_position, motion, GestureEventType::kDown));
  ASSERT_TRUE(view->OnPan(drag_position, motion, GestureEventType::kMove));

  EXPECT_EQ(view->GetSelectedRange().start_, 1);
  EXPECT_EQ(view->GetSelectedRange().end_, 3);
}
}  // namespace testing
}  // namespace serval::markdown
