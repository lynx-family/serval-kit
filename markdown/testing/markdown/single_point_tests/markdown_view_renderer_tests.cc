// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <memory>

#include "gtest/gtest.h"
#include "markdown/view/markdown_view_gesture.h"
#include "markdown/view/markdown_view_measurer.h"
#include "markdown/view/markdown_view_renderer.h"
#include "testing/markdown/mock_platform/markdown_tests_platform.h"
#include "testing/markdown/mock_platform/mock_markdown_platform_view.h"

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
    const auto position = view->GetAlignedPosition();
    if (std::fabs(position.x_ - rect.GetLeft()) < kFloatTolerance &&
        std::fabs(position.y_ - rect.GetTop()) < kFloatTolerance &&
        std::fabs(view->measured_size_.width_ - rect.GetWidth()) <
            kFloatTolerance &&
        std::fabs(view->measured_size_.height_ - rect.GetHeight()) <
            kFloatTolerance) {
      return view;
    }
  }
  return nullptr;
}

}  // namespace

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
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  // Border views are only used together with content region views, i.e. with
  // a region-based animation type.
  renderer.SetMarkdownAnimationType(MarkdownAnimationType::kTypewriter);

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

TEST(MarkdownViewRendererTest,
     SwitchesFromTypewriterToNoneRemovesAllRegionViews) {
  auto context = CreateTestMarkdownSharedContext();
  auto document = CreateDocumentWithQuoteBorder(context);
  ASSERT_NE(document, nullptr);
  auto page = document->GetPage();
  ASSERT_NE(page, nullptr);
  ASSERT_EQ(page->GetExtraBorderCount(), 1u);
  const auto border_rect = page->GetExtraBorder(0)->rect_;

  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);

  // Use a large viewport that covers both the border and the content regions.
  main_view.SetViewRectInScreen(
      RectF::MakeLTRB(0, 0, 240, border_rect.GetBottom() + 100));

  // With a region-based animation, both content region views and the border
  // view should be created.
  renderer.SetMarkdownAnimationType(MarkdownAnimationType::kTypewriter);
  renderer.OnNextFrame();
  const auto animated_subview_count = main_view.GetSubviewCount();
  EXPECT_GT(animated_subview_count, 1u);
  ASSERT_NE(FindSubviewForRect(&main_view, border_rect), nullptr);

  // Switching to kNone should drop the content region views and the border
  // view: the main view draws the whole page, quote borders included, so
  // keeping the border view would render every border twice.
  renderer.SetMarkdownAnimationType(MarkdownAnimationType::kNone);
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);
  EXPECT_EQ(FindSubviewForRect(&main_view, border_rect), nullptr);

  // Switching back to a region-based animation should recreate the content
  // region views and the border view.
  renderer.SetMarkdownAnimationType(MarkdownAnimationType::kTypewriter);
  renderer.OnNextFrame();
  EXPECT_EQ(main_view.GetSubviewCount(), animated_subview_count);
  EXPECT_NE(FindSubviewForRect(&main_view, border_rect), nullptr);
}

TEST(MarkdownViewRendererTest, NoneAnimationScrollXPanInvalidatesMainView) {
  auto context = CreateTestMarkdownSharedContext();
  MarkdownViewMeasurer measurer(context);
  ValueMap code_block_style;
  code_block_style.emplace("scrollX", Value::MakeBool(true));
  ValueMap style;
  style.emplace("codeBlock", Value::MakeMap(std::move(code_block_style)));
  measurer.SetStyle(style);
  measurer.SetContent(
      "```\n"
      "a very long line of code that is definitely wider than the two hundred "
      "and forty pixels available for the markdown view here\n"
      "```");
  measurer.Measure({.width_ = 240,
                    .width_mode_ = tttext::LayoutMode::kDefinite,
                    .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
                    .height_mode_ = tttext::LayoutMode::kIndefinite});
  auto document = measurer.GetDocument();
  ASSERT_NE(document, nullptr);
  auto page = document->GetPage();
  ASSERT_NE(page, nullptr);
  uint32_t scroll_x_region = page->GetRegionCount();
  for (uint32_t i = 0; i < page->GetRegionCount(); ++i) {
    auto* region = page->GetRegion(i);
    if (region != nullptr && region->scroll_x_) {
      scroll_x_region = i;
      break;
    }
  }
  ASSERT_LT(scroll_x_region, page->GetRegionCount());
  const auto scroll_x_rect =
      page->GetRegion(scroll_x_region)->scroll_x_view_rect_;

  MockMarkdownMainView main_view(context);
  MarkdownViewRenderer renderer;
  renderer.SetViewContainerHandle(&main_view);
  renderer.SetDocument(document);
  MarkdownViewGesture gesture(&main_view, context.get(), &renderer);
  gesture.SetDocument(document);

  // Static content (animation-type none) creates no region subviews; the
  // main view draws the whole page.
  renderer.OnNextFrame();
  ASSERT_EQ(main_view.GetSubviewCount(), 0u);
  main_view.needs_draw_ = false;

  // A horizontal pan on the scroll-x region must invalidate the main view,
  // otherwise the scrolled content is never repainted.
  const PointF pan_start{scroll_x_rect.GetLeft() + 10,
                         scroll_x_rect.GetTop() + 10};
  ASSERT_TRUE(gesture.ShouldBeginPan(pan_start, {-10, 0}));
  ASSERT_TRUE(gesture.OnPan(pan_start, {-10, 0}, GestureEventType::kDown));
  EXPECT_TRUE(gesture.OnPan(pan_start, {-10, 0}, GestureEventType::kMove));
  EXPECT_LT(page->GetRegion(scroll_x_region)->scroll_x_offset_, 0.f);
  EXPECT_TRUE(main_view.needs_draw_);
}

}  // namespace serval::markdown::testing
