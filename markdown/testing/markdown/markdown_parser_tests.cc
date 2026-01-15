// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <fstream>

#include "gtest/gtest.h"
#include "markdown/draw/markdown_drawer.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/parser/markdown_parser.h"
#include "markdown/style/markdown_style_reader.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_resource_loader.h"
#define GROUND_TRUTH_PATH "testing/markdown/ground_truth"
namespace lynx {
namespace markdown {
namespace testing {
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
  doc.Parse(json.c_str());
  return ConvertJson(doc);
}
class MarkdownParserTests : public ::testing::Test {
 protected:
  void SetUp() override {
    resource_loader_ = std::make_unique<MockMarkdownResourceLoader>();
    document_ = std::make_unique<MarkdownDocument>(resource_loader_.get());
    canvas_ = std::make_unique<MockMarkdownCanvas>(resource_loader_.get());
  }
  void TearDown() override {}
  void ParseLayoutAndDraw(float width, float height = 1e6) {
    Parse(width);
    Layout(width, height);
    Draw();
  }
  void Parse(float max_width) {
    if (!markdown_.empty()) {
      auto style =
          MarkdownStyleReader::ReadStyle(style_map_, resource_loader_.get());
      document_->SetMarkdownContent(markdown_);
      document_->SetStyle(style);
      document_->SetMaxSize(max_width, 0);
      document_->SetMarkdownContentRange(content_range_);
      MarkdownParser::Parse(document_.get());
    }
  }
  void Layout(float width, float height, int32_t max_line = -1) {
    MarkdownLayout layout(document_.get());
    layout.Layout(width, height, max_line);
  }
  void Draw() {
    MarkdownDrawer drawer(canvas_.get());
    drawer.DrawPage(*document_->GetPage());
  }

  void ExpectArray(const rapidjson::Value& result,
                   const rapidjson::Value& expect) {
    EXPECT_EQ(result.Size(), expect.Size());
    for (uint32_t i = 0; i < result.Size(); i++) {
      ExpectValue(result.GetArray()[i], expect.GetArray()[i]);
    }
  }
  void ExpectObject(const rapidjson::Value& result,
                    const rapidjson::Value& expect) {
    for (auto& [key, var] : result.GetObject()) {
      auto& expect_var = expect.GetObject()[key.GetString()];
      ExpectValue(var, expect_var);
    }
  }
  void ExpectValue(const rapidjson::Value& result,
                   const rapidjson::Value& expect) {
    EXPECT_EQ(result.GetType(), expect.GetType());
    switch (result.GetType()) {
      case rapidjson::kNullType:
        break;
      case rapidjson::kFalseType:
        break;
      case rapidjson::kTrueType:
        break;
      case rapidjson::kObjectType:
        ExpectObject(result, expect);
        break;
      case rapidjson::kArrayType:
        ExpectArray(result, expect);
        break;
      case rapidjson::kStringType:
        EXPECT_EQ(result, expect);
        break;
      case rapidjson::kNumberType:
        EXPECT_TRUE(std::abs(result.GetDouble() - expect.GetDouble()) < 1e-4);
        break;
    }
  }
  void SetDefaultStyle() {
    ValueMap ut;
    ut["marginBottom"] = Value::MakeDouble(10);
    ut["lineSpace"] = Value::MakeDouble(0);
    style_map_["normalText"] = Value::MakeMap(std::move(ut));
    ValueMap cb;
    cb["marginBottom"] = Value::MakeDouble(10);
    cb["maxWidth"] = Value::MakeDouble(500);
    cb["scrollX"] = Value::MakeBool(true);
    style_map_["codeBlock"] = Value::MakeMap(std::move(cb));
    ValueMap qt;
    qt.insert({"marginTop", Value::MakeDouble(15)});
    qt.insert({"marginBottom", Value::MakeDouble(15)});
    qt.insert({"paragraphSpace", Value::MakeDouble(20)});
    qt.insert({"borderColor", Value::MakeString("14161823")});
    style_map_.insert({"quote", Value::MakeMap(std::move(qt))});
    ValueMap tb;
    tb.insert({"marginBottom", Value::MakeDouble(10)});
    tb.insert({"scrollX", Value::MakeBool(true)});
    style_map_.insert({"table", Value::MakeMap(std::move(tb))});
    ValueMap tc;
    tc.insert({"fontSize", Value::MakeDouble(15)});
    tc.insert({"maxWidth", Value::MakeDouble(200)});
    style_map_.insert({"tableCell", Value::MakeMap(std::move(tc))});
    ValueMap ic;
    ic.insert({"borderRadius", Value::MakeDouble(4)});
    ic.insert({"paddingLeft", Value::MakeDouble(4)});
    ic.insert({"paddingRight", Value::MakeDouble(4)});
    style_map_.insert({"inlineCode", Value::MakeMap(std::move(ic))});
    ValueMap ul;
    ul.insert({"markSize", Value::MakeDouble(4)});
    ul.insert({"markMarginRight", Value::MakeDouble(10)});
    ul.insert({"paragraphSpace", Value::MakeDouble(10)});
    style_map_.insert({"unorderedList", Value::MakeMap(std::move(ul))});
    ValueMap ol;
    ol.insert({"paragraphSpace", Value::MakeDouble(10)});
    style_map_.insert({"orderedList", Value::MakeMap(std::move(ol))});
    ValueMap h2;
    h2.insert({"fontWeight", Value::MakeString("normal")});
    style_map_.insert({"h2", Value::MakeMap(std::move(h2))});
    ValueMap db;
    db.insert({"backgroundImage",
               Value::MakeString("linear-gradient(0deg, #e4ddff 0%, "
                                 "rgba(241, 240, 255, 0) 30%)")});
    db.insert({"fontWeight", Value::MakeString("bold")});
    db.insert({"marginBottom", Value::MakeDouble(20)});
    db.insert({"fontSize", Value::MakeDouble(-1)});
    style_map_.insert({"doubleBraces", Value::MakeMap(std::move(db))});
    ValueMap mk;
    mk.insert({"backgroundImage",
               Value::MakeString("linear-gradient(0deg, #14fdff 0%, "
                                 "rgba(241, 240, 255, 0) 30%)")});
    mk.insert({"fontSize", Value::MakeDouble(-1)});
    style_map_.insert({"mark", Value::MakeMap(std::move(mk))});
    ValueMap lk;
    lk.insert({"color", Value::MakeString("0000ff")});
    lk.insert({"textDecorationLine", Value::MakeString("underline")});
    lk.insert({"textDecorationStyle", Value::MakeString("dashed")});
    lk.insert({"textDecorationColor", Value::MakeString("ff0000")});
    lk.insert({"textDecorationThickness", Value::MakeDouble(1)});
    style_map_.insert({"link", Value::MakeMap(std::move(lk))});
    ValueMap th;
    style_map_.insert({"tableHeader", Value::MakeMap(std::move(lk))});
    ValueMap qbl;
    style_map_.insert({"quoteBorderLine", Value::MakeMap(std::move(qbl))});
    ValueMap ulm;
    style_map_.insert({"unorderedListMarker", Value::MakeMap(std::move(ulm))});
    ValueMap im;
    style_map_.insert({"image", Value::MakeMap(std::move(im))});
  }
  void ExpectCanvasResult(const std::string& filename, bool generate = false) {
    auto result = canvas_->GetResult();
    printf("result: \n%s\n", canvas_->GetResult().c_str());
    if (!generate) {
      std::ifstream input(std::string(GROUND_TRUTH_PATH) + "/" + filename);
      std::string str((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
      input.close();
      rapidjson::Document expect;
      expect.Parse(str.c_str());
      ExpectValue(canvas_->GetJson(), expect);
    } else {
      std::ofstream output(std::string(GROUND_TRUTH_PATH) + "/" + filename);
      output << result;
      output.flush();
      output.close();
    }
  }
  std::unique_ptr<MockMarkdownResourceLoader> resource_loader_;
  std::unique_ptr<MarkdownDocument> document_;
  std::unique_ptr<MockMarkdownCanvas> canvas_;
  std::string markdown_;
  ValueMap style_map_;
  MarkdownParserType parser_type_{MarkdownParserType::kDiscount};
  Range content_range_{0, std::numeric_limits<int32_t>::max()};
};

TEST_F(MarkdownParserTests, SimpleTextTest) {
  markdown_ = "simple text";
  Parse(100);
  auto& paras = document_->GetParagraphs();
  EXPECT_EQ(paras.size(), 1ull);
  auto element = paras.front();
  EXPECT_EQ(element->GetType(), MarkdownElementType::kParagraph);
  auto* para =
      static_cast<MarkdownParagraphElement*>(element.get())->GetParagraph();
  auto parsed_content = para->GetContentString(0, para->GetCharCount());
  EXPECT_EQ(markdown_, parsed_content);
}
}  // namespace testing
}  // namespace markdown
}  // namespace lynx
