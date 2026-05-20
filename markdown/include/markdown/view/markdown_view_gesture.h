// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_GESTURE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_GESTURE_H_
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "markdown/layout/markdown_selection.h"
#include "markdown/utils/markdown_definition.h"
namespace serval::markdown {
class MarkdownContext;
class MarkdownDocument;
class MarkdownEventListener;
class MarkdownPlatformView;
class MarkdownSelectionHandle;
class MarkdownSelectionHighlight;
class MarkdownViewContainerHandle;
class MarkdownViewRenderer;
enum class SelectionHandleType : uint8_t;
enum class SelectionState;

enum class GestureEventType : uint8_t {
  kUnknown = 0,
  kDown = 1,
  kMove = 2,
  kUp = 3,
  kCancel = 4,
};

class MarkdownViewGesture {
 public:
  explicit MarkdownViewGesture(MarkdownViewContainerHandle* handle = nullptr,
                               MarkdownContext* context = nullptr,
                               MarkdownViewRenderer* renderer = nullptr);
  void SetRenderer(MarkdownViewRenderer* renderer);
  void SetDocument(std::shared_ptr<MarkdownDocument> document);
  void SetEventListener(MarkdownEventListener* listener);

  void SetEnableSelection(bool enable_selection);
  void SetSelectionHandleSize(float size);
  void SetSelectionHandleTouchMargin(float margin);
  void SetSelectionHandleColor(uint32_t color);
  void SetSelectionHighlightColor(uint32_t color);
  void SetTextSelection(Range char_range, int32_t visible_char_count);

  Range GetSelectedRange() const;
  std::string GetSelectedText() const;
  const std::vector<RectF>& GetSelectedLineBoundingRect() const;
  PointF GetSelectionHandlePosition() const;
  float GetSelectionHandleRadius() const;

  bool OnTap(PointF position, GestureEventType event);
  bool OnLongPress(PointF position, GestureEventType event);
  bool ShouldBeginPan(PointF position, PointF motion);
  bool OnPan(PointF position, PointF motion, GestureEventType event);
  void OnScrollXRegionChanged();

  int32_t GetCharIndexByPosition(PointF position) const;
  Range GetCharRangeByPosition(
      PointF position, MarkdownSelection::CharRangeType char_range_type) const;
  std::vector<RectF> GetTextLineBoundingRect(Range range) const;
  RectF GetTextBoundingRect(Range range) const;
  std::string GetParsedContent(Range char_range) const;

 private:
  enum class PanTarget : uint8_t {
    kNone,
    kSelectionStartHandle,
    kSelectionEndHandle,
    kScrollXRegion,
  };
  struct PanHitTestResult {
    PanTarget target_{PanTarget::kNone};
    uint32_t scroll_x_region_index_{0};
    float scroll_x_origin_offset_{0};
  };

  PanHitTestResult HitTestPanTarget(PointF position, PointF motion) const;
  static bool IsHorizontalPan(PointF motion);
  bool CanBeginLongPressSelectionDrag(PointF position, PointF motion) const;
  void ClearLongPressSelectionDrag();
  bool BeginPan(PointF position, PointF motion);
  bool UpdatePan(PointF position, PointF motion);
  bool EndPan(PointF position, PointF motion, GestureEventType event);
  bool DispatchSelectionHandlePan(PointF position, PointF motion,
                                  GestureEventType event);
  void ResetPan();

  bool HasSelectionViews() const;
  std::shared_ptr<MarkdownPage> GetPage() const;
  void EnterSelection(PointF position);
  void ExitSelection();
  bool HitTestSelectionHandle(PointF position,
                              SelectionHandleType* handle_type) const;
  void UpdateSelectionStart();
  void UpdateSelectionEnd();
  void UpdateSelectionRects();
  void UpdateSelectionRects(SelectionState state);
  void UpdateSelectionViews() const;
  void SwapSelectionStartAndEnd();
  void RecalculateSelectionPosition();
  void CreateSelectionHandles();
  bool GetScrollXRegionAtPosition(PointF position, uint32_t* region_index,
                                  float* scroll_offset) const;
  bool ScrollXRegionTo(uint32_t region_index, float scroll_offset);
  bool OnStartHandleMove(PointF position, PointF motion, GestureEventType type);
  bool OnEndHandleMove(PointF position, PointF motion, GestureEventType type);
  bool OnHandleMove(PointF position, PointF motion, GestureEventType type);
  void SendSelectionChanged(SelectionState state) const;

  static MarkdownSelectionHandle* GetSelectionHandle(
      const std::shared_ptr<MarkdownPlatformView>& view);
  static MarkdownSelectionHighlight* GetSelectionHighlight(
      const std::shared_ptr<MarkdownPlatformView>& view);

  MarkdownViewContainerHandle* handle_{nullptr};
  MarkdownContext* context_{nullptr};
  MarkdownViewRenderer* renderer_{nullptr};
  std::shared_ptr<MarkdownDocument> document_{nullptr};
  MarkdownEventListener* event_listener_{nullptr};

  bool enable_selection_{false};
  struct SelectionHandles {
    std::shared_ptr<MarkdownPlatformView> left_;
    std::shared_ptr<MarkdownPlatformView> right_;
  };
  SelectionHandles selection_handles_{nullptr, nullptr};
  std::shared_ptr<MarkdownPlatformView> selection_highlight_{nullptr};
  float selection_handle_size_{0};
  float selection_handle_touch_margin_{0};
  uint32_t selection_handle_color_{0xff197de0};
  uint32_t selection_highlight_color_{0x14197de0};

  bool is_in_selection_{false};
  PointF select_start_position_{};
  PointF select_end_position_{};
  int32_t select_start_index_{0};
  int32_t select_end_index_{0};
  bool is_adjust_start_pos_{false};
  bool is_adjust_end_pos_{false};
  std::vector<RectF> selection_highlight_rects_;
  PointF handle_pan_before_motion_{0, 0};
  bool long_press_selection_drag_pending_{false};
  PointF long_press_selection_drag_origin_{};

  PanTarget pan_target_{PanTarget::kNone};
  uint32_t scroll_x_region_index_{0};
  float scroll_x_origin_offset_{0};
  bool scroll_x_scrolling_{false};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_GESTURE_H_
