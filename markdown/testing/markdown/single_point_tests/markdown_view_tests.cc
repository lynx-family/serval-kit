// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include "../mock_platform/markdown_tests_platform.h"
#include "../mock_platform/mock_markdown_platform_view.h"
#include "markdown/markdown_event_listener.h"

namespace serval::markdown {
namespace {

class CountingMarkdownViewMeasureHost final : public MarkdownViewMeasureHost {
 public:
  void RequestMeasure() override { ++request_count_; }

  int32_t request_count_{0};
};

class CountingMarkdownEventListener final : public MarkdownEventListener {
 public:
  void OnParseEnd() override { ++parse_count_; }
  void OnTextOverflow(MarkdownTextOverflow) override {}
  void OnDrawStart() override {}
  void OnDrawEnd() override {}
  void OnAnimationStep(int32_t, int32_t) override {}
  void OnLinkClicked(const char*, const char*) override {}
  void OnImageClicked(const char*) override {}
  void OnSelectionChanged(int32_t, int32_t, SelectionHandleType,
                          SelectionState) override {}

  int32_t parse_count_{0};
};

MeasureSpec MakeMeasureSpec() {
  return {
      .width_ = 200,
      .width_mode_ = tttext::LayoutMode::kDefinite,
      .height_ = MeasureSpec::LAYOUT_MAX_SIZE,
      .height_mode_ = tttext::LayoutMode::kIndefinite,
  };
}

}  // namespace

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

TEST(MarkdownViewTest, ContentChangeRequestsMeasureOnlyOnMainView) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto subview = main_view.CreateCustomSubView();
  auto* mock_subview =
      static_cast<testing::MockMarkdownPlatformView*>(subview.get());
  main_view.needs_measure_ = false;
  main_view.needs_align_ = false;
  main_view.needs_draw_ = false;

  main_view.GetMarkdownView()->SetContent("updated content");

  EXPECT_TRUE(main_view.needs_measure_);
  EXPECT_TRUE(main_view.needs_align_);
  EXPECT_TRUE(main_view.needs_draw_);
  EXPECT_FALSE(mock_subview->needs_measure_);
  EXPECT_FALSE(mock_subview->needs_align_);
  EXPECT_FALSE(mock_subview->needs_draw_);
}

TEST(MarkdownViewTest, MeasuresWithoutPlatformViewAndBindsViewOnce) {
  auto context = testing::CreateTestMarkdownSharedContext();
  CountingMarkdownViewMeasureHost measure_host;
  CountingMarkdownEventListener event_listener;
  auto view = std::make_shared<MarkdownView>(nullptr, &measure_host, context);
  view->SetEventListener(&event_listener);

  view->SetContent("markdown content");
  EXPECT_EQ(measure_host.request_count_, 1);

  const auto spec = MakeMeasureSpec();
  const auto premeasured_size = view->Measure(spec);
  EXPECT_GT(premeasured_size.width_, 0);
  EXPECT_GT(premeasured_size.height_, 0);
  EXPECT_EQ(event_listener.parse_count_, 1);

  view->SetView(nullptr);
  EXPECT_EQ(measure_host.request_count_, 1);

  testing::MockMarkdownMainView main_view(context);
  main_view.AttachDrawable(view);
  main_view.needs_draw_ = false;
  view->SetView(&main_view);

  EXPECT_EQ(measure_host.request_count_, 2);
  EXPECT_TRUE(main_view.needs_draw_);

  const auto bound_size = main_view.Measure(spec);
  EXPECT_EQ(event_listener.parse_count_, 1);
  EXPECT_FLOAT_EQ(bound_size.width_, premeasured_size.width_);
  EXPECT_FLOAT_EQ(bound_size.height_, premeasured_size.height_);

  testing::MockMarkdownMainView replacement_view(context);
  main_view.needs_draw_ = false;
  replacement_view.needs_draw_ = false;
  view->SetView(&replacement_view);
  view->NeedsDraw();

  EXPECT_TRUE(main_view.needs_draw_);
  EXPECT_FALSE(replacement_view.needs_draw_);
}

TEST(MarkdownViewTest, CreatesSelectionSubviewsAfterBindingView) {
  auto context = testing::CreateTestMarkdownSharedContext();
  CountingMarkdownViewMeasureHost measure_host;
  auto view = std::make_shared<MarkdownView>(nullptr, &measure_host, context);
  view->SetEnableSelection(true);

  testing::MockMarkdownMainView main_view(context);
  main_view.AttachDrawable(view);
  EXPECT_EQ(main_view.GetSubviewCount(), 0u);

  view->SetView(&main_view);

  EXPECT_EQ(main_view.GetSubviewCount(), 3u);
}

TEST(MarkdownViewTest, SelectionChangeRequestsSubviewDrawWithoutMeasure) {
  testing::MockMarkdownMainView main_view(
      testing::CreateTestMarkdownSharedContext());
  auto* view = main_view.GetMarkdownView();
  view->SetContent("selectable text");
  view->SetEnableSelection(true);
  main_view.Measure({.width_ = 200,
                     .width_mode_ = tttext::LayoutMode::kDefinite,
                     .height_ = 200,
                     .height_mode_ = tttext::LayoutMode::kAtMost});
  main_view.Align(0, 0);
  main_view.OnVSync(0);
  size_t selection_view_count = 0;
  for (auto* subview : main_view.GetSubviews()) {
    if (subview->view_name_.find("selection") == 0) {
      ++selection_view_count;
    }
  }
  ASSERT_EQ(selection_view_count, 3u);
  main_view.needs_measure_ = false;
  main_view.needs_align_ = false;
  main_view.needs_draw_ = false;
  for (auto* subview : main_view.GetSubviews()) {
    subview->needs_measure_ = false;
    subview->needs_align_ = false;
    subview->needs_draw_ = false;
  }

  view->SetTextSelection({0, 1});

  EXPECT_FALSE(main_view.needs_measure_);
  EXPECT_FALSE(main_view.needs_align_);
  EXPECT_TRUE(main_view.needs_draw_);
  for (auto* subview : main_view.GetSubviews()) {
    if (subview->view_name_.find("selection") != 0) {
      continue;
    }
    EXPECT_FALSE(subview->needs_measure_);
    EXPECT_FALSE(subview->needs_align_);
    EXPECT_TRUE(subview->needs_draw_);
    EXPECT_GT(subview->measured_size_.width_, 0);
    EXPECT_GT(subview->measured_size_.height_, 0);
  }
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
