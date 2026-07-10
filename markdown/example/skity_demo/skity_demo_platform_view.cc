// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "skity_demo/skity_demo_platform_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "markdown/view/markdown_selection_view.h"
#include "skity_demo/skity_demo_config.h"

namespace serval::markdown::example {

void SkityDemoPlatformView::AttachDrawable(
    std::shared_ptr<MarkdownDrawable> drawable) {
  MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
}

void SkityDemoPlatformView::RequestMeasure() {
  needs_measure_ = true;
}

void SkityDemoPlatformView::RequestAlign() {
  needs_align_ = true;
}

void SkityDemoPlatformView::RequestDraw() {}

void SkityDemoPlatformView::Align(float left, float top) {
  align_position_ = {left, top};
  if (drawable_ != nullptr) {
    drawable_->Align(left, top);
  }
}

void SkityDemoPlatformView::Draw(tttext::ICanvasHelper* canvas, float x,
                                 float y) {
  if (visible_ && drawable_ != nullptr) {
    canvas->Save();
    canvas->Translate(x, y);
    if (measured_size_.width_ > 0 && measured_size_.height_ > 0) {
      canvas->ClipRect(0, 0, measured_size_.width_, measured_size_.height_,
                       true);
    }
    drawable_->Draw(canvas, 0, 0);
    canvas->Restore();
  }
}

PointF SkityDemoPlatformView::GetAlignedPosition() {
  return align_position_;
}

SizeF SkityDemoPlatformView::GetMeasuredSize() {
  return measured_size_;
}

void SkityDemoPlatformView::SetMeasuredSize(SizeF size) {
  measured_size_ = size;
}

void SkityDemoPlatformView::SetAlignPosition(PointF position) {
  align_position_ = position;
}

void SkityDemoPlatformView::SetVisibility(bool visible) {
  visible_ = visible;
}

MarkdownCustomViewHandle* SkityDemoPlatformView::GetCustomViewHandle() {
  return this;
}

bool SkityDemoPlatformView::TakeNeedsMeasure() {
  const bool needs_measure = needs_measure_;
  needs_measure_ = false;
  return needs_measure;
}

bool SkityDemoPlatformView::TakeNeedsAlign() {
  const bool needs_align = needs_align_;
  needs_align_ = false;
  return needs_align;
}

MeasureResult SkityDemoPlatformView::OnMeasure(MeasureSpec spec) {
  if (drawable_ == nullptr) {
    return {};
  }
  const auto result = drawable_->Measure(spec);
  measured_size_ = {result.width_, result.height_};
  return result;
}

SkityDemoMainView::SkityDemoMainView()
    : view_rect_in_screen_(
          RectF::MakeLTWH(0, 0, kSkityDemoWidth, kSkityDemoHeight)) {}

std::shared_ptr<MarkdownPlatformView> SkityDemoMainView::CreateCustomSubView() {
  auto view = std::make_shared<SkityDemoPlatformView>();
  overlay_views_.emplace_back(view);
  return view;
}

std::shared_ptr<MarkdownPlatformView> SkityDemoMainView::CreateRegionSubView() {
  auto view = std::make_shared<SkityDemoPlatformView>();
  region_views_.emplace_back(view);
  return view;
}

std::shared_ptr<MarkdownPlatformView>
SkityDemoMainView::CreateScrollXRegionView() {
  return CreateRegionSubView();
}

std::shared_ptr<MarkdownPlatformView>
SkityDemoMainView::CreateSelectionHandleSubView(SelectionHandleType type,
                                                float size, uint32_t color) {
  auto view =
      std::static_pointer_cast<SkityDemoPlatformView>(CreateCustomSubView());
  auto handle = std::make_shared<MarkdownSelectionHandle>(
      size, type, color, SelectionHandleShape::kWaterDrop);
  handle->SetMarkdownContext(markdown_context_);
  view->AttachDrawable(std::move(handle));
  return view;
}

std::shared_ptr<MarkdownPlatformView>
SkityDemoMainView::CreateSelectionHighlightSubView(uint32_t color) {
  auto view =
      std::static_pointer_cast<SkityDemoPlatformView>(CreateCustomSubView());
  auto highlight = std::make_shared<MarkdownSelectionHighlight>();
  highlight->SetColor(color);
  view->AttachDrawable(std::move(highlight));
  return view;
}

void SkityDemoMainView::RemoveSubView(MarkdownPlatformView* subview) {
  RemoveSubViewFrom(&region_views_, subview);
  RemoveSubViewFrom(&overlay_views_, subview);
}

void SkityDemoMainView::RemoveAllSubViews() {
  region_views_.clear();
  overlay_views_.clear();
}

RectF SkityDemoMainView::GetViewRectInScreen() {
  return view_rect_in_screen_;
}

MarkdownViewContainerHandle* SkityDemoMainView::GetViewContainerHandle() {
  return this;
}

void SkityDemoMainView::SetMarkdownContext(MarkdownContext* context) {
  markdown_context_ = context;
}

void SkityDemoMainView::SetViewRectInScreen(RectF rect) {
  view_rect_in_screen_ = rect;
}

void SkityDemoMainView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  SkityDemoPlatformView::Draw(canvas, x, y);
  DrawSubViews(canvas, region_views_, x, y);
  DrawSubViews(canvas, overlay_views_, x, y);
}

void SkityDemoMainView::RemoveSubViewFrom(
    std::vector<std::shared_ptr<MarkdownPlatformView>>* views,
    MarkdownPlatformView* subview) {
  if (views == nullptr) {
    return;
  }
  views->erase(std::remove_if(views->begin(), views->end(),
                              [subview](const auto& item) {
                                return item.get() == subview;
                              }),
               views->end());
}

void SkityDemoMainView::DrawSubViews(
    tttext::ICanvasHelper* canvas,
    const std::vector<std::shared_ptr<MarkdownPlatformView>>& views, float x,
    float y) {
  for (const auto& subview : views) {
    if (subview != nullptr) {
      const auto position = subview->GetAlignedPosition();
      subview->Draw(canvas, x + position.x_, y + position.y_);
    }
  }
}

}  // namespace serval::markdown::example
