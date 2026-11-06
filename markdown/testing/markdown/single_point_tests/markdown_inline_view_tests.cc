// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "gtest/gtest.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_view_measurer.h"
#include "markdown/view/markdown_view_renderer.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"

namespace serval::markdown::testing {
namespace {

class AlignedInlineView final : public MarkdownDrawable {
 public:
  MarkdownVerticalAlign GetVerticalAlign() const override {
    return vertical_align_;
  }
  float GetVerticalAlignLength() const override { return length_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {}
  void Align(float x, float y) override {
    ++align_count_;
    position_ = {x, y};
  }
  void SetBounds(RectF bounds) override { bounds_ = bounds; }

  MarkdownVerticalAlign vertical_align_{MarkdownVerticalAlign::kBaseline};
  float length_{0};
  PointF position_{};
  RectF bounds_{};
  MeasureSpec last_spec_{};
  int measure_count_{0};
  int align_count_{0};

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    ++measure_count_;
    last_spec_ = spec;
    return {.width_ = 20, .height_ = 30, .baseline_ = 11};
  }
};

class InlineViewResourceLoader final : public MockMarkdownResourceLoader {
 public:
  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override {
    return view_;
  }

  std::shared_ptr<AlignedInlineView> view_ =
      std::make_shared<AlignedInlineView>();
};

struct InlineViewAlignmentCase {
  MarkdownVerticalAlign align;
  float length;
  float normal_baseline;
  float heading_baseline;
};

TEST(MarkdownInlineViewRenderTest, TableScrollUpdatesBoundsWithoutLayout) {
  InlineViewResourceLoader loader;
  auto context = CreateTestMarkdownSharedContext();
  MarkdownViewMeasurer measurer(context, &loader);
  auto style = MarkdownStyleReader::ReadStyle({}, &loader, context.get());
  style.table_.scroll_.scroll_x_ = true;
  style.table_cell_.block_.min_width_ = 150;
  measurer.SetStyle(style);
  measurer.SetContent(
      "| first | second | third |\n| --- | --- | --- |\n"
      "| ![](inlineview://view) | content | content |\n");
  measurer.Measure(
      {.width_ = 200, .width_mode_ = tttext::LayoutMode::kDefinite});
  measurer.Align();
  auto document = measurer.GetDocument();
  ASSERT_EQ(document->GetInlineViews().size(), 1u);
  ASSERT_TRUE(document->GetPage()->GetRegion(0)->scroll_x_);
  const auto layout_position = loader.view_->position_;
  const auto measure_count = loader.view_->measure_count_;
  const auto align_count = loader.view_->align_count_;

  MarkdownViewRenderer renderer;
  renderer.SetDocument(document);
  const auto before = loader.view_->bounds_;
  EXPECT_FLOAT_EQ(before.GetLeft(), layout_position.x_);
  EXPECT_FLOAT_EQ(before.GetTop(), layout_position.y_);
  ASSERT_TRUE(document->SetScrollXRegionOffset(0, -30));
  document->UpdateInlineViewBoundsInRegion(0);

  EXPECT_FLOAT_EQ(loader.view_->bounds_.GetLeft(), before.GetLeft() - 30);
  EXPECT_FLOAT_EQ(loader.view_->bounds_.GetTop(), before.GetTop());
  EXPECT_FLOAT_EQ(loader.view_->bounds_.GetWidth(), 20);
  EXPECT_FLOAT_EQ(loader.view_->bounds_.GetHeight(), 30);
  EXPECT_FLOAT_EQ(loader.view_->position_.x_, layout_position.x_);
  EXPECT_FLOAT_EQ(loader.view_->position_.y_, layout_position.y_);
  EXPECT_EQ(loader.view_->measure_count_, measure_count);
  EXPECT_EQ(loader.view_->align_count_, align_count);
}

class MarkdownInlineViewTest
    : public ::testing::TestWithParam<InlineViewAlignmentCase> {};

TEST_P(MarkdownInlineViewTest, AlignsUsingSurroundingFontSizeAfterRelayout) {
  InlineViewResourceLoader loader;
  loader.view_->vertical_align_ = GetParam().align;
  loader.view_->length_ = GetParam().length;
  MarkdownViewMeasurer measurer(CreateTestMarkdownSharedContext(), &loader);
  measurer.InitialDocument();
  auto style = measurer.GetDocument()->GetStyle();
  style.normal_text_.base_.font_size_ = 20;
  style.h1_.base_.font_size_ = 40;
  measurer.SetStyle(style);

  for (bool heading : {false, true}) {
    measurer.SetContent(heading ? "# a ![](inlineview://view) b"
                                : "a ![](inlineview://view) b");
    for (float width : {400.f, 380.f}) {
      SCOPED_TRACE(heading);
      SCOPED_TRACE(width);
      MeasureSpec spec;
      spec.width_ = width;
      spec.width_mode_ = tttext::LayoutMode::kDefinite;
      measurer.Measure(spec);
      measurer.Align();

      EXPECT_FLOAT_EQ(loader.view_->last_spec_.width_, width);
      EXPECT_FLOAT_EQ(loader.view_->last_spec_.height_,
                      MeasureSpec::LAYOUT_MAX_SIZE);
      auto document = measurer.GetDocument();
      ASSERT_EQ(document->GetInlineViews().size(), 1u);
      auto* region = static_cast<MarkdownPageParagraphRegion*>(
          document->GetPage()->GetRegion(0));
      ASSERT_EQ(region->region_->GetLineCount(), 1u);
      auto* line = region->region_->GetLine(0);
      const auto expected_top =
          region->rect_.GetTop() + line->GetLineBaseLine() -
          (heading ? GetParam().heading_baseline : GetParam().normal_baseline);
      const auto origin = document->GetInlineViewOrigin("view");
      EXPECT_NEAR(origin.second, expected_top, 0.001f);
      EXPECT_FLOAT_EQ(loader.view_->position_.x_, origin.first);
      EXPECT_FLOAT_EQ(loader.view_->position_.y_, origin.second);

      document->UpdateInlineViewBoundsInRegion(0);
      EXPECT_NEAR(loader.view_->bounds_.GetTop(), expected_top, 0.001f);
      EXPECT_FLOAT_EQ(loader.view_->bounds_.GetWidth(), 20);
      EXPECT_FLOAT_EQ(loader.view_->bounds_.GetHeight(), 30);
      // Reusing the same platform view must not accumulate baseline shifts.
      EXPECT_FLOAT_EQ(loader.view_->GetAscent(), -11);
    }
  }
}

TEST_P(MarkdownInlineViewTest, BlockViewsKeepMeasuredBaseline) {
  InlineViewResourceLoader loader;
  loader.view_->vertical_align_ = GetParam().align;
  loader.view_->length_ = GetParam().length;
  MarkdownViewMeasurer measurer(CreateTestMarkdownSharedContext(), &loader);
  measurer.SetContent("![](blockview://view)");
  MeasureSpec spec;
  spec.width_ = 400;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  measurer.Measure(spec);

  auto document = measurer.GetDocument();
  ASSERT_EQ(document->GetInlineViews().size(), 1u);
  EXPECT_TRUE(document->GetInlineViews().front().is_block_view_);
  auto* region = static_cast<MarkdownPageParagraphRegion*>(
      document->GetPage()->GetRegion(0));
  auto* line = region->region_->GetLine(0);
  EXPECT_NEAR(document->GetInlineViewOrigin("view").second,
              region->rect_.GetTop() + line->GetLineBaseLine() - 11, 0.001f);
}

INSTANTIATE_TEST_SUITE_P(
    VerticalAlign, MarkdownInlineViewTest,
    ::testing::Values(
        InlineViewAlignmentCase{MarkdownVerticalAlign::kBaseline, 10, 11, 11},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kTop, 0, 18.4f, 36.8f},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kCenter, 0, 21.8f,
                                28.6f},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kBottom, 0, 25.2f,
                                20.4f},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kTextTop, 0, 18.4f,
                                36.8f},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kTextBottom, 0, 25.2f,
                                20.4f},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kLength, 10, 21, 21},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kLength, -8, 3, 3},
        InlineViewAlignmentCase{MarkdownVerticalAlign::kLength, 0, 11, 11}));

}  // namespace
}  // namespace serval::markdown::testing
