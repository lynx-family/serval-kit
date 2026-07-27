// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include "../mock_platform/markdown_tests_platform.h"
#include "../mock_platform/mock_markdown_platform_view.h"

namespace serval::markdown {

TEST(MarkdownContextTest, HarmonyShaperForceLowAPIDefaultsToEnabled) {
  auto context = testing::CreateTestMarkdownSharedContext();

  EXPECT_TRUE(context->IsHarmonyShaperForceLowAPI());

  context->SetHarmonyShaperForceLowAPI(false);
  EXPECT_FALSE(context->IsHarmonyShaperForceLowAPI());
}

TEST(MarkdownViewTest, GetMarkdownContextReturnsConstructorContext) {
  auto context = testing::CreateTestMarkdownSharedContext();
  testing::MockMarkdownMainView main_view(context);

  EXPECT_EQ(main_view.GetMarkdownView()->GetMarkdownContext(), context.get());
}

TEST(MarkdownViewTest, GestureListenersOverrideDefaultDispatch) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto* view = main_view.GetMarkdownView();

  const PointF position{10, 20};
  const PointF motion{3, 4};
  int long_press_count = 0;
  int tap_count = 0;
  int pan_count = 0;

  view->SetLongPressListener(
      [&](PointF received_position, GestureEventType event) {
        ++long_press_count;
        EXPECT_EQ(received_position, position);
        EXPECT_EQ(event, GestureEventType::kDown);
        return true;
      });
  view->SetTapListener([&](PointF received_position, GestureEventType event) {
    ++tap_count;
    EXPECT_EQ(received_position, position);
    EXPECT_EQ(event, GestureEventType::kUp);
    return true;
  });
  view->SetPanListener([&](PointF received_position, PointF received_motion,
                           GestureEventType event) {
    ++pan_count;
    EXPECT_EQ(received_position, position);
    EXPECT_EQ(received_motion, motion);
    EXPECT_EQ(event, GestureEventType::kMove);
    EXPECT_EQ(view->GetPanPosition(), PointF());
    return true;
  });

  EXPECT_TRUE(view->OnLongPress(position, GestureEventType::kDown));
  EXPECT_TRUE(view->OnTap(position, GestureEventType::kUp));
  EXPECT_TRUE(view->OnPan(position, motion, GestureEventType::kMove));
  EXPECT_EQ(long_press_count, 1);
  EXPECT_EQ(tap_count, 1);
  EXPECT_EQ(pan_count, 1);
}

TEST(MarkdownViewTest, GetPanPositionUpdatesOnlyWhenDoPanRuns) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto* view = main_view.GetMarkdownView();

  EXPECT_EQ(view->GetPanPosition(), PointF());

  const PointF listener_position{10, 20};
  view->SetPanListener([](PointF, PointF, GestureEventType) { return true; });
  EXPECT_TRUE(view->OnPan(listener_position, {1, 2}, GestureEventType::kMove));
  EXPECT_EQ(view->GetPanPosition(), PointF());

  const PointF default_position{30, 40};
  view->DoPan(default_position, {3, 4}, GestureEventType::kMove);
  EXPECT_EQ(view->GetPanPosition(), default_position);
}

TEST(MarkdownViewTest, DoGesturesBypassListeners) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto* view = main_view.GetMarkdownView();

  int listener_count = 0;
  view->SetLongPressListener([&](PointF, GestureEventType) {
    ++listener_count;
    return true;
  });
  view->SetTapListener([&](PointF, GestureEventType) {
    ++listener_count;
    return true;
  });
  view->SetPanListener([&](PointF, PointF, GestureEventType) {
    ++listener_count;
    return true;
  });

  EXPECT_FALSE(view->DoLongPress({}, GestureEventType::kDown));
  EXPECT_FALSE(view->DoTap({}, GestureEventType::kDown));
  EXPECT_FALSE(view->DoPan({}, {}, GestureEventType::kDown));
  EXPECT_EQ(listener_count, 0);
}

TEST(MarkdownViewTest, EmptyGestureListenersUseDefaultDispatch) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto* view = main_view.GetMarkdownView();

  view->SetLongPressListener({});
  view->SetTapListener({});
  view->SetPanListener({});

  EXPECT_EQ(view->OnLongPress({}, GestureEventType::kDown),
            view->DoLongPress({}, GestureEventType::kDown));
  EXPECT_EQ(view->OnTap({}, GestureEventType::kDown),
            view->DoTap({}, GestureEventType::kDown));
  EXPECT_EQ(view->OnPan({}, {}, GestureEventType::kDown),
            view->DoPan({}, {}, GestureEventType::kDown));
}

}  // namespace serval::markdown
