// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/layout/markdown_layout_node.h"

#include <memory>

#include "gtest/gtest.h"
#include "markdown/layout/markdown_layout_list_item.h"
#include "markdown/layout/markdown_layout_paragraph.h"
#include "markdown/layout/markdown_layout_quote.h"
#include "markdown/layout/markdown_layout_table.h"
#include "markdown/style/markdown_style.h"
#include "testing/markdown/markdown_tests_platform.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_resource_loader.h"

namespace serval::markdown {
namespace {

BorderSide MakeBorderSide(MarkdownBorderType type, int32_t color, float width,
                          float radius = 0) {
  BorderSide side{};
  side.type_ = type;
  side.color_ = color;
  side.width_ = width;
  side.radius_top_ = radius;
  side.radius_bottom_ = radius;
  return side;
}

std::unique_ptr<tttext::Paragraph> MakeParagraph(const char* content,
                                                 float font_size = 10) {
  auto paragraph = tttext::Paragraph::Create();
  tttext::Style style;
  style.SetTextSize(font_size);
  paragraph->AddTextRun(&style, content);
  return paragraph;
}

class TestMarker : public MarkdownDrawable {
 public:
  TestMarker(float width, float height, float baseline)
      : width_(width), height_(height), baseline_(baseline) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    auto painter = canvas->CreatePainter();
    canvas->DrawRect(x, y, x + width_, y + height_, painter.get());
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {.width_ = width_, .height_ = height_, .baseline_ = baseline_};
  }

 private:
  float width_;
  float height_;
  float baseline_;
};

class TestLayoutNode : public MarkdownLayoutNode {
 public:
  explicit TestLayoutNode(bool leaf = false,
                          LayoutResult desired_result = LayoutResult{},
                          std::string content = "")
      : MarkdownLayoutNode(nullptr),
        leaf_(leaf),
        desired_result_(desired_result),
        content_(std::move(content)) {}

  const LayoutParams& LastLayoutParams() const { return last_layout_params_; }
  const AlignParams& LastAlignParams() const { return last_align_params_; }
  const RectF& LayoutRect() const { return layout_rect_; }
  int RenderCount() const { return render_count_; }
  int ForceAddTruncationCallCount() const {
    return force_add_truncation_call_count_;
  }

  Range QueryCharRange(PointF point, CharRangeType type) {
    return GetCharRangeByPoint(point, type);
  }

  bool CallForceAddTruncation() { return ForceAddTruncation(); }

  void SetForceAddTruncationResult(bool result) {
    force_add_truncation_result_ = result;
  }

  void SetLineEndCharIndices(std::vector<int32_t> indices) {
    line_end_char_indices_ = std::move(indices);
  }

  void GetSelectionRectByCharPos(std::vector<RectF>* result,
                                 int32_t char_pos_start, int32_t char_pos_end,
                                 RectType type,
                                 RectCoordinate coordinate) override {
    if (!leaf_) {
      MarkdownLayoutNode::GetSelectionRectByCharPos(
          result, char_pos_start, char_pos_end, type, coordinate);
      return;
    }
    result->emplace_back(static_cast<float>(char_pos_start),
                         static_cast<float>(char_pos_end), 1, 1);
  }

  void GetContentByCharPos(std::string* result, int32_t char_pos_start,
                           int32_t char_pos_end) override {
    if (!leaf_) {
      MarkdownLayoutNode::GetContentByCharPos(result, char_pos_start,
                                              char_pos_end);
      return;
    }
    result->append(content_);
  }

  void GetLineEndCharIndices(std::vector<int32_t>* result) override {
    if (!leaf_) {
      MarkdownLayoutNode::GetLineEndCharIndices(result);
      return;
    }
    result->insert(result->end(), line_end_char_indices_.begin(),
                   line_end_char_indices_.end());
  }

  Range GetCharRangeByPoint(PointF point, CharRangeType type) override {
    if (!leaf_) {
      return MarkdownLayoutNode::GetCharRangeByPoint(point, type);
    }
    return {static_cast<int32_t>(point.x_), static_cast<int32_t>(point.y_)};
  }

 protected:
  LayoutResult OnLayout(LayoutParams params) override {
    last_layout_params_ = params;
    if (!leaf_) {
      return MarkdownLayoutNode::OnLayout(params);
    }

    auto result = desired_result_;
    if (params.height_mode == tttext::LayoutMode::kDefinite &&
        params.height < desired_result_.height) {
      result.overflow = true;
      result.truncated = false;
    }
    return result;
  }

  void OnAlign(AlignParams params) override {
    last_align_params_ = params;
    MarkdownLayoutNode::OnAlign(params);
  }

  void OnRender(RenderParams params) override {
    if (!leaf_) {
      MarkdownLayoutNode::OnRender(params);
      return;
    }

    render_count_++;
    auto painter = params.canvas->CreatePainter();
    params.canvas->DrawLine(0, 0, desired_result_.width, desired_result_.height,
                            painter.get());
  }

  bool ForceAddTruncation() override {
    if (!leaf_) {
      return MarkdownLayoutNode::ForceAddTruncation();
    }
    force_add_truncation_call_count_++;
    return force_add_truncation_result_;
  }

 private:
  bool leaf_{false};
  LayoutResult desired_result_{};
  std::string content_;
  std::vector<int32_t> line_end_char_indices_;
  LayoutParams last_layout_params_{};
  AlignParams last_align_params_{};
  int render_count_{0};
  bool force_add_truncation_result_{false};
  int force_add_truncation_call_count_{0};
};

}  // namespace

TEST(MarkdownLayoutNodeTest, AccessorsAndEmptyLayoutUseBoxEdges) {
  TestLayoutNode node;
  node.SetMargins({1, 2, 3, 4});
  node.SetPaddings({5, 6, 7, 8});
  node.SetBackground({static_cast<int32_t>(0xff123456), nullptr});
  const auto border = MakeBorderSide(MarkdownBorderType::kSolid, 0xffabcdef, 2);
  node.SetBorders({border, border, border, border});

  EXPECT_FLOAT_EQ(node.GetMargins().left_, 1);
  EXPECT_FLOAT_EQ(node.GetPaddings().bottom_, 8);
  EXPECT_EQ(node.GetBackground().background_color_, 0xff123456);
  EXPECT_FLOAT_EQ(node.GetBorders().left_.width_, 2);

  auto result = node.Layout({.width = 100, .height = 100});

  EXPECT_FLOAT_EQ(result.width, 16);
  EXPECT_FLOAT_EQ(result.height, 18);
  EXPECT_EQ(result.char_count, 0);
  EXPECT_EQ(result.line_count, 0);
  EXPECT_FALSE(result.overflow);
  EXPECT_FALSE(result.truncated);
}

TEST(MarkdownLayoutNodeTest, AggregatesChildLayoutAndKeepsRemainingHeight) {
  TestLayoutNode parent;
  TestLayoutNode child1(true, {.width = 10,
                               .height = 10,
                               .baseline = 8,
                               .char_count = 1,
                               .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 2, .line_count = 2});
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 3, .line_count = 3});
  child3.SetMargins({2, 0, 3, 0});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);

  auto result = parent.Layout({.width = 100, .height = 40});

  EXPECT_FALSE(result.overflow);
  EXPECT_EQ(result.char_count, 6);
  EXPECT_EQ(result.line_count, 6);
  EXPECT_FLOAT_EQ(child1.LastLayoutParams().height, 40);
  EXPECT_FLOAT_EQ(child2.LastLayoutParams().height, 30);
  EXPECT_FLOAT_EQ(child3.LastLayoutParams().height, 10);
  EXPECT_FLOAT_EQ(result.width, 35);
  EXPECT_FLOAT_EQ(result.height, 40);
  EXPECT_FLOAT_EQ(result.baseline, 8);
  EXPECT_FLOAT_EQ(child3.LayoutRect().GetLeft(), 2);
  EXPECT_FLOAT_EQ(child3.LayoutRect().GetTop(), 30);
}

TEST(MarkdownLayoutNodeTest, HorizontalAxisPlacesChildrenInline) {
  TestLayoutNode parent;
  parent.SetLayoutMainAxis(MarkdownLayoutNode::LayoutMainAxis::kHorizontal);
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 1, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 5, .char_count = 2, .line_count = 2});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  auto result = parent.Layout({.width = 100, .height = 50});

  EXPECT_EQ(result.char_count, 3);
  EXPECT_EQ(result.line_count, 2);
  EXPECT_FLOAT_EQ(result.width, 30);
  EXPECT_FLOAT_EQ(result.height, 10);
  EXPECT_FLOAT_EQ(child1.LayoutRect().GetLeft(), 0);
  EXPECT_FLOAT_EQ(child2.LayoutRect().GetLeft(), 10);
  EXPECT_FLOAT_EQ(child2.LayoutRect().GetTop(), 0);
}

TEST(MarkdownLayoutNodeTest, HorizontalAxisUsesContentBoxForChildConstraints) {
  TestLayoutNode parent;
  parent.SetLayoutMainAxis(MarkdownLayoutNode::LayoutMainAxis::kHorizontal);
  parent.SetPaddings({2, 3, 4, 5});
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 1, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 5, .char_count = 2, .line_count = 2});
  child1.SetMargins({1, 2, 3, 4});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  auto result = parent.Layout({.width = 100, .height = 50});

  EXPECT_FLOAT_EQ(child1.LastLayoutParams().width, 90);
  EXPECT_FLOAT_EQ(child1.LastLayoutParams().height, 36);
  EXPECT_FLOAT_EQ(child2.LastLayoutParams().width, 80);
  EXPECT_FLOAT_EQ(child2.LastLayoutParams().height, 42);
  EXPECT_FLOAT_EQ(child1.LayoutRect().GetLeft(), 3);
  EXPECT_FLOAT_EQ(child1.LayoutRect().GetTop(), 5);
  EXPECT_FLOAT_EQ(child2.LayoutRect().GetLeft(), 16);
  EXPECT_FLOAT_EQ(child2.LayoutRect().GetTop(), 3);
  EXPECT_FLOAT_EQ(result.width, 40);
  EXPECT_FLOAT_EQ(result.height, 24);
}

TEST(MarkdownLayoutNodeTest, ClearsStaleLayoutAndPropagatesOverflow) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 1, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 2, .line_count = 2});
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 3, .line_count = 3});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);

  auto first_result = parent.Layout({.width = 100, .height = 40});
  ASSERT_FALSE(first_result.overflow);
  ASSERT_TRUE(child1.HasLayout());
  ASSERT_TRUE(child2.HasLayout());
  ASSERT_TRUE(child3.HasLayout());

  auto second_result = parent.Layout({.width = 100, .height = 15});

  EXPECT_TRUE(second_result.overflow);
  EXPECT_TRUE(child1.HasLayout());
  EXPECT_FALSE(child2.HasLayout());
  EXPECT_FALSE(child3.HasLayout());
  EXPECT_EQ(second_result.char_count, 1);
  EXPECT_EQ(second_result.line_count, 1);
}

TEST(MarkdownLayoutNodeTest, KeepsTruncatedOverflowChildInLayout) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 1, .line_count = 1});
  TestLayoutNode child2(true, {.width = 20,
                               .height = 20,
                               .char_count = 2,
                               .line_count = 2,
                               .overflow = true,
                               .truncated = true});
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 3, .line_count = 3});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);

  auto result = parent.Layout({.width = 100, .height = 100});

  EXPECT_TRUE(result.overflow);
  EXPECT_TRUE(result.truncated);
  EXPECT_EQ(result.char_count, 3);
  EXPECT_EQ(result.line_count, 3);
  EXPECT_TRUE(child1.HasLayout());
  EXPECT_TRUE(child2.HasLayout());
  EXPECT_FALSE(child3.HasLayout());
}

TEST(MarkdownLayoutNodeTest, AlignPropagatesPositionAndCharOffset) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 2, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 3, .line_count = 2});
  child1.SetMargins({2, 3, 0, 0});
  child2.SetMargins({4, 5, 0, 0});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.Layout({.width = 100, .height = 100});

  parent.Align({.absolute_position = {10, 20}, .absolute_char_pos = 7});

  EXPECT_FLOAT_EQ(child1.LastAlignParams().absolute_position.x_, 12);
  EXPECT_FLOAT_EQ(child1.LastAlignParams().absolute_position.y_, 23);
  EXPECT_EQ(child1.LastAlignParams().absolute_char_pos, 7);
  EXPECT_FLOAT_EQ(child2.LastAlignParams().absolute_position.x_, 14);
  EXPECT_FLOAT_EQ(child2.LastAlignParams().absolute_position.y_, 38);
  EXPECT_EQ(child2.LastAlignParams().absolute_char_pos, 9);
}

TEST(MarkdownLayoutNodeTest,
     SelectionContentAndLineQueriesDispatchByCharRange) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 2, .line_count = 1}, "A");
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 3, .line_count = 2}, "B");
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 4, .line_count = 3}, "C");
  child1.SetLineEndCharIndices({2});
  child2.SetLineEndCharIndices({4, 5});
  child3.SetLineEndCharIndices({9});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);
  parent.Layout({.width = 100, .height = 100});
  parent.Align({.absolute_position = {0, 0}, .absolute_char_pos = 10});

  std::vector<RectF> selection_rects;
  parent.GetSelectionRectByCharPos(
      &selection_rects, 12, 15, MarkdownLayoutNode::RectType::kSelection,
      MarkdownLayoutNode::RectCoordinate::kAbsolute);
  std::string content;
  parent.GetContentByCharPos(&content, 12, 15);
  std::vector<int32_t> line_end_indices;
  parent.GetLineEndCharIndices(&line_end_indices);

  ASSERT_EQ(selection_rects.size(), 1u);
  EXPECT_FLOAT_EQ(selection_rects[0].GetLeft(), 12);
  EXPECT_FLOAT_EQ(selection_rects[0].GetTop(), 15);
  EXPECT_EQ(content, "B");
  EXPECT_EQ(line_end_indices, (std::vector<int32_t>{2, 4, 5, 9}));
}

TEST(MarkdownLayoutNodeTest, ForceAddTruncationSearchesLastLaidOutChildFirst) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 1, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 2, .line_count = 2});
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 3, .line_count = 3});
  child1.SetForceAddTruncationResult(true);
  child2.SetForceAddTruncationResult(true);
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);
  parent.Layout({.width = 100, .height = 35});

  EXPECT_TRUE(parent.CallForceAddTruncation());
  EXPECT_EQ(child3.ForceAddTruncationCallCount(), 0);
  EXPECT_EQ(child2.ForceAddTruncationCallCount(), 1);
  EXPECT_EQ(child1.ForceAddTruncationCallCount(), 0);
}

TEST(MarkdownLayoutNodeTest, RenderAndHitTestUseChildLocalCoordinates) {
  TestLayoutNode parent;
  TestLayoutNode child(
      true, {.width = 11, .height = 13, .char_count = 1, .line_count = 1});
  child.SetMargins({7, 4, 0, 0});
  parent.AppendChild(&child);
  parent.Layout({.width = 100, .height = 100});

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  parent.Render({.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});

  const auto& ops = canvas.GetJson();
  ASSERT_GE(ops.Size(), 2u);
  const auto& child_line = ops[ops.Size() - 1];
  ASSERT_STREQ(child_line["op"].GetString(), "line");
  EXPECT_FLOAT_EQ(child_line["p1"]["x"].GetFloat(), 7);
  EXPECT_FLOAT_EQ(child_line["p1"]["y"].GetFloat(), 4);
  EXPECT_FLOAT_EQ(child_line["p2"]["x"].GetFloat(), 18);
  EXPECT_FLOAT_EQ(child_line["p2"]["y"].GetFloat(), 17);

  auto range =
      parent.QueryCharRange({9, 7}, MarkdownLayoutNode::CharRangeType::kChar);
  EXPECT_EQ(range.start_, 2);
  EXPECT_EQ(range.end_, 3);
}

TEST(MarkdownLayoutNodeTest, RenderStopsAfterVisibleCharBudgetIsExceeded) {
  TestLayoutNode parent;
  TestLayoutNode child1(
      true, {.width = 10, .height = 10, .char_count = 2, .line_count = 1});
  TestLayoutNode child2(
      true, {.width = 20, .height = 20, .char_count = 3, .line_count = 2});
  TestLayoutNode child3(
      true, {.width = 30, .height = 10, .char_count = 4, .line_count = 3});
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);
  parent.Layout({.width = 100, .height = 100});

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  parent.Render({.canvas = &canvas, .max_char_count = 1, .cursor = nullptr});

  EXPECT_EQ(child1.RenderCount(), 1);
  EXPECT_EQ(child2.RenderCount(), 0);
  EXPECT_EQ(child3.RenderCount(), 0);
}

TEST(MarkdownLayoutNodeTest, DrawsRoundBackgroundAndBorder) {
  TestLayoutNode node;
  const auto border =
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff0000ff, 2, 5);
  node.SetBorders({border, border, border, border});
  node.SetBackground({static_cast<int32_t>(0xff00ff00), nullptr});
  node.Layout({.width = 100, .height = 100});

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  node.Render({.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});

  const auto& ops = canvas.GetJson();
  ASSERT_EQ(ops.Size(), 2u);
  ASSERT_STREQ(ops[0]["op"].GetString(), "round rect");
  EXPECT_FLOAT_EQ(ops[0]["rect"]["left"].GetFloat(), 0);
  EXPECT_FLOAT_EQ(ops[0]["rect"]["top"].GetFloat(), 0);
  EXPECT_FLOAT_EQ(ops[0]["rect"]["right"].GetFloat(), 4);
  EXPECT_FLOAT_EQ(ops[0]["rect"]["bottom"].GetFloat(), 4);
  EXPECT_FLOAT_EQ(ops[0]["radius"].GetFloat(), 5);
  ASSERT_STREQ(ops[1]["op"].GetString(), "round rect");
  EXPECT_FLOAT_EQ(ops[1]["rect"]["left"].GetFloat(), 1);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["top"].GetFloat(), 1);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["right"].GetFloat(), 3);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["bottom"].GetFloat(), 3);
}

TEST(MarkdownLayoutNodeTest, DrawsIndependentBorderSides) {
  TestLayoutNode node;
  node.SetBorders({
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff000001, 2),
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff000002, 4),
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff000003, 3),
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff000004, 5),
  });
  node.Layout({.width = 100, .height = 100});

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  node.Render({.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});

  const auto& ops = canvas.GetJson();
  ASSERT_EQ(ops.Size(), 5u);
  ASSERT_STREQ(ops[0]["op"].GetString(), "rect");
  ASSERT_STREQ(ops[1]["op"].GetString(), "line");
  ASSERT_STREQ(ops[2]["op"].GetString(), "line");
  ASSERT_STREQ(ops[3]["op"].GetString(), "line");
  ASSERT_STREQ(ops[4]["op"].GetString(), "line");
  EXPECT_FLOAT_EQ(ops[1]["p1"]["x"].GetFloat(), 1);
  EXPECT_FLOAT_EQ(ops[2]["p1"]["x"].GetFloat(), 3.5);
  EXPECT_FLOAT_EQ(ops[3]["p1"]["y"].GetFloat(), 2);
  EXPECT_FLOAT_EQ(ops[4]["p1"]["y"].GetFloat(), 6.5);
}

TEST(MarkdownLayoutParagraphTest, LayoutSelectionContentAndRender) {
  auto context = testing::CreateTestMarkdownSharedContext();
  MarkdownLayoutParagraph paragraph(context.get(), MakeParagraph("hello"));

  auto result = paragraph.Layout({.width = 200, .height = 100});
  paragraph.Align({.absolute_position = {10, 20}, .absolute_char_pos = 7});

  EXPECT_EQ(result.char_count, 5);
  EXPECT_EQ(result.line_count, 1);
  EXPECT_GT(result.width, 0);
  EXPECT_GT(result.height, 0);

  auto range = paragraph.GetCharRangeByPoint(
      {0.1f, result.baseline}, MarkdownLayoutNode::CharRangeType::kChar);
  EXPECT_EQ(range.start_, 7);
  EXPECT_EQ(range.end_, 8);

  std::string content;
  paragraph.GetContentByCharPos(&content, 7, 12);
  EXPECT_EQ(content, "hello\n");

  std::vector<int32_t> line_end_indices;
  paragraph.GetLineEndCharIndices(&line_end_indices);
  EXPECT_EQ(line_end_indices, (std::vector<int32_t>{12}));

  testing::MockMarkdownResourceLoader loader;
  testing::MockMarkdownCanvas canvas(&loader, nullptr);
  paragraph.Render(
      {.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});
  EXPECT_GT(canvas.GetJson().Size(), 0u);
}

TEST(MarkdownLayoutListItemTest, MarkerOccupiesLeadingColumn) {
  MarkdownLayoutListItem list_item(nullptr);
  list_item.SetMarker(std::make_shared<TestMarker>(6, 8, 6));
  TestLayoutNode child(true, {.width = 20,
                              .height = 10,
                              .baseline = 8,
                              .char_count = 3,
                              .line_count = 1});
  list_item.AppendChild(&child);

  auto result = list_item.Layout({.width = 100, .height = 100});

  EXPECT_EQ(result.char_count, 3);
  EXPECT_FLOAT_EQ(child.LastLayoutParams().width, 94);
  EXPECT_FLOAT_EQ(child.LayoutRect().GetLeft(), 6);
  EXPECT_FLOAT_EQ(result.width, 26);

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  list_item.Render(
      {.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});

  const auto& ops = canvas.GetJson();
  ASSERT_GE(ops.Size(), 3u);
  ASSERT_STREQ(ops[1]["op"].GetString(), "rect");
  EXPECT_FLOAT_EQ(ops[1]["rect"]["left"].GetFloat(), 0);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["top"].GetFloat(), 2);
  ASSERT_STREQ(ops[2]["op"].GetString(), "line");
  EXPECT_FLOAT_EQ(ops[2]["p1"]["x"].GetFloat(), 6);
}

TEST(MarkdownLayoutQuoteTest, DrawsFilledLeftBorder) {
  MarkdownLayoutQuote quote(nullptr);
  MarkdownQuoteBorderLineStyle line_style{};
  line_style.line_.width_ = 4;
  line_style.line_.color_ = 0xff123456;
  line_style.line_.radius_ = 2;
  line_style.line_.shrink_ = 1;
  line_style.line_.line_type_ = MarkdownLineType::kSolid;
  quote.SetQuoteBorderLineStyle(line_style);
  quote.SetBorders({
      MakeBorderSide(MarkdownBorderType::kSolid, 0xff123456, 4),
      MakeBorderSide(MarkdownBorderType::kNone, 0, 0),
      MakeBorderSide(MarkdownBorderType::kNone, 0, 0),
      MakeBorderSide(MarkdownBorderType::kNone, 0, 0),
  });
  TestLayoutNode child(true, {.width = 20,
                              .height = 10,
                              .baseline = 8,
                              .char_count = 3,
                              .line_count = 1});
  quote.AppendChild(&child);
  quote.Layout({.width = 100, .height = 100});

  testing::MockMarkdownCanvas canvas(nullptr, nullptr);
  quote.Render({.canvas = &canvas, .max_char_count = 10, .cursor = nullptr});

  const auto& ops = canvas.GetJson();
  ASSERT_GE(ops.Size(), 3u);
  ASSERT_STREQ(ops[1]["op"].GetString(), "round rect");
  EXPECT_FLOAT_EQ(ops[1]["rect"]["left"].GetFloat(), 2);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["top"].GetFloat(), 1);
  EXPECT_FLOAT_EQ(ops[1]["rect"]["right"].GetFloat(), 6);
  ASSERT_STREQ(ops[2]["op"].GetString(), "line");
  EXPECT_FLOAT_EQ(ops[2]["p1"]["x"].GetFloat(), 4);
}

}  // namespace serval::markdown
