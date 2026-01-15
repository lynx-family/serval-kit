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
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_selection_view.h"
namespace lynx::markdown {
enum class MarkdownAnimationType {
  kNone,
  kTypewriter,
};
enum class SourceType { kPlainText, kMarkdown };
class MarkdownView final : public MarkdownDrawable {
 public:
  explicit MarkdownView(MarkdownPlatformView* view);
  ~MarkdownView() override;
  void SetResourceLoader(MarkdownResourceLoader* loader);
  void SetEventListener(MarkdownEventListener* listener);
  void SetExposureListener(MarkdownExposureListener* listener);
  void SetContent(std::string_view content);

  void SetStyle(const ValueMap& style_map);
  void ApplyStyleInRange(const ValueMap& style_map, int32_t char_start,
                         int32_t char_end);
  void SetTextMaxLines(int32_t max_lines);
  void SetAnimationStep(int32_t animation_step);
  void SetAnimationType(MarkdownAnimationType type);
  void SetAnimationVelocity(float velocity);
  void SetTypewriterDynamicHeight(bool enable);
  void SetFrameRate(int32_t frame_rate) const;
  void SetParserType(std::string_view parser_type, void* parser_ud = nullptr);
  void SetSourceType(SourceType type);

  void SetEnableSelection(bool enable_selection);
  void SetSelectionHandleSize(float size);
  void SetSelectionHandleTouchMargin(float margin);
  void SetSelectionHandleColor(uint32_t color);
  void SetSelectionHighlightColor(uint32_t color);

  void SetTextSelection(Range char_range);

  void SetTrimParagraphSpaces(bool trim_spaces);

  void SetPaddings(float left, float top, float right, float bottom);
  void SetPadding(float padding);

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
  void Align();

  float GetMeasureWidth() const { return measured_width_; }
  float GetMeasureHeight() const { return measured_height_; }

  void OnNextFrame(int64_t timestamp);

  void NeedsParse();
  void NeedsMeasure();
  void NeedsDraw() const;

  void OnLongPress(PointF position, GestureEventType event);
  void OnTap(PointF position, GestureEventType event);

 protected:
  int32_t GetCharCount();
  void UpdateAnimationStep(int64_t timestamp);
  void UpdateExposure();

  void ClearForParse();
  void RemoveInlineViews();
  void HideAllSubviews();
  std::set<MarkdownPlatformView*> GetInlineViews();
  void RemoveUnusedViews(const std::set<MarkdownPlatformView*>& before,
                         const std::set<MarkdownPlatformView*>& after);

 protected:
  void SendParseEnd();
  void SendDrawStart();
  void SendDrawEnd();
  void SendAnimationStep(int32_t animation_step, int32_t max_animation_step);
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
  MarkdownMainViewHandle* handle_{nullptr};

  MarkdownDocument document_;
  MarkdownExposureListener* exposure_listener_{nullptr};

  std::string parser_type_{};
  SourceType source_type_{SourceType::kMarkdown};
  void* parser_ud_{nullptr};

  Paddings paddings_;

  float measured_width_{0};
  float measured_height_{0};

  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  float animation_velocity_{1};

  int32_t current_animation_step_{0};
  int32_t max_animation_step_{0};
  int64_t current_animation_step_time_{0};
  MarkdownPlatformView* custom_typewriter_cursor_{nullptr};
  PointF custom_cursor_position_{0, 0};

  bool typewriter_dynamic_height_{true};

  bool needs_parse_{true};
  bool needs_measure_{true};

  bool draw_start_sent_{false};
  bool draw_end_sent_{false};

  bool enable_selection_{false};

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
};

}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
