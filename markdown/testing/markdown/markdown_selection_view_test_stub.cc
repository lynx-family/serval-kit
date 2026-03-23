// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "markdown/view/markdown_selection_view.h"

namespace serval::markdown {

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

class StubViewContainerHandle : public MarkdownViewContainerHandle {
 public:
  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override {
    auto view = std::make_shared<StubPlatformView>();
    subviews_.push_back(view);
    return view;
  }

  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin,
      uint32_t color) override {
    const auto view = CreateCustomSubView();
    auto drawable =
        std::make_unique<MarkdownSelectionHandle>(size, margin, type, color);
    view->GetCustomViewHandle()->AttachDrawable(std::move(drawable));
    return view;
  }

  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override {
    const auto view = CreateCustomSubView();
    auto drawable = std::make_unique<MarkdownSelectionHighlight>();
    drawable->SetColor(color);
    view->GetCustomViewHandle()->AttachDrawable(std::move(drawable));
    return view;
  }

  void RemoveSubView(MarkdownPlatformView* subview) override {
    for (auto iter = subviews_.begin(); iter != subviews_.end(); ++iter) {
      if (iter->get() == subview) {
        subviews_.erase(iter);
        return;
      }
    }
  }

  void RemoveAllSubViews() override { subviews_.clear(); }

  RectF GetViewRectInScreen() override { return {}; }

 private:
  std::vector<std::shared_ptr<MarkdownPlatformView>> subviews_;
};

}  // namespace serval::markdown
