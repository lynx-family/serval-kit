// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_paragraph.h"

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/layout/markdown_layout_table.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_canvas.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"

namespace serval::markdown {
namespace {

std::unique_ptr<tttext::Paragraph> MakeText(const char* content) {
  auto paragraph = tttext::Paragraph::Create();
  tttext::Style style;
  style.SetTextSize(10);
  paragraph->AddTextRun(&style, content);
  return paragraph;
}

class TypewriterCursor : public MarkdownDrawable {
 public:
  explicit TypewriterCursor(float width = 2) : width_(width) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    draw_count++;
    auto painter = canvas->CreatePainter();
    canvas->DrawRect(x, y, x + GetAdvance(), y + GetDescent() - GetAscent(),
                     painter.get());
  }

  int draw_count{0};

 protected:
  MeasureResult OnMeasure(MeasureSpec) override {
    return {.width_ = width_, .height_ = 6, .baseline_ = 4};
  }

 private:
  float width_;
};

class MarkdownLayoutTypewriterTest : public ::testing::Test {
 protected:
  MarkdownLayoutNode::LayoutResult Layout(MarkdownLayoutNode* node,
                                          float width = 200) {
    auto result = node->Layout({.width = width,
                                .height = 200,
                                .height_mode = tttext::LayoutMode::kAtMost});
    node->Align({});
    return result;
  }

  std::string Render(MarkdownLayoutNode* node, int32_t char_count,
                     MarkdownDrawable* cursor = nullptr) {
    canvas_.StartPaint();
    node->Render(
        {.canvas = &canvas_, .max_char_count = char_count, .cursor = cursor});
    canvas_.EndPaint();
    std::string text;
    for (const auto& batch : canvas_.GetJson().GetArray()) {
      for (const auto& op : batch.GetArray()) {
        if (std::string(op["op"].GetString()) == "glyphs") {
          text += op["text"].GetString();
        }
      }
    }
    return text;
  }

  const rapidjson::Value& LastRect() const {
    const auto& batch = canvas_.GetJson()[0];
    return batch[batch.Size() - 1]["rect"];
  }

  std::shared_ptr<MarkdownContext> context_ =
      testing::CreateTestMarkdownSharedContext();
  testing::MockMarkdownResourceLoader loader_;
  testing::MockMarkdownCanvas canvas_{&loader_};
};

TEST_F(MarkdownLayoutTypewriterTest, RevealsUnicodeCharactersAcrossFrames) {
  MarkdownLayoutParagraph paragraph(context_.get(), MakeText("你好abc"));
  Layout(&paragraph);
  const std::vector<std::string> frames = {"",      "你",     "你好",
                                           "你好a", "你好ab", "你好abc"};
  for (int32_t count = 0; count < frames.size(); count++) {
    SCOPED_TRACE(count);
    EXPECT_EQ(Render(&paragraph, count), frames[count]);
  }
  EXPECT_EQ(Render(&paragraph, 20), "你好abc");
  EXPECT_EQ(Render(&paragraph, 1), "你");
}

TEST_F(MarkdownLayoutTypewriterTest, SpendsBudgetAcrossLinesAndParagraphs) {
  MarkdownLayoutNode root(context_.get());
  MarkdownLayoutParagraph first(context_.get(), MakeText("abcd"));
  MarkdownLayoutParagraph second(context_.get(), MakeText("XYZ"));
  root.AppendChild(&first);
  root.AppendChild(&second);
  Layout(&root, 20);
  ASSERT_GT(first.GetLineCount(), 1);
  const std::string content = "abcdXYZ";
  for (int32_t count = 0; count <= content.size(); count++) {
    SCOPED_TRACE(count);
    EXPECT_EQ(Render(&root, count), content.substr(0, count));
  }
}

TEST_F(MarkdownLayoutTypewriterTest, HidesUnstartedParagraphBackground) {
  MarkdownLayoutNode root(context_.get());
  MarkdownLayoutParagraph first(context_.get(), MakeText("ab"));
  MarkdownLayoutParagraph second(context_.get(), MakeText("cd"));
  second.SetBackground({static_cast<int32_t>(0xff123456), nullptr});
  root.AppendChild(&first);
  root.AppendChild(&second);
  Layout(&root);
  EXPECT_EQ(Render(&root, 0), "");
  EXPECT_TRUE(canvas_.GetJson().Empty());
  EXPECT_EQ(Render(&root, 2), "ab");
  for (const auto& op : canvas_.GetJson()[0].GetArray()) {
    if (std::string(op["op"].GetString()) == "rect") {
      EXPECT_NE(op["painter"]["fill_color"].GetUint(), 0xff123456u);
    }
  }
}

TEST_F(MarkdownLayoutTypewriterTest, DrawsOneCursorThroughNestedParagraphs) {
  MarkdownLayoutNode root(context_.get());
  MarkdownLayoutNode group(context_.get());
  MarkdownLayoutParagraph first(context_.get(), MakeText("ab"));
  MarkdownLayoutParagraph second(context_.get(), MakeText("cd"));
  first.SetMargins({7, 5, 0, 0});
  first.SetPaddings({2, 3, 0, 0});
  group.AppendChild(&first);
  group.AppendChild(&second);
  root.AppendChild(&group);
  Layout(&root);
  TypewriterCursor cursor;
  for (int32_t count = 0; count <= 4; count++) {
    SCOPED_TRACE(count);
    cursor.draw_count = 0;
    EXPECT_EQ(Render(&root, count, &cursor),
              std::string("abcd").substr(0, count));
    ASSERT_EQ(cursor.draw_count, 1);
    if (count < 2) {
      EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), 9 + count * 10);
      EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(), 14);
    } else {
      EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), (count - 2) * 10);
      EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(),
                      second.GetLayoutRect().GetTop() + 6);
    }
  }
  cursor.draw_count = 0;
  EXPECT_EQ(Render(&root, 20, &cursor), "abcd");
  ASSERT_EQ(cursor.draw_count, 1);
  EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), 20);
  EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(),
                  second.GetLayoutRect().GetTop() + 6);
}

TEST_F(MarkdownLayoutTypewriterTest, PositionsCursorAtNextLineCharacter) {
  MarkdownLayoutParagraph paragraph(context_.get(), MakeText("abcd"));
  paragraph.SetPaddings({2, 3, 2, 0});
  Layout(&paragraph, 24);
  TypewriterCursor cursor;
  EXPECT_EQ(Render(&paragraph, 2, &cursor), "ab");
  ASSERT_EQ(cursor.draw_count, 1);
  std::vector<RectF> rects;
  paragraph.GetSelectionRectByCharPos(
      &rects, 2, 3, MarkdownLayoutNode::RectType::kCharBounding,
      MarkdownLayoutNode::RectCoordinate::kAbsolute);
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), rects[0].GetLeft());
  EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(), rects[0].GetTop() + 6);
}

TEST_F(MarkdownLayoutTypewriterTest, KeepsWideCursorAtNextCharacter) {
  MarkdownLayoutParagraph paragraph(context_.get(), MakeText("ab"));
  Layout(&paragraph, 20);
  TypewriterCursor cursor(16);
  EXPECT_EQ(Render(&paragraph, 1, &cursor), "a");
  ASSERT_EQ(cursor.draw_count, 1);
  EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), 10);
  EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(), 6);
}

TEST_F(MarkdownLayoutTypewriterTest, KeepsCompletedCursorAfterLastCharacter) {
  MarkdownLayoutParagraph paragraph(context_.get(), MakeText("ab"));
  Layout(&paragraph, 20);
  TypewriterCursor cursor;
  for (int count : {2, 20}) {
    SCOPED_TRACE(count);
    cursor.draw_count = 0;
    EXPECT_EQ(Render(&paragraph, count, &cursor), "ab");
    ASSERT_EQ(cursor.draw_count, 1);
    EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), 20);
    EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(), 6);
  }
}

TEST_F(MarkdownLayoutTypewriterTest, RevealsOnlyStartedTableRows) {
  MarkdownLayoutTable table(context_.get());
  MarkdownLayoutTableRow rows[] = {MarkdownLayoutTableRow(context_.get()),
                                   MarkdownLayoutTableRow(context_.get()),
                                   MarkdownLayoutTableRow(context_.get())};
  MarkdownLayoutTableCell cells[] = {MarkdownLayoutTableCell(context_.get()),
                                     MarkdownLayoutTableCell(context_.get()),
                                     MarkdownLayoutTableCell(context_.get())};
  MarkdownLayoutParagraph paragraphs[] = {
      MarkdownLayoutParagraph(context_.get(), MakeText("abcde")),
      MarkdownLayoutParagraph(context_.get(), MakeText("fghij")),
      MarkdownLayoutParagraph(context_.get(), MakeText("klmno"))};
  for (int i = 0; i < 3; i++) {
    cells[i].AppendChild(&paragraphs[i]);
    rows[i].AppendChild(&cells[i]);
    table.AppendChild(&rows[i]);
  }
  Layout(&table);
  for (int count : {1, 5, 6, 10, 11, 15, 20}) {
    SCOPED_TRACE(count);
    EXPECT_EQ(Render(&table, count),
              std::string("abcdefghijklmno").substr(0, count));
    const auto& clip = canvas_.GetJson()[0][0];
    ASSERT_STREQ(clip["op"].GetString(), "clip");
    int last_row = count <= 5 ? 0 : (count <= 10 ? 1 : 2);
    EXPECT_FLOAT_EQ(clip["rect"]["bottom"].GetFloat(),
                    rows[last_row].GetLayoutRect().GetBottom());
  }
  TypewriterCursor cursor;
  for (int count : {0, 5, 10, 15, 20}) {
    SCOPED_TRACE(count);
    cursor.draw_count = 0;
    EXPECT_EQ(Render(&table, count, &cursor),
              std::string("abcdefghijklmno").substr(0, count));
    ASSERT_EQ(cursor.draw_count, 1);
    const auto& clip = canvas_.GetJson()[0][0];
    ASSERT_STREQ(clip["op"].GetString(), "clip");
    int last_row = count < 5 ? 0 : (count < 10 ? 1 : 2);
    EXPECT_FLOAT_EQ(clip["rect"]["bottom"].GetFloat(),
                    rows[last_row].GetLayoutRect().GetBottom());
    EXPECT_FLOAT_EQ(LastRect()["left"].GetFloat(), count < 15 ? 0 : 50);
    EXPECT_FLOAT_EQ(LastRect()["top"].GetFloat(),
                    rows[last_row].GetLayoutRect().GetTop() + 6);
  }
}

}  // namespace
}  // namespace serval::markdown
