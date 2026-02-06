// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
#include <memory>
#include <utility>

#include "markdown/element/markdown_drawable.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
#include "markdown/view/markdown_gesture.h"
namespace lynx::markdown {

class MarkdownPlatformView;
class MarkdownViewContainerHandle {
 public:
  virtual ~MarkdownViewContainerHandle() = default;
  virtual MarkdownPlatformView* CreateCustomSubView() = 0;
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

class MarkdownPlatformView {
 public:
  virtual ~MarkdownPlatformView() = default;

  virtual void RequestMeasure() = 0;
  virtual void RequestAlign() = 0;
  virtual void RequestDraw() = 0;

  virtual SizeF Measure(MeasureSpec spec) = 0;
  virtual void Align(float left, float top) = 0;
  virtual void Draw(tttext::ICanvasHelper* canvas) = 0;

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
};

class MarkdownViewDelegate : public MarkdownDrawable {
 public:
  explicit MarkdownViewDelegate(MarkdownPlatformView* view, float max_width,
                                float max_height, float font_size = 0)
      : view_(view),
        max_width_(max_width),
        max_height_(max_height),
        font_size_(font_size) {}

  float GetWidth() const override;
  float GetHeight() const override;
  float GetBaseLine() const override;
  SizeF Measure(MeasureSpec spec) override;
  void Draw(tttext::ICanvasHelper* canvas, float left, float top, float right,
            float bottom) override;
  void Align(float x, float y) override;
  MarkdownPlatformView* GetPlatformView() const { return view_; }

 protected:
  MarkdownPlatformView* view_;
  SizeF size_;
  float max_width_;
  float max_height_;
  float font_size_;
};

class MarkdownBlockViewDelegate : public MarkdownViewDelegate {
 public:
  explicit MarkdownBlockViewDelegate(MarkdownPlatformView* view,
                                     float max_width, float max_height)
      : MarkdownViewDelegate(view, max_width, max_height, 0) {}
  void Draw(tttext::ICanvasHelper* canvas, float left, float top, float right,
            float bottom) override;
};

}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
