// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <fstream>

#include "gtest/gtest.h"
#include "markdown/draw/markdown_drawer.h"
#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/impl/markdown_parser_impl.h"
#include "markdown/style/markdown_style_reader.h"
#include "testing/markdown/mock_markdown_canvas.h"
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

class MarkdownParserUnittest {
 public:
  MarkdownParserUnittest() {
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
        draw_cursor_if_complete_ = !(iter->second->AsBool());
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
        auto& array = iter->second->AsArray();
        if (array.size() >= 4) {
          float left = array[0]->GetDouble();
          float top = array[1]->GetDouble();
          float right = array[2]->GetDouble();
          float bottom = array[3]->GetDouble();
          region_rect_ = RectF::MakeLTRB(left, top, right, bottom);
        }
      }
    }
  }

  void RunCaseInDirectory(const fs::path& path) {
    auto case_name = path.filename();
    printf("running case:%s\n", case_name.c_str());

    const auto attributes_path = path / "attributes.json";
    const auto markdown_path = path / "markdown.md";
    const auto ground_truth_path = path / "ground_truth.json";

    if (fs::is_regular_file(markdown_path)) {
      if (auto markdown = ReadFileToString(markdown_path); !markdown.empty()) {
        markdown_ = markdown;
      }
    } else {
      return;
    }

    if (fs::is_regular_file(attributes_path)) {
      const auto attributes = ReadJsonFileToValue(attributes_path);
      ApplyAttributes(attributes);
    }
    ParseLayoutAndDraw();
    if (generate_ground_truth_) {
      std::ofstream output(ground_truth_path);
      output << canvas_->GetResult();
      output.flush();
      output.close();
    } else if (fs::is_regular_file(ground_truth_path)) {
      const auto ground_truth = ReadJsonFileToValue(ground_truth_path);
      ExpectCanvas(ground_truth);
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
  int32_t animation_step_ = 0;
  double animation_velocity_ = 0;
  std::unique_ptr<Value> attachments_ = nullptr;
  bool region_view_ = false;
  RectF region_rect_;

  bool generate_ground_truth_ = false;
};
TEST(MarkdownParserUnittest, Cases) {
  for (const auto& test_case : fs::directory_iterator(CASES_PATH)) {
    if (test_case.is_directory()) {
      MarkdownParserUnittest unittest;
      unittest.RunCaseInDirectory(test_case);
    }
  }
}

TEST(MarkdownParserUnittest, SimpleTextTest) {
  MarkdownParserUnittest unittest;
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

TEST(MarkdownParserUnittest, GetContentByCharPosLineBreakTest) {
  MarkdownParserUnittest unittest;
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
TEST(MarkdownParserUnittest, ContentRangeCrash) {
  MarkdownParserUnittest unittest;
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
TEST(MarkdownParserUnittest, GetLinkByTouchPosition) {
  MarkdownParserUnittest unittest;
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
TEST(MarkdownParserUnittest, TypewriterDynamicHeight) {
  MarkdownParserUnittest unittest;
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
TEST(MarkdownParserUnittest, SelectionGetContent) {
  MarkdownParserUnittest unittest;
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
TEST(MarkdownParserUnittest, EmptyContent) {
  MarkdownParserUnittest unittest;
  unittest.markdown_ = R"(
- ####
)";
  unittest.ParseLayoutAndDraw();
}
TEST(MarkdownParserUnittest, OffsetConvert) {
  MarkdownParserUnittest unittest;
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

TEST(MarkdownParserUnittest, DrawAttachment) {
  MarkdownParserUnittest unittest;
  unittest.RunCaseInDirectory(CASES_PATH / "template");
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

TEST(MarkdownParserUnittest, MarkAttachments) {
  MarkdownParserUnittest unittest;
  unittest.RunCaseInDirectory(CASES_PATH / "text_attachments_mark");
}
}  // namespace testing
}  // namespace markdown
}  // namespace lynx
