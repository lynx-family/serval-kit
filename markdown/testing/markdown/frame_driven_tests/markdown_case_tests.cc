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
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "testing/markdown/frame_driven_tests/markdown_case_builder.h"
#include "testing/markdown/frame_driven_tests/markdown_frame_driver.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_canvas.h"
#include "testing/markdown/mock_platform/mock_markdown_platform_view.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"
#define GROUND_TRUTH_PATH "markdown/testing/markdown/ground_truth"
namespace serval::markdown {
namespace testing {
namespace fs = std::filesystem;
const fs::path CASES_PATH = "markdown/testing/markdown/cases";

std::string SerializeJson(const rapidjson::Value& value) {
  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
  writer.SetIndent(' ', 2);
  value.Accept(writer);
  return s.GetString();
}
void ExpectValue(const std::unique_ptr<Value>& result,
                 const std::unique_ptr<Value>& truth);
void ExpectMap(const ValueMap& result, const ValueMap& truth) {
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
void ExpectArray(const ValueArray& result, const ValueArray& truth) {
  EXPECT_EQ(result.size(), truth.size());
  if (result.size() != truth.size())
    return;
  for (size_t i = 0; i < result.size(); i++) {
    ExpectValue(result[i], truth[i]);
  }
}
void ExpectValue(const std::unique_ptr<Value>& result,
                 const std::unique_ptr<Value>& truth) {
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

void RunSingleCase(MarkdownCaseEntry& single_case) {
  SCOPED_TRACE(single_case.name);
  ASSERT_NE(single_case.attributes.markdown.length(), 0);
  auto context = CreateTestMarkdownSharedContext();
  MockMarkdownResourceLoader resource_loader;
  MockMarkdownCanvas canvas(&resource_loader);
  MockMarkdownMainView main_view(context);
  MarkdownCaseBuilder::ApplyAttributes(single_case.attributes,
                                       main_view.GetMarkdownView());
  main_view.GetMarkdownView()->SetResourceLoader(&resource_loader);
  resource_loader.SetMainView(&main_view);
  MarkdownFrameDriver driver(&main_view, &canvas);
  const auto& result = driver.RunSteps(single_case.steps);
  if (single_case.attributes.generate_ground_truth) {
    auto result_json = SerializeJson(result);
    std::ofstream output(single_case.directory / "ground_truth.json");
    output << result_json;
    output.flush();
    output.close();
  } else {
    auto result_value = MarkdownCaseBuilder::ConvertJson(result);
    ExpectValue(result_value, single_case.attributes.ground_truth);
  }
}

void RunSingleCase(const fs::path& directory) {
  auto single_case = MarkdownCaseBuilder::LoadSingleCase(directory);
  RunSingleCase(single_case);
}

TEST(MarkdownCaseUnittest, BasicHeadingsH1H6) {
  RunSingleCase(CASES_PATH / "basic_headings_h1_h6");
}

TEST(MarkdownCaseUnittest, CodeFenceUnclosed) {
  RunSingleCase(CASES_PATH / "code_fence_unclosed");
}

TEST(MarkdownCaseUnittest, DemoAllFeatures) {
  RunSingleCase(CASES_PATH / "demo_all_features");
}

TEST(MarkdownCaseUnittest, EntityAndMultilineBreaks) {
  RunSingleCase(CASES_PATH / "entity_and_multiline_breaks");
}

TEST(MarkdownCaseUnittest, FrameSteps) {
  RunSingleCase(CASES_PATH / "frame_steps");
}

TEST(MarkdownCaseUnittest, FrameVisibleRectSingle) {
  RunSingleCase(CASES_PATH / "frame_visible_rect_single");
}

TEST(MarkdownCaseUnittest, FrameVisibleRects) {
  RunSingleCase(CASES_PATH / "frame_visible_rects");
}

TEST(MarkdownCaseUnittest, HrOnlyBlocks) {
  RunSingleCase(CASES_PATH / "hr_only_blocks");
}

TEST(MarkdownCaseUnittest, ImageAltFallbackIntrinsicSize) {
  RunSingleCase(CASES_PATH / "image_alt_fallback_intrinsic_size");
}

TEST(MarkdownCaseUnittest, ImageCaptionAltFallback) {
  RunSingleCase(CASES_PATH / "image_caption_alt_fallback");
}

TEST(MarkdownCaseUnittest, InlineComplexLink) {
  RunSingleCase(CASES_PATH / "inline_complex_link");
}

TEST(MarkdownCaseUnittest, InlineHtmlClassAndBr) {
  RunSingleCase(CASES_PATH / "inline_html_class_and_br");
}

TEST(MarkdownCaseUnittest, LineExpandFrames) {
  RunSingleCase(CASES_PATH / "line_expand_frames");
}

TEST(MarkdownCaseUnittest, NormalBasicDocument) {
  RunSingleCase(CASES_PATH / "normal_basic_document");
}

TEST(MarkdownCaseUnittest, NormalHrSections) {
  RunSingleCase(CASES_PATH / "normal_hr_sections");
}

TEST(MarkdownCaseUnittest, NormalHtmlMarkSpan) {
  RunSingleCase(CASES_PATH / "normal_html_mark_span");
}

TEST(MarkdownCaseUnittest, NormalImageCaption) {
  RunSingleCase(CASES_PATH / "normal_image_caption");
}

TEST(MarkdownCaseUnittest, NormalInlineStyles) {
  RunSingleCase(CASES_PATH / "normal_inline_styles");
}

TEST(MarkdownCaseUnittest, NormalNestedLists) {
  RunSingleCase(CASES_PATH / "normal_nested_lists");
}

TEST(MarkdownCaseUnittest, NormalQuoteCode) {
  RunSingleCase(CASES_PATH / "normal_quote_code");
}

TEST(MarkdownCaseUnittest, NormalTableLinks) {
  RunSingleCase(CASES_PATH / "normal_table_links");
}

TEST(MarkdownCaseUnittest, PlainTextSource) {
  RunSingleCase(CASES_PATH / "plain_text_source");
}

TEST(MarkdownCaseUnittest, TableEscapePipeShortAlign) {
  RunSingleCase(CASES_PATH / "table_escape_pipe_short_align");
}

TEST(MarkdownCaseUnittest, TaskListEdge) {
  RunSingleCase(CASES_PATH / "task_list_edge");
}

TEST(MarkdownCaseUnittest, Template) {
  RunSingleCase(CASES_PATH / "template");
}

TEST(MarkdownCaseUnittest, TextAttachmentsMark) {
  RunSingleCase(CASES_PATH / "text_attachments_mark");
}

TEST(MarkdownCaseUnittest, TextAttachmentsNegativeIndex) {
  RunSingleCase(CASES_PATH / "text_attachments_negative_index");
}

TEST(MarkdownCaseUnittest, TextAttachmentsSourceIndex) {
  RunSingleCase(CASES_PATH / "text_attachments_source_index");
}

TEST(MarkdownCaseUnittest, TypewriterFrames) {
  RunSingleCase(CASES_PATH / "typewriter_frames");
}

TEST(MarkdownCaseUnittest, ViewPropsEffectRange) {
  RunSingleCase(CASES_PATH / "view_props_effect_range");
}

}  // namespace testing
}  // namespace serval::markdown
