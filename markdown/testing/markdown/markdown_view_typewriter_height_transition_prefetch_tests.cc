// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include "markdown/markdown_platform_loader.h"
#include "markdown/view/markdown_view.h"

namespace lynx::markdown {

class TestViewContainerHandle : public MarkdownViewContainerHandle {
 public:
  MarkdownPlatformView* CreateCustomSubView() override { return nullptr; }
  void RemoveSubView(MarkdownPlatformView* subview) override {}
  void RemoveAllSubViews() override {}
  RectF GetViewRectInScreen() override { return {}; }
};

class TestPlatformView : public MarkdownPlatformView {
 public:
  void RequestMeasure() override {}
  void RequestAlign() override {}
  void RequestDraw() override {}

  SizeF Measure(MeasureSpec spec) override { return measured_size_; }
  void Align(float left, float top) override { align_position_ = {left, top}; }
  void Draw(tttext::ICanvasHelper* canvas) override {}

  PointF GetAlignedPosition() override { return align_position_; }
  SizeF GetMeasuredSize() override { return measured_size_; }

  void SetMeasuredSize(SizeF size) override { measured_size_ = size; }
  void SetAlignPosition(PointF position) override {
    align_position_ = position;
  }

  void SetVisibility(bool visible) override {}

  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return &container_handle_;
  }

 private:
  TestViewContainerHandle container_handle_;
  SizeF measured_size_{};
  PointF align_position_{};
};

class TestPlatformLoader : public MarkdownPlatformLoader {
 public:
  MarkdownPlatformView* LoadImageView(const char* src, float desire_width,
                                      float desire_height, float max_width,
                                      float max_height,
                                      float border_radius) override {
    return nullptr;
  }
  MarkdownPlatformView* LoadInlineView(const char* id_selector, float max_width,
                                       float max_height) override {
    return nullptr;
  }
  void* LoadFont(const char* family, MarkdownFontWeight weight) override {
    return nullptr;
  }
  MarkdownPlatformView* LoadReplacementView(void* ud, int32_t id,
                                            float max_width,
                                            float max_height) override {
    return nullptr;
  }
};

TEST(MarkdownViewTest,
     TypewriterHeightTransitionPrefetchTargetsTransitionEndHeight) {
  TestPlatformView platform_view;
  MarkdownView view(&platform_view);
  TestPlatformLoader loader;
  view.SetPlatformLoader(&loader);
  view.SetSourceType(SourceType::kPlainText);
  view.SetContent("1234567890\n1234567890\n1234567890\n");
  view.SetAnimationType(MarkdownAnimationType::kTypewriter);
  view.SetTypewriterDynamicHeight(true);
  view.SetAnimationVelocity(100.0f);

  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;

  view.SetHeightTransitionDuration(0.0f);
  view.SetAnimationStep(10);
  const auto size10 = view.Measure(spec);
  view.SetAnimationStep(20);
  const auto size20 = view.Measure(spec);

  ASSERT_GT(size20.height_, size10.height_);

  view.SetAnimationStep(10);
  view.SetHeightTransitionDuration(0.1f);
  view.SetTypewriterHeightTransitionPrefetch(true);
  const auto size_prefetch = view.Measure(spec);
  EXPECT_NEAR(size_prefetch.height_, size20.height_, 0.01f);
}

}  // namespace lynx::markdown
