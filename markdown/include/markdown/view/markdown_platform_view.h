// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
#include <cstdint>
#include <memory>
#include <utility>

#include "markdown/element/markdown_drawable.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
#include "markdown/view/markdown_gesture.h"
namespace serval::markdown {

class MarkdownPlatformView;
enum class SelectionHandleType : uint8_t;
class MarkdownViewContainerHandle {
 public:
  virtual ~MarkdownViewContainerHandle() = default;
  virtual std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() = 0;
  virtual std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() {
    return CreateCustomSubView();
  }
  virtual std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin, uint32_t color) = 0;
  virtual std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) = 0;
  virtual void RemoveSubView(MarkdownPlatformView* subview) = 0;
  virtual void RemoveAllSubViews() = 0;
  virtual RectF GetViewRectInScreen() = 0;
};

class MarkdownCustomViewHandle {
 public:
  virtual ~MarkdownCustomViewHandle() = default;
  virtual void AttachDrawable(std::unique_ptr<MarkdownDrawable> drawable) {
    drawable_ = std::move(drawable);
  }
  MarkdownDrawable* GetDrawable() const { return drawable_.get(); }

 protected:
  std::unique_ptr<MarkdownDrawable> drawable_{nullptr};
};

class MarkdownPlatformView : public MarkdownDrawable {
 public:
  ~MarkdownPlatformView() override = default;

  virtual void RequestMeasure() = 0;
  virtual void RequestAlign() = 0;
  virtual void RequestDraw() = 0;

  void Align(float left, float top) override = 0;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override = 0;

  virtual PointF GetAlignedPosition() = 0;
  virtual SizeF GetMeasuredSize() = 0;

  virtual void SetMeasuredSize(SizeF size) = 0;
  virtual void SetAlignPosition(PointF position) = 0;

  virtual void SetVisibility(bool visible) = 0;

  void SetTapListener(TapGestureListener tap_gesture_listener) {
    tap_gesture_listener_ = tap_gesture_listener;
  }
  void SetLongPressListener(
      LongPressGestureListener long_press_gesture_listener) {
    long_press_gesture_listener_ = long_press_gesture_listener;
  }
  void SetPanGestureListener(PanGestureListener pan_gesture_listener) {
    pan_gesture_listener_ = pan_gesture_listener;
  }

  virtual MarkdownViewContainerHandle* GetViewContainerHandle() {
    return nullptr;
  }
  virtual MarkdownCustomViewHandle* GetCustomViewHandle() { return nullptr; }

 protected:
  TapGestureListener tap_gesture_listener_{};
  LongPressGestureListener long_press_gesture_listener_{};
  PanGestureListener pan_gesture_listener_{};

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override = 0;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
