// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_PLATFORM_MOCK_MARKDOWN_PLATFORM_VIEW_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_PLATFORM_MOCK_MARKDOWN_PLATFORM_VIEW_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/element/markdown_context.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_view.h"
#include "markdown/view/markdown_view_gesture.h"
#include "markdown/view/markdown_view_measure_host.h"

namespace serval::markdown {
class MarkdownView;
}

namespace serval::markdown::testing {

class MockMarkdownPlatformView : public MarkdownPlatformView {
  friend class MockMarkdownMainView;

 public:
  explicit MockMarkdownPlatformView(MarkdownContext* context,
                                    MockMarkdownPlatformView* parent);
  ~MockMarkdownPlatformView() override = default;

  void RequestDraw() override;

  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  PointF GetAlignedPosition() override;
  SizeF GetMeasuredSize() override;

  void SetMeasuredSize(SizeF size) override;
  void SetAlignPosition(PointF position) override;
  void SetVisibility(bool visible) override;

  bool IsVisible() const { return visible_; }

  bool HasDirty() const {
    return needs_measure_ || needs_align_ || needs_draw_;
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 public:
  MockMarkdownPlatformView* parent_{nullptr};
  SizeF measured_size_{};
  PointF align_position_{};
  bool visible_{true};

  bool needs_measure_{false};
  bool needs_align_{false};
  bool needs_draw_{false};

  int32_t view_id_{};
  std::string view_name_{};

  MarkdownContext* context_;
};

class MockMarkdownCustomView : public MockMarkdownPlatformView,
                               public MarkdownCustomViewHandle {
 public:
  explicit MockMarkdownCustomView(MarkdownContext* context,
                                  MockMarkdownPlatformView* parent)
      : MockMarkdownPlatformView(context, parent) {}
  ~MockMarkdownCustomView() override = default;

  void AttachDrawable(std::shared_ptr<MarkdownDrawable> drawable) override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  SizeF GetMeasuredSize() override;

  MarkdownCustomViewHandle* GetCustomViewHandle() override { return this; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
};

class MockInlineView : public MockMarkdownPlatformView {
 public:
  MockInlineView(MarkdownContext* context, MockMarkdownPlatformView* parent,
                 const char* id_selector, float max_width, float max_height)
      : MockMarkdownPlatformView(context, parent),
        id_(id_selector),
        width_(max_width * 0.2f),
        height_(30) {
    (void)max_height;
  }
  ~MockInlineView() override = default;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  std::string id_;
  float width_;
  float height_;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {.width_ = width_, .height_ = height_, .baseline_ = height_};
  }
};

class MockMarkdownMainView : public MockMarkdownCustomView,
                             public MarkdownViewContainerHandle,
                             public MarkdownViewMeasureHost {
  friend class MarkdownFrameDriver;

 public:
  MockMarkdownMainView(std::shared_ptr<MarkdownContext> context);
  ~MockMarkdownMainView() override = default;

  void RequestMeasure() override;
  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, uint32_t color) override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override;
  std::shared_ptr<MockInlineView> CreateInlineSubView(const char* id_selector,
                                                      float max_width,
                                                      float max_height);
  void RemoveSubView(MarkdownPlatformView* subview) override;
  void RemoveAllSubViews() override;

  RectF GetViewRectInScreen() override { return cached_view_rect_in_screen_; }
  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return this;
  }
  MarkdownView* GetMarkdownView() const;
  void OnVSync(int64_t timestamp) const;

  bool OnLongPress(PointF position, GestureEventType event) const {
    return GetMarkdownView()->OnLongPress(position, event);
  }
  bool OnTap(PointF position, GestureEventType event) const {
    return GetMarkdownView()->OnTap(position, event);
  }
  bool ShouldBeginPan(PointF position, PointF motion) const {
    return GetMarkdownView()->ShouldBeginPan(position, motion);
  }
  bool OnPan(PointF position, PointF motion, GestureEventType event) const {
    return GetMarkdownView()->OnPan(position, motion, event);
  }

  void SetViewRectInScreen(RectF rect) {
    cached_view_rect_in_screen_ = rect;
    RequestDraw();
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 public:
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  size_t GetSubviewCount() const { return subviews_.size(); }
  MockMarkdownPlatformView* FindSubviewByTop(float top) const;
  std::vector<MockMarkdownPlatformView*> GetSubviews() const;
  bool ContainsSubview(MarkdownPlatformView* subview) const;

  static RectF MakeDefaultViewRectInScreen() {
    const float max = MeasureSpec::LAYOUT_MAX_SIZE;
    return RectF::MakeLTRB(0, 0, max, max);
  }
  std::shared_ptr<MockMarkdownCustomView> CreateSubview(bool insert_front);

  std::vector<std::shared_ptr<MockMarkdownPlatformView>> subviews_;
  RectF cached_view_rect_in_screen_{MakeDefaultViewRectInScreen()};

  std::shared_ptr<MarkdownContext> main_context_;
  MeasureSpec last_spec_;
  int32_t last_view_id_ = 1;
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_PLATFORM_MOCK_MARKDOWN_PLATFORM_VIEW_H_
