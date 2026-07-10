// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "mock_markdown_platform_view.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view.h"
#include "testing/markdown/mock_platform/mock_markdown_canvas.h"

namespace serval::markdown::testing {
MockMarkdownPlatformView::MockMarkdownPlatformView(
    MarkdownContext* context, MockMarkdownPlatformView* parent)
    : parent_(parent), context_(context) {}
MockMarkdownMainView::MockMarkdownMainView(
    std::shared_ptr<MarkdownContext> context)
    : MockMarkdownCustomView(nullptr, nullptr) {
  main_context_ = context;
  MockMarkdownCustomView::AttachDrawable(
      std::make_unique<MarkdownView>(this, main_context_));
  context_ = main_context_.get();
  view_id_ = 0;
  view_name_ = "main";
}

void MockMarkdownPlatformView::RequestMeasure() {
  needs_measure_ = true;
  needs_align_ = true;
  needs_draw_ = true;
  if (parent_) {
    parent_->RequestMeasure();
  }
}

void MockMarkdownPlatformView::RequestAlign() {
  needs_align_ = true;
  needs_draw_ = true;
  if (parent_) {
    parent_->RequestAlign();
  }
}

void MockMarkdownPlatformView::RequestDraw() {
  needs_draw_ = true;
  if (parent_) {
    parent_->RequestDraw();
  }
}

void MockMarkdownPlatformView::Align(float left, float top) {
  needs_align_ = false;
  align_position_ = {left, top};
}

void MockMarkdownPlatformView::Draw(tttext::ICanvasHelper* canvas, float x,
                                    float y) {
  needs_draw_ = false;
}

PointF MockMarkdownPlatformView::GetAlignedPosition() {
  return align_position_;
}

SizeF MockMarkdownPlatformView::GetMeasuredSize() {
  return measured_size_;
}

void MockMarkdownPlatformView::SetMeasuredSize(SizeF size) {
  measured_size_ = size;
}

void MockMarkdownPlatformView::SetAlignPosition(PointF position) {
  align_position_ = position;
}

void MockMarkdownPlatformView::SetVisibility(bool visible) {
  if (!visible_ && visible) {
    RequestDraw();
  }
  visible_ = visible;
}

MeasureResult MockMarkdownPlatformView::OnMeasure(MeasureSpec spec) {
  needs_measure_ = false;
  return {.width_ = measured_size_.width_,
          .height_ = measured_size_.height_,
          .baseline_ = measured_size_.height_};
}

void MockMarkdownCustomView::AttachDrawable(
    std::shared_ptr<MarkdownDrawable> drawable) {
  MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
}

void MockMarkdownCustomView::Align(float left, float top) {
  MockMarkdownPlatformView::Align(left, top);
  if (drawable_ != nullptr) {
    drawable_->Align(left, top);
  }
}

void MockMarkdownCustomView::Draw(tttext::ICanvasHelper* canvas, float x,
                                  float y) {
  MockMarkdownPlatformView::Draw(canvas, x, y);
  reinterpret_cast<MockMarkdownCanvas*>(canvas)->BeginViewDraw(
      view_id_, view_name_.c_str());
  if (drawable_ != nullptr) {
    drawable_->Draw(canvas, x, y);
  }
  reinterpret_cast<MockMarkdownCanvas*>(canvas)->EndViewDraw();
}

SizeF MockMarkdownCustomView::GetMeasuredSize() {
  if (drawable_ == nullptr) {
    return {0, 0};
  }
  return {drawable_->GetAdvance(),
          drawable_->GetDescent() - drawable_->GetAscent()};
}

MeasureResult MockMarkdownCustomView::OnMeasure(MeasureSpec spec) {
  needs_measure_ = false;
  if (drawable_ == nullptr) {
    return {};
  }
  const auto result = drawable_->Measure(spec);
  SetMeasuredSize({.width_ = result.width_, .height_ = result.height_});
  return result;
}

void MockInlineView::Draw(tttext::ICanvasHelper* canvas, float x, float y) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawView(id_.c_str(), x, y,
                                                     x + width_, y + height_);
}

std::shared_ptr<MockMarkdownCustomView> MockMarkdownMainView::CreateSubview(
    bool insert_front) {
  auto subview = std::make_shared<MockMarkdownCustomView>(context_, this);
  if (insert_front) {
    subviews_.insert(subviews_.begin(), subview);
  } else {
    subviews_.push_back(subview);
  }
  return subview;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateCustomSubView() {
  const auto view = CreateSubview(false);
  view->view_name_ = "custom view";
  view->view_id_ = last_view_id_++;
  return view;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateRegionSubView() {
  const auto view = CreateSubview(true);
  view->view_name_ = "region view";
  view->view_id_ = last_view_id_++;
  return view;
}

std::shared_ptr<MockInlineView> MockMarkdownMainView::CreateInlineSubView(
    const char* id_selector, float max_width, float max_height) {
  auto subview = std::make_shared<MockInlineView>(
      context_, parent_, id_selector, max_width, max_height);
  subviews_.push_back(subview);
  return subview;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateSelectionHandleSubView(SelectionHandleType type,
                                                   float size, uint32_t color) {
  const auto view = CreateCustomSubView();
  reinterpret_cast<MockMarkdownPlatformView*>(view.get())->view_name_ =
      std::string("selection handle:") +
      (type == SelectionHandleType::kLeftHandle ? "left" : "right");
  auto* handle = view->GetCustomViewHandle();
  if (handle != nullptr) {
    auto drawable =
        std::make_unique<MarkdownSelectionHandle>(size, type, color);
    handle->AttachDrawable(std::move(drawable));
  }
  return view;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateSelectionHighlightSubView(uint32_t color) {
  const auto view = CreateCustomSubView();
  reinterpret_cast<MockMarkdownPlatformView*>(view.get())->view_name_ =
      "selection highlight";
  auto* handle = view->GetCustomViewHandle();
  if (handle != nullptr) {
    auto drawable = std::make_unique<MarkdownSelectionHighlight>();
    drawable->SetColor(color);
    handle->AttachDrawable(std::move(drawable));
  }
  return view;
}

void MockMarkdownMainView::RemoveSubView(MarkdownPlatformView* subview) {
  auto iter = std::find_if(
      subviews_.begin(), subviews_.end(),
      [subview](const std::shared_ptr<MockMarkdownPlatformView>& view) {
        return static_cast<MarkdownPlatformView*>(view.get()) == subview;
      });
  if (iter != subviews_.end()) {
    subviews_.erase(iter);
  }
}

void MockMarkdownMainView::RemoveAllSubViews() {
  subviews_.clear();
}

MarkdownView* MockMarkdownMainView::GetMarkdownView() const {
  return static_cast<MarkdownView*>(GetDrawable());
}

void MockMarkdownMainView::OnVSync(int64_t timestamp) const {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view != nullptr) {
    markdown_view->OnLayoutFrame(timestamp);
    markdown_view->OnRendererFrame(timestamp);
  }
}

MockMarkdownPlatformView* MockMarkdownMainView::FindSubviewByTop(
    float top) const {
  for (const auto& view : subviews_) {
    if (std::fabs(view->GetAlignedPosition().y_ - top) < 0.001f) {
      return view.get();
    }
  }
  return nullptr;
}

std::vector<MockMarkdownPlatformView*> MockMarkdownMainView::GetSubviews()
    const {
  std::vector<MockMarkdownPlatformView*> result;
  result.reserve(subviews_.size());
  for (const auto& view : subviews_) {
    result.push_back(view.get());
  }
  return result;
}

bool MockMarkdownMainView::ContainsSubview(
    MarkdownPlatformView* subview) const {
  return std::find_if(
             subviews_.begin(), subviews_.end(),
             [subview](const std::shared_ptr<MockMarkdownPlatformView>& view) {
               return static_cast<MarkdownPlatformView*>(view.get()) == subview;
             }) != subviews_.end();
}

MeasureResult MockMarkdownMainView::OnMeasure(MeasureSpec spec) {
  if (!needs_measure_ && last_spec_.width_ == spec.width_ &&
      last_spec_.height_ == spec.height_) {
    return measure_result_;
  }
  for (auto& sub : subviews_) {
    sub->Measure(spec);
  }
  return MockMarkdownCustomView::OnMeasure(spec);
}

void MockMarkdownMainView::Draw(tttext::ICanvasHelper* canvas, float x,
                                float y) {
  if (!needs_draw_)
    return;
  MockMarkdownCustomView::Draw(canvas, x, y);
  for (auto& sub : subviews_) {
    if (sub->needs_draw_) {
      canvas->Translate(sub->align_position_.x_, sub->align_position_.y_);
      sub->Draw(canvas, x, y);
      canvas->Translate(-sub->align_position_.x_, -sub->align_position_.y_);
    }
  }
}

}  // namespace serval::markdown::testing
