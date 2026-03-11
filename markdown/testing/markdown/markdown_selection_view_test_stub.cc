// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "markdown/view/markdown_selection_view.h"

namespace lynx::markdown {

class StubCustomViewHandle : public MarkdownCustomViewHandle {};

class StubPlatformView : public MarkdownPlatformView {
 public:
  void RequestMeasure() override {}
  void RequestAlign() override {}
  void RequestDraw() override {}

  void Align(float left, float top) override { align_position_ = {left, top}; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {}

  PointF GetAlignedPosition() override { return align_position_; }
  SizeF GetMeasuredSize() override { return measured_size_; }

  void SetMeasuredSize(SizeF size) override { measured_size_ = size; }
  void SetAlignPosition(PointF position) override {
    align_position_ = position;
  }

  void SetVisibility(bool visible) override {}

  MarkdownCustomViewHandle* GetCustomViewHandle() override {
    return &custom_view_handle_;
  }

 private:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {.width_ = measured_size_.width_,
            .height_ = measured_size_.height_,
            .baseline_ = measured_size_.height_};
  }

  StubCustomViewHandle custom_view_handle_;
  SizeF measured_size_{};
  PointF align_position_{};
};

MarkdownPlatformView* MarkdownSelectionHandle::CreateView(
    MarkdownViewContainerHandle* parent, SelectionHandleType type, float size,
    float margin, uint32_t color) {
  auto* view = new StubPlatformView();
  view->GetCustomViewHandle()->AttachDrawable(
      std::make_unique<MarkdownSelectionHandle>(size, margin, type, color));
  return view;
}

MarkdownPlatformView* MarkdownSelectionHighlight::CreateView(
    MarkdownViewContainerHandle* parent, uint32_t color) {
  auto* view = new StubPlatformView();
  auto drawable = std::make_unique<MarkdownSelectionHighlight>();
  drawable->SetColor(color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(drawable));
  return view;
}

}  // namespace lynx::markdown
