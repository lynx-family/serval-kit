// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_document.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_exposure_listener.h"
#include "markdown/markdown_platform_loader.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_props.h"
#include "markdown/view/markdown_selection_view.h"
#include "markdown/view/markdown_view_animator.h"
#include "markdown/view/markdown_view_measurer.h"
#include "markdown_view_renderer.h"
namespace lynx::markdown {
class MarkdownView final : public MarkdownDrawable,
                           public MarkdownResourceLoader {
 public:
  explicit MarkdownView(MarkdownPlatformView* view);
  ~MarkdownView() override;
  void SetPlatformLoader(MarkdownPlatformLoader* loader);
  MarkdownPlatformLoader* GetPlatformLoader() const;
  void SetEventListener(MarkdownEventListener* listener);
  void SetExposureListener(MarkdownExposureListener* listener);

  void SetContent(std::string_view content);
  void SetContentID(std::string_view id);
  void SetContentComplete(bool complete);
  void SetContentRange(Range range);
  void SetParserType(std::string_view parser_type, void* parser_ud = nullptr);
  void SetSourceType(SourceType type);

  void SetStyle(const ValueMap& style_map);
  void ApplyStyleInRange(const ValueMap& style_map, int32_t char_start,
                         int32_t char_end);
  void SetTextMaxLines(int32_t max_lines);
  void SetEnableBreakAroundPunctuation(bool allow);
  void SetTextAttachments(std::unique_ptr<Value> attachments);

  void SetAnimationStep(int32_t animation_step);
  void SetAnimationType(MarkdownAnimationType type);
  void SetAnimationVelocity(float velocity);
  void SetInitialAnimationStep(int32_t step);
  void SetTypewriterDynamicHeight(bool enable);
  void SetHeightTransitionDuration(float duration);
  void SetTypewriterHeightTransitionPrefetch(bool enable);

  void SetEnableSelection(bool enable_selection);
  void SetSelectionHandleSize(float size);
  void SetSelectionHandleTouchMargin(float margin);
  void SetSelectionHandleColor(uint32_t color);
  void SetSelectionHighlightColor(uint32_t color);

  void SetTextSelection(Range char_range);

  void SetTrimParagraphSpaces(bool trim_spaces);

  void SetPaddings(float left, float top, float right, float bottom);
  void SetPadding(float padding);

  void SetNumberProp(MarkdownProps prop, double value);
  void SetStringProp(MarkdownProps prop, std::string_view value);
  void SetArrayProp(MarkdownProps prop, const ValueArray& array);
  void SetMapProp(MarkdownProps prop, const ValueMap& map);

  std::string GetSelectedText();
  Range GetSelectedRange() const;
  const std::vector<RectF>& GetSelectedLineBoundingRect();

  int32_t GetCharIndexByPosition(PointF position);
  Range GetCharRangeByPosition(
      PointF position, MarkdownSelection::CharRangeType char_range_type);
  std::vector<RectF> GetTextLineBoundingRect(Range range);
  RectF GetTextBoundingRect(Range range);

  std::string GetParsedContent(Range char_range);

  SizeF Measure(MeasureSpec spec) override;
  void Draw(tttext::ICanvasHelper* canvas, float left, float top, float right,
            float bottom) override;
  float GetWidth() const override;
  float GetHeight() const override;
  void Align(float x, float y) override;

  float GetMeasureWidth() const { return measurer_.GetMeasuredSize().width_; }
  float GetMeasureHeight() const { return measurer_.GetMeasuredSize().height_; }

  void OnNextFrame(int64_t timestamp);

  void NeedsMeasure();
  void NeedsAlign() const;
  void NeedsDraw() const;

  void OnLongPress(PointF position, GestureEventType event);
  void OnTap(PointF position, GestureEventType event);
  void OnPan(PointF position, PointF motion, GestureEventType event);

  std::unique_ptr<tttext::RunDelegate> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override;
  void* LoadFont(const char* family, MarkdownFontWeight weight) override;
  std::unique_ptr<tttext::RunDelegate> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) override;
  std::unique_ptr<tttext::RunDelegate> LoadInlineView(
      const char* id_selector, float max_width, float max_height) override;
  std::unique_ptr<tttext::RunDelegate> LoadGradient(
      const char* gradient, float font_size, float root_font_size) override;

 protected:
  int32_t GetCharCount();
  float CalculateHeightByAnimationStep();
  void EnsureTypewriterCursor();
  float CalculateHeightByAnimationStep(int32_t animation_step,
                                       bool update_cursor_position);
  void UpdateAnimationStep();
  void UpdateTransitionHeight() const;
  void UpdateExposure();

  void ClearForParse();
  void RemoveInlineViews();
  void HideAllSubviews();
  std::set<MarkdownPlatformView*> GetInlineViews();
  void RemoveUnusedViews(const std::set<MarkdownPlatformView*>& before,
                         const std::set<MarkdownPlatformView*>& after) const;

 protected:
  void SendDrawStart();
  void SendDrawEnd();
  void SendLinkClicked(const char* url, const char* content);
  void SendImageClicked(const char* url);
  void SendSelectionChanged(SelectionState state) const;

 protected:
  void EnterSelection(PointF position);
  void ExitSelection();
  void UpdateSelectionStart();
  void UpdateSelectionEnd();
  void UpdateSelectionRects(SelectionState state);
  void UpdateSelectionViews() const;
  void SwapSelectionStartAndEnd();
  void RecalculateSelectionPosition();
  void CreateSelectionHandles();
  void OnStartHandleMove(PointF position, PointF motion, GestureEventType type);
  void OnEndHandleMove(PointF position, PointF motion, GestureEventType type);
  void OnHandleMove(PointF position, PointF motion, GestureEventType type);

 protected:
  static MarkdownSelectionHandle* GetSelectionHandle(
      MarkdownPlatformView* view) {
    return static_cast<MarkdownSelectionHandle*>(
        view->GetCustomViewHandle()->GetDrawable());
  }
  static MarkdownSelectionHighlight* GetSelectionHighlight(
      MarkdownPlatformView* view) {
    return static_cast<MarkdownSelectionHighlight*>(
        view->GetCustomViewHandle()->GetDrawable());
  }

 protected:
  MarkdownPlatformView* view_{nullptr};
  MarkdownViewContainerHandle* handle_{nullptr};
  bool enable_selection_{false};
  std::unique_ptr<Value> attachments_;

  std::shared_ptr<MarkdownDocument> document_;
  bool document_updated_{false};
  MarkdownViewMeasurer measurer_;
  MarkdownViewAnimator animator_;
  MarkdownViewRenderer renderer_;
  MarkdownExposureListener* exposure_listener_{nullptr};
  MarkdownPlatformLoader* platform_loader_{nullptr};
  MarkdownEventListener* event_listener_{nullptr};

  MarkdownPlatformView* custom_typewriter_cursor_{nullptr};
  std::string custom_typewriter_cursor_selector_{};
  PointF custom_cursor_position_{0, 0};
  bool draw_start_sent_{false};
  bool draw_end_sent_{false};

  struct SelectionHandles {
    MarkdownPlatformView* left_;
    MarkdownPlatformView* right_;
  };
  SelectionHandles selection_handles_{nullptr, nullptr};
  MarkdownPlatformView* selection_highlight_{nullptr};
  float selection_handle_size_{17};
  float selection_handle_touch_margin_{0};
  uint32_t selection_handle_color_{0xff0000ff};
  uint32_t selection_highlight_color_{0x400000ff};

  bool is_in_selection_{false};
  PointF select_start_position_{};
  PointF select_end_position_{};
  int32_t select_start_index_{0};
  int32_t select_end_index_{0};
  bool is_adjust_start_pos_{false};
  bool is_adjust_end_pos_{false};
  std::vector<RectF> selection_highlight_rects_;
  PointF start_handle_position_{};
  PointF end_handle_position_{};
  PointF handle_pan_before_motion_{0, 0};

  bool trim_paragraph_spaces_{false};

  std::unordered_set<MarkdownLink*> exposure_links_;
  std::unordered_set<MarkdownImage*> exposure_images_;

  bool typewriter_height_transition_prefetch_{false};
};

}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
