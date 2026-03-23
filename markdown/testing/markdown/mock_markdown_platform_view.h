// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_PLATFORM_VIEW_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_PLATFORM_VIEW_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/view/markdown_platform_view.h"

namespace serval::markdown {
class MarkdownView;
}

namespace serval::markdown::testing {

class MockMarkdownPlatformView : public MarkdownPlatformView {
 public:
  explicit MockMarkdownPlatformView() = default;
  ~MockMarkdownPlatformView() override = default;

  void RequestMeasure() override;
  void RequestAlign() override;
  void RequestDraw() override;

  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  virtual void DrawFromFrameDriver(tttext::ICanvasHelper* canvas, float x,
                                   float y) {
    Draw(canvas, x, y);
  }

  PointF GetAlignedPosition() override;
  SizeF GetMeasuredSize() override;

  void SetMeasuredSize(SizeF size) override;
  void SetAlignPosition(PointF position) override;
  void SetVisibility(bool visible) override;

  bool IsVisible() const { return visible_; }
  int32_t GetRequestMeasureCount() const { return request_measure_count_; }
  int32_t GetRequestAlignCount() const { return request_align_count_; }
  int32_t GetRequestDrawCount() const { return request_draw_count_; }
  int32_t GetDrawCount() const { return draw_count_; }
  bool HasDirty() const {
    return needs_measure_ || needs_align_ || needs_draw_;
  }
  bool TakeNeedsMeasure() { return std::exchange(needs_measure_, false); }
  bool TakeNeedsAlign() { return std::exchange(needs_align_, false); }
  bool TakeNeedsDraw() { return std::exchange(needs_draw_, false); }

  void ResetRequestCount() {
    request_measure_count_ = 0;
    request_align_count_ = 0;
    request_draw_count_ = 0;
  }
  void ResetDrawCount() {
    request_draw_count_ = 0;
    draw_count_ = 0;
  }

  bool DispatchTap(PointF position, GestureEventType event) {
    if (!tap_gesture_listener_) {
      return false;
    }
    return tap_gesture_listener_(position, event);
  }

  bool DispatchLongPress(PointF position, GestureEventType event) {
    if (!long_press_gesture_listener_) {
      return false;
    }
    return long_press_gesture_listener_(position, event);
  }

  bool DispatchPan(PointF position, PointF motion, GestureEventType event) {
    if (!pan_gesture_listener_) {
      return false;
    }
    return pan_gesture_listener_(position, motion, event);
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 protected:
  SizeF measured_size_{};
  PointF align_position_{};
  bool visible_{true};
  int32_t request_measure_count_{0};
  int32_t request_align_count_{0};
  int32_t request_draw_count_{0};
  int32_t draw_count_{0};
  bool needs_measure_{false};
  bool needs_align_{false};
  bool needs_draw_{false};
};

class MockMarkdownCustomView : public MockMarkdownPlatformView,
                               public MarkdownCustomViewHandle {
 public:
  explicit MockMarkdownCustomView() = default;
  ~MockMarkdownCustomView() override = default;

  void AttachDrawable(std::unique_ptr<MarkdownDrawable> drawable) override;
  void Align(float left, float top) override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  SizeF GetMeasuredSize() override;

  MarkdownCustomViewHandle* GetCustomViewHandle() override { return this; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
};

class MockInlineView : public MockMarkdownPlatformView {
 public:
  MockInlineView(const char* id_selector, float max_width, float max_height)
      : id_(id_selector), width_(max_width * 0.2f), height_(30) {
    (void)max_height;
  }
  ~MockInlineView() override = default;

  void DrawFromFrameDriver(tttext::ICanvasHelper* canvas, float x,
                           float y) override;

  std::string id_;
  float width_;
  float height_;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {.width_ = width_, .height_ = height_, .baseline_ = height_};
  }
};

class MockMarkdownMainView : public MockMarkdownCustomView,
                             public MarkdownViewContainerHandle {
 public:
  MockMarkdownMainView();
  ~MockMarkdownMainView() override = default;

  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin,
      uint32_t color) override;
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
  MarkdownView* GetMarkdownView();
  void OnVSync(int64_t timestamp);

  void SetViewRectInScreen(RectF rect) { cached_view_rect_in_screen_ = rect; }

  size_t GetSubviewCount() const { return subviews_.size(); }
  MockMarkdownPlatformView* FindSubviewByTop(float top) const;
  std::vector<MockMarkdownPlatformView*> GetSubviews() const;
  void ResetDrawCount() const;
  bool ContainsSubview(MarkdownPlatformView* subview) const;

 private:
  static RectF MakeDefaultViewRectInScreen() {
    const float max = MeasureSpec::LAYOUT_MAX_SIZE;
    return RectF::MakeLTRB(0, 0, max, max);
  }
  std::shared_ptr<MockMarkdownCustomView> CreateSubview(bool insert_front);

 private:
  std::vector<std::shared_ptr<MockMarkdownPlatformView>> subviews_;
  RectF cached_view_rect_in_screen_{MakeDefaultViewRectInScreen()};
};

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_PLATFORM_VIEW_H_
