// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>
#include <memory>

#include "gtest/gtest.h"
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

}  // namespace serval::markdown::testing
