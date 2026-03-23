// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <algorithm>
#include <fstream>

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
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_frame_driver.h"
#include "testing/markdown/mock_markdown_platform_view.h"
#include "testing/markdown/mock_markdown_resource_loader.h"
#define GROUND_TRUTH_PATH "markdown/testing/markdown/ground_truth"
namespace lynx {
namespace markdown {
namespace testing {
namespace fs = std::filesystem;
const fs::path CASES_PATH = "markdown/testing/markdown/cases";
using ValuePtr = std::unique_ptr<Value>;
std::unique_ptr<Value> ConvertJson(const rapidjson::Value& value) {
  switch (value.GetType()) {
    case rapidjson::kFalseType:
      return Value::MakeBool(false);
    case rapidjson::kTrueType:
      return Value::MakeBool(true);
    case rapidjson::kObjectType: {
      ValueMap map;
      for (auto& [key, var] : value.GetObject()) {
        map[key.GetString()] = ConvertJson(var);
      }
      return Value::MakeMap(std::move(map));
    }
    case rapidjson::kArrayType: {
      ValueArray array;
      for (uint32_t i = 0; i < value.Size(); i++) {
        array.emplace_back(ConvertJson(value[i]));
      }
      return Value::MakeArray(std::move(array));
    }
    case rapidjson::kStringType:
      return Value::MakeString(value.GetString());
    case rapidjson::kNumberType:
      return Value::MakeDouble(value.GetDouble());
    default:
      return Value::MakeNull();
  }
}
std::unique_ptr<Value> ConvertJson(const std::string& json) {
  rapidjson::Document doc;
  doc.Parse(json);
  return ConvertJson(doc);
}

std::string ReadFileToString(const fs::path& path) {
  std::ifstream input(path);
  std::string str((std::istreambuf_iterator<char>(input)),
                  std::istreambuf_iterator<char>());
  input.close();
  return str;
}

ValuePtr ReadJsonFileToValue(const fs::path& path) {
  auto str = ReadFileToString(path);
  rapidjson::Document doc;
  doc.Parse(str);
  return ConvertJson(doc);
}

std::string SerializeJson(const rapidjson::Value& value) {
  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
  writer.SetIndent(' ', 2);
  value.Accept(writer);
  return s.GetString();
}

bool ParseRectValue(Value* value, RectF* rect) {
  if (value == nullptr || rect == nullptr ||
      value->GetType() != ValueType::kArray) {
    return false;
  }
  const auto& array = value->AsArray();
  if (array.size() < 4) {
    return false;
  }
  *rect = RectF::MakeLTRB(static_cast<float>(array[0]->GetDouble()),
                          static_cast<float>(array[1]->GetDouble()),
                          static_cast<float>(array[2]->GetDouble()),
                          static_cast<float>(array[3]->GetDouble()));
  return true;
}

class MarkdownCaseUnittest {
 public:
  MarkdownCaseUnittest() {
    resource_loader_ = std::make_unique<MockMarkdownResourceLoader>();
    document_ = std::make_unique<MarkdownDocument>(resource_loader_.get());
    canvas_ = std::make_unique<MockMarkdownCanvas>(resource_loader_.get(),
                                                   document_.get());

    const auto default_attributes = CASES_PATH / "template" / "attributes.json";
    EXPECT_TRUE(fs::is_regular_file(default_attributes));
    if (!fs::is_regular_file(default_attributes)) {
      return;
    }
    ApplyAttributes(ReadJsonFileToValue(default_attributes));
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
    if (typewriter_ && !region_view_) {
      auto cursor = document_->GetStyle()
                        .typewriter_cursor_.typewriter_cursor_.custom_cursor_;
      auto cursor_delegate = cursor.empty() ? nullptr
                                            : resource_loader_->LoadInlineView(
                                                  cursor.c_str(), 10, 10);
      MarkdownCharTypewriterDrawer drawer(
          canvas_.get(), animation_step_, resource_loader_.get(),
          document_->GetStyle().typewriter_cursor_, draw_cursor_if_complete_,
          cursor_delegate.get());
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
        if (typewriter_) {
          auto cursor =
              document_->GetStyle()
                  .typewriter_cursor_.typewriter_cursor_.custom_cursor_;
          auto cursor_delegate =
              cursor.empty()
                  ? nullptr
                  : resource_loader_->LoadInlineView(cursor.c_str(), 10, 10);
          MarkdownCharTypewriterDrawer drawer(
              canvas_.get(), animation_step_, resource_loader_.get(),
              document_->GetStyle().typewriter_cursor_,
              draw_cursor_if_complete_, cursor_delegate.get());
          drawer.DrawRegion(*document_->GetPage(), i);
        } else {
          MarkdownDrawer drawer(canvas_.get());
          drawer.DrawRegion(*document_->GetPage(), i);
        }
        canvas_->Restore();
      }
      const auto extra_range = document_->GetShowedExtraContents(top, bottom);
      for (auto i = extra_range.start_; i < extra_range.end_; i++) {
        MarkdownDrawer drawer(canvas_.get());
        drawer.DrawQuoteBorder(*page, i);
      }
    } else {
      MarkdownDrawer drawer(canvas_.get());
      drawer.DrawPage(*document_->GetPage());
    }
  }

  void ApplyAttributes(const ValuePtr& attributes) {
    if (attributes->GetType() == ValueType::kMap) {
      auto& map = attributes->AsMap();
      if (const auto iter = map.find("width"); iter != map.end()) {
        width_ = iter->second->AsDouble();
      }
      if (const auto iter = map.find("height"); iter != map.end()) {
        height_ = iter->second->AsDouble();
      }
      if (const auto iter = map.find("animation-type"); iter != map.end()) {
        typewriter_ = iter->second->AsString() == "typewriter";
      }
      if (const auto iter = map.find("use-char-based-drawer");
          iter != map.end()) {
        use_char_based_drawer_ = iter->second->AsBool();
      }
      if (const auto iter = map.find("generate"); iter != map.end()) {
        generate_ground_truth_ = iter->second->AsBool();
      }
      if (const auto iter = map.find("initial-animation-step");
          iter != map.end()) {
        animation_step_ = iter->second->GetInt();
      }
      if (const auto iter = map.find("text-maxlines"); iter != map.end()) {
        max_lines_ = iter->second->GetInt();
      }
      if (const auto iter = map.find("content-complete"); iter != map.end()) {
        content_complete_ = iter->second->AsBool();
        draw_cursor_if_complete_ = !content_complete_;
      }
      if (const auto iter = map.find("style"); iter != map.end()) {
        MergeMap(style_map_, iter->second->AsMap());
      }
      if (const auto iter = map.find("text-mark-attachments");
          iter != map.end()) {
        attachments_ = std::move(iter->second);
      }
      if (const auto iter = map.find("animation-velocity"); iter != map.end()) {
        animation_velocity_ = iter->second->GetDouble();
      }
      if (const auto iter = map.find("enable-region-view"); iter != map.end()) {
        region_view_ = iter->second->AsBool();
      }
      if (const auto iter = map.find("region-rect");
          iter != map.end() && iter->second->GetType() == ValueType::kArray) {
        ParseRectValue(iter->second.get(), &region_rect_);
      }
      if (const auto iter = map.find("source-type"); iter != map.end()) {
        source_type_ = iter->second->AsString() == "plainText"
                           ? SourceType::kPlainText
                           : SourceType::kMarkdown;
      }
      if (const auto iter = map.find("frame-step-count"); iter != map.end()) {
        frame_step_count_ = std::max(1, iter->second->GetInt());
      }
      if (const auto iter = map.find("frame-step-interval-ms");
          iter != map.end()) {
        const auto value = static_cast<int64_t>(iter->second->GetDouble());
        frame_step_interval_ms_ = std::max<int64_t>(0, value);
      }
      if (const auto iter = map.find("frame-visible-rect"); iter != map.end()) {
        has_frame_visible_rect_ =
            ParseRectValue(iter->second.get(), &frame_visible_rect_);
      }
      if (const auto iter = map.find("frame-visible-rects");
          iter != map.end() && iter->second->GetType() == ValueType::kArray) {
        frame_visible_rects_.clear();
        for (const auto& value : iter->second->AsArray()) {
          RectF rect;
          if (ParseRectValue(value.get(), &rect)) {
            frame_visible_rects_.push_back(rect);
          }
        }
      }
      if (const auto iter = map.find("frame-steps");
          iter != map.end() && iter->second->GetType() == ValueType::kArray) {
        frame_steps_.clear();
        for (const auto& step_value : iter->second->AsArray()) {
          if (step_value->GetType() != ValueType::kMap) {
            continue;
          }
          MockMarkdownFrameStep step;
          const auto& step_map = step_value->AsMap();
          if (const auto interval_iter = step_map.find("interval-ms");
              interval_iter != step_map.end()) {
            step.interval_ms_ =
                std::max<int64_t>(0, interval_iter->second->GetDouble());
          }
          if (const auto interval_iter = step_map.find("interval");
              interval_iter != step_map.end()) {
            step.interval_ms_ =
                std::max<int64_t>(0, interval_iter->second->GetDouble());
          }
          RectF rect;
          if (const auto rect_iter = step_map.find("visible-rect");
              rect_iter != step_map.end() &&
              ParseRectValue(rect_iter->second.get(), &rect)) {
            step.has_visible_rect_ = true;
            step.visible_rect_ = rect;
          }
          frame_steps_.push_back(step);
        }
      }
    }
  }

  bool LoadCaseInDirectory(const fs::path& path) {
    const auto attributes_path = path / "attributes.json";
    const auto markdown_path = path / "markdown.md";
    if (!fs::is_regular_file(markdown_path)) {
      return false;
    }
    if (auto markdown = ReadFileToString(markdown_path); !markdown.empty()) {
      markdown_ = markdown;
    }
    if (fs::is_regular_file(attributes_path)) {
      const auto attributes = ReadJsonFileToValue(attributes_path);
      ApplyAttributes(attributes);
    }
    return true;
  }

  std::vector<MockMarkdownFrameStep> BuildFrameSteps() const {
    if (!frame_steps_.empty()) {
      return frame_steps_;
    }
    std::vector<MockMarkdownFrameStep> steps;
    steps.reserve(frame_step_count_);
    for (int32_t i = 0; i < frame_step_count_; i++) {
      MockMarkdownFrameStep step;
      step.interval_ms_ = frame_step_interval_ms_;
      if (i < static_cast<int32_t>(frame_visible_rects_.size())) {
        step.has_visible_rect_ = true;
        step.visible_rect_ = frame_visible_rects_[i];
      } else if (has_frame_visible_rect_) {
        step.has_visible_rect_ = true;
        step.visible_rect_ = frame_visible_rect_;
      }
      steps.push_back(step);
    }
    return steps;
  }

  rapidjson::Document RunViewCase() {
    auto main_view = std::make_unique<MockMarkdownMainView>();
    auto* view_ptr = main_view->GetMarkdownView();
    resource_loader_->SetMainView(main_view.get());

    view_ptr->SetResourceLoader(resource_loader_.get());
    view_ptr->SetSourceType(source_type_);
    view_ptr->SetStyle(style_map_);
    view_ptr->SetContent(markdown_);
    view_ptr->SetContentComplete(content_complete_);
    if (max_lines_ >= 0) {
      view_ptr->SetTextMaxLines(max_lines_);
    }
    if (attachments_ != nullptr) {
      view_ptr->SetTextAttachments(std::move(attachments_));
    }
    view_ptr->SetAnimationType(typewriter_ ? MarkdownAnimationType::kTypewriter
                                           : MarkdownAnimationType::kNone);
    view_ptr->SetInitialAnimationStep(animation_step_);
    view_ptr->SetAnimationVelocity(static_cast<float>(animation_velocity_));

    MeasureSpec spec;
    spec.width_ = width_;
    spec.width_mode_ = tttext::LayoutMode::kDefinite;
    spec.height_ = height_;
    spec.height_mode_ = tttext::LayoutMode::kIndefinite;

    MockMarkdownFrameDriver driver(main_view.get(), canvas_.get());
    driver.SetMeasureSpec(spec);
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
    const auto result = ConvertJson(frames);
    if (generate_ground_truth_) {
      std::ofstream output(ground_truth_path);
      output << SerializeJson(frames);
      output.flush();
      output.close();
    } else if (fs::is_regular_file(ground_truth_path)) {
      const auto ground_truth = ReadJsonFileToValue(ground_truth_path);
      ExpectValue(result, ground_truth);
    } else {
      return;
    }
    printf("end case:%s\n", case_name.c_str());
  }

  static void MergeMap(ValueMap& dst, ValueMap& src) {
    for (auto& [key, value] : src) {
      auto dst_v = dst.find(key);
      if (dst_v == dst.end() || dst_v->second->GetType() != ValueType::kMap) {
        dst[key] = std::move(value);
      } else {
        MergeMap(dst_v->second->AsMap(), value->AsMap());
      }
    }
  }

  void ExpectCanvas(const ValuePtr& ground_truth) const {
    const auto result = ConvertJson(canvas_->GetJson());
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
        EXPECT_FLOAT_EQ(result->AsDouble(), truth->AsDouble());
        break;
      case ValueType::kString:
        EXPECT_EQ(result->AsString(), truth->AsString());
        break;
    }
  }

  std::unique_ptr<MockMarkdownResourceLoader> resource_loader_;
  std::unique_ptr<MarkdownDocument> document_;
  std::unique_ptr<MockMarkdownCanvas> canvas_;
  std::string markdown_;
  ValueMap style_map_;

  float width_ = 0;
  float height_ = 0;
  int32_t max_lines_ = -1;
  bool typewriter_ = false;
  bool use_char_based_drawer_ = false;
  bool draw_cursor_if_complete_ = false;
  bool content_complete_ = true;
  int32_t animation_step_ = 0;
  double animation_velocity_ = 0;
  std::unique_ptr<Value> attachments_ = nullptr;
  bool region_view_ = false;
  RectF region_rect_;
  SourceType source_type_ = SourceType::kMarkdown;

  int32_t frame_step_count_ = 1;
  int64_t frame_step_interval_ms_ = 16;
  bool has_frame_visible_rect_ = false;
  RectF frame_visible_rect_;
  std::vector<RectF> frame_visible_rects_;
  std::vector<MockMarkdownFrameStep> frame_steps_;

  bool generate_ground_truth_ = false;
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
      unittest.canvas_.get(), 1000, unittest.resource_loader_.get(),
      unittest.document_->GetStyle().typewriter_cursor_, false, nullptr);
  typewriter_drawer.DrawPage(*unittest.document_->GetPage());
  EXPECT_EQ(typewriter_drawer.GetMaxDrawHeight(), 115);
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
}  // namespace testing
}  // namespace markdown
}  // namespace lynx
