// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_PLATFORM_VIEW_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_PLATFORM_VIEW_H_

#include <memory>
#include <vector>

#include "markdown/element/markdown_context.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_view_measure_host.h"

namespace serval::markdown::example {

class SkityDemoPlatformView : public MarkdownPlatformView,
                              public MarkdownCustomViewHandle {
 public:
  SkityDemoPlatformView() = default;
  ~SkityDemoPlatformView() override = default;

  void AttachDrawable(std::shared_ptr<MarkdownDrawable> drawable) override;

  void RequestDraw() override;

  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  PointF GetAlignedPosition() override;
  SizeF GetMeasuredSize() override;
  void SetMeasuredSize(SizeF size) override;
  void SetAlignPosition(PointF position) override;
  void SetVisibility(bool visible) override;
  MarkdownCustomViewHandle* GetCustomViewHandle() override;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 private:
  PointF align_position_;
  SizeF measured_size_;
  bool visible_{true};
};

class SkityDemoMainView final : public SkityDemoPlatformView,
                                public MarkdownViewContainerHandle,
                                public MarkdownViewMeasureHost {
 public:
  SkityDemoMainView();
  ~SkityDemoMainView() override = default;

  void RequestMeasure() override;
  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateScrollXRegionView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, uint32_t color) override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override;

  void RemoveSubView(MarkdownPlatformView* subview) override;
  void RemoveAllSubViews() override;
  RectF GetViewRectInScreen() override;
  MarkdownViewContainerHandle* GetViewContainerHandle() override;

  void SetMarkdownContext(MarkdownContext* context);
  void SetViewRectInScreen(RectF rect);
  bool TakeNeedsMeasure();

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 private:
  static void RemoveSubViewFrom(
      std::vector<std::shared_ptr<MarkdownPlatformView>>* views,
      MarkdownPlatformView* subview);
  static void DrawSubViews(
      tttext::ICanvasHelper* canvas,
      const std::vector<std::shared_ptr<MarkdownPlatformView>>& views, float x,
      float y);

  std::vector<std::shared_ptr<MarkdownPlatformView>> region_views_;
  std::vector<std::shared_ptr<MarkdownPlatformView>> overlay_views_;
  RectF view_rect_in_screen_;
  MarkdownContext* markdown_context_{nullptr};
  bool needs_measure_{true};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_PLATFORM_VIEW_H_
