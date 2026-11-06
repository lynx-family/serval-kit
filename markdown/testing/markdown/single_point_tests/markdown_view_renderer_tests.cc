// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/view/markdown_view_measurer.h"
#include "markdown/view/markdown_view_renderer.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_platform_view.h"
#include "testing/markdown/mock_platform/mock_markdown_resource_loader.h"

namespace serval::markdown::testing {
namespace {

constexpr float kFloatTolerance = 0.001f;

std::shared_ptr<MarkdownDocument> CreateDocumentWithQuoteBorder(
    const std::shared_ptr<MarkdownContext>& context) {
  MarkdownViewMeasurer measurer(context);
  measurer.SetContent(
      "> quote line one\n> quote line two\n\n"
      "paragraph after quote\n\n"
      "another paragraph after quote");
  measurer.Measure({.width_ = 240,
                    .width_mode_ = tttext::LayoutMode::kDefinite,
                    .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
                    .height_mode_ = tttext::LayoutMode::kIndefinite});
  return measurer.GetDocument();
}

MockMarkdownPlatformView* FindSubviewForRect(MockMarkdownMainView* main_view,
                                             const RectF& rect) {
  for (auto* view : main_view->GetSubviews()) {
    const auto& bounds = view->GetBounds();
    if (std::fabs(bounds.GetLeft() - rect.GetLeft()) < kFloatTolerance &&
        std::fabs(bounds.GetTop() - rect.GetTop()) < kFloatTolerance &&
        std::fabs(bounds.GetWidth() - rect.GetWidth()) < kFloatTolerance &&
        std::fabs(bounds.GetHeight() - rect.GetHeight()) < kFloatTolerance) {
      return view;
    }
  }
  return nullptr;
}

}  // namespace

TEST(MarkdownPlatformViewTest, RenderBoundsPreserveLayoutResults) {
  MockMarkdownPlatformView view(nullptr, nullptr);
  view.measured_size_ = {20, 30};
  view.Measure({});
  view.Align(1, 2);
  view.SetBounds(RectF::MakeLTWH(40, 50, 60, 70));

  EXPECT_FLOAT_EQ(view.GetMeasuredSize().width_, 20);
  EXPECT_FLOAT_EQ(view.GetMeasuredSize().height_, 30);
  EXPECT_FLOAT_EQ(view.GetAlignedPosition().x_, 1);
  EXPECT_FLOAT_EQ(view.GetAlignedPosition().y_, 2);
  EXPECT_FLOAT_EQ(view.GetBounds().GetLeft(), 40);
  EXPECT_FLOAT_EQ(view.GetBounds().GetTop(), 50);
  EXPECT_FLOAT_EQ(view.GetBounds().GetWidth(), 60);
  EXPECT_FLOAT_EQ(view.GetBounds().GetHeight(), 70);

  view.SetBounds(RectF::MakeEmpty());
  EXPECT_FALSE(view.IsVisible());
  EXPECT_TRUE(view.GetBounds().IsEmpty());
  view.SetBounds(RectF::MakeLTWH(10, 15, 25, 35));
  EXPECT_TRUE(view.IsVisible());
  EXPECT_FLOAT_EQ(view.GetBounds().GetWidth(), 25);
  EXPECT_FLOAT_EQ(view.GetMeasuredSize().width_, 20);
  EXPECT_FLOAT_EQ(view.GetAlignedPosition().x_, 1);
}

TEST(MarkdownViewRendererTest, RegionBoundsDoNotMeasureOrAlignViews) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetEnableRegionView(true);
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  renderer.OnNextFrame();

  ASSERT_GT(main_view.GetSubviewCount(), 0u);
  for (auto* view : main_view.GetSubviews()) {
    EXPECT_FALSE(view->GetBounds().IsEmpty());
    EXPECT_FLOAT_EQ(view->measured_size_.width_, 0);
    EXPECT_FLOAT_EQ(view->measured_size_.height_, 0);
    EXPECT_FLOAT_EQ(view->align_position_.x_, 0);
    EXPECT_FLOAT_EQ(view->align_position_.y_, 0);
  }
}

TEST(MarkdownViewRendererTest, VisibleRenderRectIntersectsPageAndHostClip) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  const auto& page = document->GetPage();
  const auto page_bounds =
      RectF::MakeWH(page->GetLayoutWidth(), page->GetLayoutHeight());

  // The native view can be larger than the rendered content.
  main_view.SetViewRectInScreen(RectF::MakeLTRB(
      -20, -30, page_bounds.GetRight() + 50, page_bounds.GetBottom() + 60));
  EXPECT_EQ(renderer.GetVisibleRenderRect(), page_bounds);

  const auto host_clip = RectF::MakeLTRB(10, 5, page_bounds.GetRight() - 10,
                                         page_bounds.GetBottom() - 5);
  main_view.SetViewRectInScreen(host_clip);
  EXPECT_EQ(renderer.GetVisibleRenderRect(), host_clip);

  // Clipping at the bottom still retains the host's horizontal clipping.
  main_view.SetViewRectInScreen(RectF::MakeLTRB(
      10, 5, page_bounds.GetRight() + 50, page_bounds.GetBottom() + 60));
  EXPECT_EQ(
      renderer.GetVisibleRenderRect(),
      RectF::MakeLTRB(10, 5, page_bounds.GetRight(), page_bounds.GetBottom()));
}

TEST(MarkdownViewRendererTest, VisibleRenderRectUsesTableViewportWidth) {
  auto context = CreateTestMarkdownSharedContext();
  MarkdownViewMeasurer measurer(context);
  auto style = MarkdownStyleReader::ReadStyle({}, nullptr, context.get());
  style.table_.scroll_.scroll_x_ = true;
  style.table_cell_.block_.min_width_ = 150;
  measurer.SetStyle(style);
  measurer.SetContent(
      "| first | second | third |\n| --- | --- | --- |\n"
      "| content | content | content |\n");
  measurer.Measure(
      {.width_ = 200, .width_mode_ = tttext::LayoutMode::kDefinite});
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(measurer.GetDocument());
  const auto& page = measurer.GetDocument()->GetPage();

  ASSERT_TRUE(page->GetRegion(0)->scroll_x_);
  ASSERT_GT(page->GetRegionRect(0).GetWidth(), 200);
  EXPECT_FLOAT_EQ(renderer.GetVisibleRenderRect().GetWidth(), 200);

  measurer.GetDocument()->SetScrollXRegionOffset(0, -30);
  EXPECT_FLOAT_EQ(renderer.GetVisibleRenderRect().GetWidth(), 200);
}

TEST(MarkdownViewRendererTest, VisibleRenderRectUsesRenderedDocument) {
  auto context = CreateTestMarkdownSharedContext();
  MarkdownViewMeasurer measurer(context);
  measurer.SetContent("first paragraph\n\nsecond paragraph");
  measurer.Measure(
      {.width_ = 240, .width_mode_ = tttext::LayoutMode::kDefinite});
  auto rendered_document = measurer.GetDocument();
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(rendered_document);
  const auto rendered_rect = renderer.GetVisibleRenderRect();

  measurer.Measure(
      {.width_ = 120, .width_mode_ = tttext::LayoutMode::kDefinite});
  ASSERT_NE(rendered_document, measurer.GetDocument());
  EXPECT_EQ(renderer.GetVisibleRenderRect(), rendered_rect);

  renderer.SetDocument(measurer.GetDocument());
  EXPECT_FLOAT_EQ(renderer.GetVisibleRenderRect().GetWidth(),
                  measurer.GetDocument()->GetPage()->GetLayoutWidth());
  EXPECT_LT(renderer.GetVisibleRenderRect().GetWidth(),
            rendered_rect.GetWidth());
}

TEST(MarkdownViewRendererTest, RemovesRegionViewsWhenHostClipMissesPage) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetEnableRegionView(true);
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  renderer.OnNextFrame();
  ASSERT_GT(main_view.GetSubviewCount(), 0u);

  const auto& page = document->GetPage();
  main_view.SetViewRectInScreen(RectF::MakeLTWH(page->GetLayoutWidth() + 10, 0,
                                                50, page->GetLayoutHeight()));
  EXPECT_TRUE(renderer.GetVisibleRenderRect().IsEmpty());
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);

  main_view.SetViewRectInScreen(RectF::MakeWH(240, 1));
  renderer.OnNextFrame();
  ASSERT_GT(main_view.GetSubviewCount(), 0u);
  main_view.SetViewRectInScreen(RectF::MakeEmpty());
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);

  main_view.SetViewRectInScreen(
      MockMarkdownMainView::MakeDefaultViewRectInScreen());
  renderer.OnNextFrame();
  EXPECT_GT(main_view.GetSubviewCount(), 0u);
}

TEST(MarkdownViewRendererTest, PublishesCustomTruncationBoundsWithoutAlign) {
  auto context = CreateTestMarkdownSharedContext();
  MockMarkdownMainView main_view(context);
  MockMarkdownResourceLoader loader;
  loader.SetMainView(&main_view);
  MarkdownViewMeasurer measurer(context, &loader);
  auto style = MarkdownStyleReader::ReadStyle({}, &loader, context.get());
  style.truncation_.truncation_.truncation_type_ =
      MarkdownTruncationType::kView;
  style.truncation_.truncation_.content_ = "truncation";
  measurer.SetStyle(style);
  measurer.SetContent("first paragraph\n\nsecond paragraph\n\nthird paragraph");
  measurer.SetTextMaxLines(2);
  measurer.Measure(
      {.width_ = 300, .width_mode_ = tttext::LayoutMode::kDefinite});

  auto document = measurer.GetDocument();
  auto truncation = std::static_pointer_cast<MockInlineView>(
      document->GetTruncationDelegate());
  ASSERT_NE(truncation, nullptr);
  const auto origin = document->GetTruncationOrigin();
  ASSERT_GT(origin.x_, 0);
  ASSERT_GT(origin.y_, 0);

  MarkdownViewRenderer renderer;
  renderer.SetDocument(document);

  EXPECT_FLOAT_EQ(truncation->GetBounds().GetLeft(), origin.x_);
  EXPECT_FLOAT_EQ(truncation->GetBounds().GetTop(), origin.y_);
  EXPECT_FLOAT_EQ(truncation->GetBounds().GetWidth(), truncation->GetAdvance());
  EXPECT_FLOAT_EQ(truncation->GetAlignedPosition().x_, 0);
  EXPECT_FLOAT_EQ(truncation->GetAlignedPosition().y_, 0);
}

TEST(MarkdownViewRendererTest, ExtraBorderRangeOutsideViewportIsEmpty) {
  auto document =
      CreateDocumentWithQuoteBorder(CreateTestMarkdownSharedContext());
  ASSERT_NE(document, nullptr);
  auto page = document->GetPage();
  ASSERT_NE(page, nullptr);
  ASSERT_EQ(page->GetExtraBorderCount(), 1u);
  const auto border_rect = page->GetExtraBorder(0)->rect_;

  const auto before = document->GetShowedExtraContents(
      border_rect.GetTop() - 100, border_rect.GetTop() - 10);
  EXPECT_LT(before.start_, 0);
  EXPECT_LT(before.end_, 0);

  const auto intersecting = document->GetShowedExtraContents(
      border_rect.GetTop(), border_rect.GetBottom());
  EXPECT_EQ(intersecting.start_, 0);
  EXPECT_EQ(intersecting.end_, 0);

  const auto after = document->GetShowedExtraContents(
      border_rect.GetBottom() + 10, border_rect.GetBottom() + 100);
  EXPECT_LT(after.start_, 0);
  EXPECT_LT(after.end_, 0);
}

TEST(MarkdownViewRendererTest, RemovesBorderViewAfterViewportJump) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  ASSERT_NE(document, nullptr);
  auto page = document->GetPage();
  ASSERT_NE(page, nullptr);
  ASSERT_EQ(page->GetExtraBorderCount(), 1u);
  const auto border_rect = page->GetExtraBorder(0)->rect_;

  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetEnableRegionView(true);
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);

  main_view.SetViewRectInScreen(RectF::MakeLTRB(
      0, border_rect.GetTop() - 100, 240, border_rect.GetTop() - 10));
  renderer.OnNextFrame();
  EXPECT_EQ(FindSubviewForRect(&main_view, border_rect), nullptr);

  main_view.SetViewRectInScreen(
      RectF::MakeLTRB(0, border_rect.GetTop(), 240, border_rect.GetBottom()));
  renderer.OnNextFrame();
  auto* border_view = FindSubviewForRect(&main_view, border_rect);
  ASSERT_NE(border_view, nullptr);

  main_view.SetViewRectInScreen(RectF::MakeLTRB(
      0, border_rect.GetBottom() + 10, 240, border_rect.GetBottom() + 100));
  renderer.OnNextFrame();
  EXPECT_FALSE(main_view.ContainsSubview(border_view));
}

TEST(MarkdownViewRendererTest, EnableRegionViewControlsSubviewRendering) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  ASSERT_NE(document, nullptr);

  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);

  renderer.SetEnableRegionView(true);
  renderer.OnNextFrame();
  EXPECT_GT(main_view.GetSubviewCount(), 0u);

  renderer.SetEnableRegionView(false);
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);

  renderer.SetEnableRegionView(true);
  renderer.OnNextFrame();
  EXPECT_GT(main_view.GetSubviewCount(), 0u);
}

TEST(MarkdownViewRendererTest,
     RequestDrawRegionRequestsMainViewWhenRegionViewsDisabled) {
  auto context = CreateTestMarkdownSharedContext();
  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer(&main_view);
  renderer.SetViewContainerHandle(&main_view);
  main_view.needs_draw_ = false;

  renderer.RequestDrawRegion(0);

  EXPECT_TRUE(main_view.needs_draw_);
}

TEST(MarkdownViewRendererTest, RebindsRegionViewsWhenDocumentChanges) {
  auto context = CreateTestMarkdownSharedContext();
  auto first_document = CreateDocumentWithQuoteBorder(context);
  auto second_document = CreateDocumentWithQuoteBorder(context);
  ASSERT_NE(first_document, nullptr);
  ASSERT_NE(second_document, nullptr);

  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetEnableRegionView(true);
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(first_document);
  renderer.OnNextFrame();
  ASSERT_GT(main_view.GetSubviewCount(), 0u);

  const auto original_views = main_view.GetSubviews();
  std::vector<MarkdownDrawable*> original_drawables;
  for (auto* view : original_views) {
    ASSERT_NE(view->GetCustomViewHandle(), nullptr);
    original_drawables.push_back(view->GetCustomViewHandle()->GetDrawable());
    view->needs_draw_ = false;
  }

  renderer.SetDocument(second_document);

  ASSERT_EQ(main_view.GetSubviewCount(), original_views.size());
  for (size_t i = 0; i < original_views.size(); ++i) {
    EXPECT_TRUE(main_view.ContainsSubview(original_views[i]));
    EXPECT_NE(original_views[i]->GetCustomViewHandle()->GetDrawable(),
              original_drawables[i]);
    EXPECT_TRUE(original_views[i]->needs_draw_);
  }
}

}  // namespace serval::markdown::testing
