// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/markdown/mock_markdown_platform_view.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view.h"
#include "testing/markdown/mock_markdown_canvas.h"

namespace lynx::markdown::testing {

MockMarkdownMainView::MockMarkdownMainView() {
  MockMarkdownCustomView::AttachDrawable(std::make_unique<MarkdownView>(this));
}

void MockMarkdownPlatformView::RequestMeasure() {
  request_measure_count_++;
  needs_measure_ = true;
  needs_align_ = true;
  needs_draw_ = true;
}

void MockMarkdownPlatformView::RequestAlign() {
  request_align_count_++;
  needs_align_ = true;
  needs_draw_ = true;
}

void MockMarkdownPlatformView::RequestDraw() {
  request_draw_count_++;
  needs_draw_ = true;
}

void MockMarkdownPlatformView::Align(float left, float top) {
  align_position_ = {left, top};
}

void MockMarkdownPlatformView::Draw(tttext::ICanvasHelper* canvas, float x,
                                    float y) {
  visible_ = true;
  draw_count_++;
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
    needs_draw_ = true;
  }
  visible_ = visible;
}

MeasureResult MockMarkdownPlatformView::OnMeasure(MeasureSpec spec) {
  return {.width_ = measured_size_.width_,
          .height_ = measured_size_.height_,
          .baseline_ = measured_size_.height_};
}

void MockMarkdownCustomView::AttachDrawable(
    std::unique_ptr<MarkdownDrawable> drawable) {
  MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
}

void MockMarkdownCustomView::Align(float left, float top) {
  SetAlignPosition({left, top});
  if (drawable_ != nullptr) {
    drawable_->Align(left, top);
  }
}

void MockMarkdownCustomView::Draw(tttext::ICanvasHelper* canvas, float x,
                                  float y) {
  MockMarkdownPlatformView::Draw(canvas, x, y);
  if (drawable_ != nullptr) {
    drawable_->Draw(canvas, x, y);
  }
}

SizeF MockMarkdownCustomView::GetMeasuredSize() {
  if (drawable_ == nullptr) {
    return MockMarkdownPlatformView::GetMeasuredSize();
  }
  return {drawable_->GetAdvance(),
          drawable_->GetDescent() - drawable_->GetAscent()};
}

MeasureResult MockMarkdownCustomView::OnMeasure(MeasureSpec spec) {
  if (drawable_ == nullptr) {
    return {};
  }
  const auto result = drawable_->Measure(spec);
  SetMeasuredSize({.width_ = result.width_, .height_ = result.height_});
  return result;
}

void MockInlineView::DrawFromFrameDriver(tttext::ICanvasHelper* canvas, float x,
                                         float y) {
  static_cast<MockMarkdownCanvas*>(canvas)->DrawView(id_.c_str(), x, y,
                                                     x + width_, y + height_);
  lynx::markdown::testing::MockMarkdownPlatformView::Draw(canvas, x, y);
}

std::shared_ptr<MockMarkdownCustomView> MockMarkdownMainView::CreateSubview(
    bool insert_front) {
  auto subview = std::make_shared<MockMarkdownCustomView>();
  if (insert_front) {
    subviews_.insert(subviews_.begin(), subview);
  } else {
    subviews_.push_back(subview);
  }
  return subview;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateCustomSubView() {
  return CreateSubview(false);
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateRegionSubView() {
  return CreateSubview(true);
}

std::shared_ptr<MockInlineView> MockMarkdownMainView::CreateInlineSubView(
    const char* id_selector, float max_width, float max_height) {
  auto subview =
      std::make_shared<MockInlineView>(id_selector, max_width, max_height);
  subviews_.push_back(subview);
  return subview;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateSelectionHandleSubView(SelectionHandleType type,
                                                   float size, float margin,
                                                   uint32_t color) {
  const auto view = CreateCustomSubView();
  auto* handle = view->GetCustomViewHandle();
  if (handle != nullptr) {
    auto drawable =
        std::make_unique<MarkdownSelectionHandle>(size, margin, type, color);
    handle->AttachDrawable(std::move(drawable));
  }
  return view;
}

std::shared_ptr<MarkdownPlatformView>
MockMarkdownMainView::CreateSelectionHighlightSubView(uint32_t color) {
  const auto view = CreateCustomSubView();
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

MarkdownView* MockMarkdownMainView::GetMarkdownView() {
  return static_cast<MarkdownView*>(GetDrawable());
}

void MockMarkdownMainView::OnVSync(int64_t timestamp) {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view != nullptr) {
    markdown_view->OnNextFrame(timestamp);
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

void MockMarkdownMainView::ResetDrawCount() const {
  for (const auto& view : subviews_) {
    view->ResetDrawCount();
  }
}

bool MockMarkdownMainView::ContainsSubview(
    MarkdownPlatformView* subview) const {
  return std::find_if(
             subviews_.begin(), subviews_.end(),
             [subview](const std::shared_ptr<MockMarkdownPlatformView>& view) {
               return static_cast<MarkdownPlatformView*>(view.get()) == subview;
             }) != subviews_.end();
}

}  // namespace lynx::markdown::testing
