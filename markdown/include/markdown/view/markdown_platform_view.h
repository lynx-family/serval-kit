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
  virtual std::shared_ptr<MarkdownPlatformView> CreateScrollXRegionView() {
    return CreateRegionSubView();
  }
  virtual std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, uint32_t color) = 0;
  virtual std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) = 0;
  virtual void RemoveSubView(MarkdownPlatformView* subview) = 0;
  virtual void RemoveAllSubViews() = 0;
  // Host-visible clip in view-local layout units (points on iOS, physical pixels
  // on Android/Harmony). The renderer intersects this rectangle with the bounds
  // of its current document snapshot.
  virtual RectF GetViewRectInScreen() = 0;
};

class MarkdownCustomViewHandle {
 public:
  virtual ~MarkdownCustomViewHandle() = default;
  virtual void AttachDrawable(std::shared_ptr<MarkdownDrawable> drawable) {
    drawable_ = std::move(drawable);
  }
  MarkdownDrawable* GetDrawable() const { return drawable_.get(); }

 protected:
  std::shared_ptr<MarkdownDrawable> drawable_{nullptr};
};

class MarkdownPlatformView : public MarkdownDrawable {
 public:
  ~MarkdownPlatformView() override = default;

  virtual void RequestDraw() = 0;

  void Align(float left, float top) override = 0;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override = 0;

  // Render-thread API. Updating a native view must not re-enter measure/align.
  void SetBounds(RectF bounds) final {
    bounds_ = bounds;
    if (!CheckValidBounds(bounds)) {
      return;
    }
    OnSetBounds(bounds);
  }
  const RectF& GetBounds() const { return bounds_; }

  virtual void SetVisibility(bool visible) = 0;

  virtual MarkdownViewContainerHandle* GetViewContainerHandle() {
    return nullptr;
  }
  virtual MarkdownCustomViewHandle* GetCustomViewHandle() { return nullptr; }

 protected:
  virtual void OnSetBounds(RectF bounds) = 0;
  bool CheckValidBounds(RectF bounds) {
    const auto valid = !bounds.IsEmpty();
    if (valid != valid_bounds_) {
      valid_bounds_ = valid;
      SetVisibility(valid);
    }
    return valid_bounds_;
  }
  MeasureResult OnMeasure(MeasureSpec spec) override = 0;
  bool valid_bounds_ = true;

 private:
  RectF bounds_{};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_PLATFORM_VIEW_H_
