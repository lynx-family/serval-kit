// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_context.h"
#include "markdown/element/markdown_document.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/utils/markdown_marco.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_props.h"
#include "markdown/view/markdown_view_animator.h"
#include "markdown/view/markdown_view_gesture.h"
#include "markdown/view/markdown_view_measurer.h"
#include "markdown_view_renderer.h"
namespace serval::markdown {
class MarkdownEventListener;
class MarkdownExposureListener;

class L_EXPORT MarkdownView final : public MarkdownDrawable {
 public:
  MarkdownView(MarkdownPlatformView* view,
               std::shared_ptr<MarkdownContext> context);
  ~MarkdownView() override;
  void SetResourceLoader(MarkdownResourceLoader* loader);
  MarkdownResourceLoader* GetResourceLoader() const;
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
  void SetMarkdownEffect(std::unique_ptr<Value> effect);

  void SetAnimationStep(int32_t animation_step);
  int32_t GetAnimationStep() const { return animator_.GetAnimationStep(); }
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
  void SetArrayProp(MarkdownProps prop, ValueArray& array);
  void SetMapProp(MarkdownProps prop, ValueMap& map);

  std::string GetSelectedText() const;
  std::string GetContent() const;
  std::string GetContentID() const;
  Range GetSelectedRange() const;
  const std::vector<RectF>& GetSelectedLineBoundingRect();
  PointF GetSelectionHandlePosition() const;
  float GetSelectionHandleRadius() const;
  std::vector<std::string> GetAllImageUrl() const;
  std::vector<std::string> GetLinkUrl() const;
  std::vector<std::string> GetLinkContent() const;
  std::vector<RectF> GetLinkBoundingRect() const;
  std::vector<Range> GetSyntaxSourceRanges(std::string_view tag) const;

  int32_t GetCharIndexByPosition(PointF position) const;
  Range GetCharRangeByPosition(
      PointF position, MarkdownSelection::CharRangeType char_range_type) const;
  std::vector<RectF> GetTextLineBoundingRect(Range range) const;
  RectF GetTextBoundingRect(Range range) const;

  std::string GetParsedContent(Range char_range) const;
  int32_t CharOffsetToSourceOffset(int32_t char_offset) const;
  int32_t SourceOffsetToCharOffset(int32_t source_offset) const;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  void Align(float x, float y) override;

  float GetMeasureWidth() const { return measurer_.GetMeasuredSize().width_; }
  float GetMeasureHeight() const { return measurer_.GetMeasuredSize().height_; }

  void OnLayoutFrame(int64_t timestamp);
  void OnRendererFrame(int64_t timestamp);

  void MarkDirty();
  void NeedsMeasure();
  void NeedsAlign() const;
  void NeedsDraw() const;

  bool OnLongPress(PointF position, GestureEventType event);
  bool OnTap(PointF position, GestureEventType event);
  bool ShouldBeginPan(PointF position, PointF motion);
  bool OnPan(PointF position, PointF motion, GestureEventType event);

  void OnFontLoaded(std::string_view family, int weight, int style);
  void OnImageLoaded(std::string_view url);

 protected:
  void SetContentRangeStart(int32_t start);
  void SetContentRangeEnd(int32_t end);
  MeasureResult OnMeasure(MeasureSpec spec) override;
  int32_t GetCharCount() const;
  float CalculateHeightByAnimationStep();
  float CalculateHeightByAnimationStep(int32_t animation_step,
                                       bool update_cursor_position);
  void UpdateAnimationStep();
  void UpdateTransitionHeight() const;
  void UpdateDrawEventsByAnimation();
  void UpdateExposure();
  void UpdateTextAttachments();
  void PublishRendererBundle();
  void ConsumeRendererBundleIfNeeded();

  std::set<MarkdownPlatformView*> GetInlineViews() const;
  void RemoveUnusedViews(const std::set<MarkdownPlatformView*>& before,
                         const std::set<MarkdownPlatformView*>& after) const;
  std::vector<int32_t> GetLineExpandAnimationSteps() const;
  bool IsStepAnimatedHeightEnabled() const;
  bool IsAnimationComplete() const;

 protected:
  void SendDrawStart();
  void SendDrawEnd();

 protected:
  MarkdownPlatformView* view_{nullptr};
  MarkdownViewContainerHandle* handle_{nullptr};
  std::unique_ptr<Value> attachments_;
  std::unique_ptr<Value> effect_;

  struct LayoutData {
    std::shared_ptr<MarkdownDocument> document_{nullptr};
    PointF custom_cursor_position_{0, 0};
    bool content_complete_{true};
  };

  struct RendererBundle {
    std::shared_ptr<MarkdownDocument> document_{nullptr};
    MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
    int32_t animation_step_{0};
    bool content_complete_{true};
  };

  struct ExposureKey {
    std::string url_{};
    int32_t char_index_{0};
    std::string content_{};

    bool operator==(const ExposureKey& other) const {
      return url_ == other.url_ && char_index_ == other.char_index_ &&
             content_ == other.content_;
    }
    struct Hash {
      size_t operator()(const ExposureKey& key) const {
        auto hash = std::hash<std::string>{}(key.url_);
        hash ^= std::hash<std::string>{}(key.content_) << 1;
        hash ^= std::hash<int>{}(key.char_index_) << 2;
        return hash;
      }
    };
  };

  struct RendererData {
    std::shared_ptr<MarkdownDocument> document_{nullptr};
    std::unordered_set<ExposureKey, ExposureKey::Hash> exposure_links_;
    std::unordered_set<ExposureKey, ExposureKey::Hash> exposure_images_;
  };

  LayoutData layout_data_{};
  RendererData renderer_data_{};
  std::mutex renderer_bundle_mutex_{};
  std::unique_ptr<RendererBundle> renderer_bundle_{nullptr};

  std::shared_ptr<MarkdownContext> context_{nullptr};
  MarkdownViewMeasurer measurer_;
  MarkdownViewAnimator animator_;
  MarkdownViewRenderer renderer_;
  MarkdownViewGesture gesture_;
  MarkdownExposureListener* exposure_listener_{nullptr};
  MarkdownResourceLoader* resource_loader_{nullptr};
  MarkdownEventListener* event_listener_{nullptr};

  bool trim_paragraph_spaces_{false};

  bool typewriter_height_transition_prefetch_{true};
  bool draw_start_sent_{false};
  bool draw_end_sent_{false};
};

}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_H_
