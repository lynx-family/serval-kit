// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#include <memory>
#include <unordered_map>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown_view_measurer.h"
namespace lynx::markdown {
class MarkdownPlatformView;
class MarkdownViewContainerHandle;

class MarkdownViewRenderer {
 public:
  MarkdownViewRenderer() = default;
  ~MarkdownViewRenderer() = default;

  void Draw(tttext::ICanvasHelper* canvas, float left, float top) const;
  void SetDocument(std::shared_ptr<MarkdownDocument> document);
  void SetViewContainerHandle(MarkdownViewContainerHandle* handle);
  void SetMarkdownAnimationType(MarkdownAnimationType type);
  void SetMarkdownAnimationStep(int32_t step);
  void SetTypewriterCursor(std::shared_ptr<MarkdownDrawable> cursor) {
    cursor_ = std::move(cursor);
  }
  void OnNextFrame();

 private:
  static void UpdateSubViewRect(MarkdownPlatformView* view, const RectF& rect);
  bool NeedUseRegionView() const;
  bool NeedUpdateVisibleRegionViews(const RectF& view_rect) const;
  void RemoveAllRegionViews();
  void UpdateRegionViewsByViewRect();
  void UpdateRegionViewsByAnimationStep(int32_t previous_step);
  void UpdateVisibleRegionViews(RectF view_rect);

  std::shared_ptr<MarkdownDocument> document_;
  MarkdownViewContainerHandle* handle_{nullptr};
  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  int32_t animation_step_{0};
  std::shared_ptr<MarkdownDrawable> cursor_{nullptr};
  std::unordered_map<int32_t, std::shared_ptr<MarkdownPlatformView>>
      region_views_;
  std::unordered_map<int32_t, std::shared_ptr<MarkdownPlatformView>>
      border_views_;
  bool region_views_dirty_{true};
  bool full_redraw_required_{true};
  bool has_last_view_rect_{false};
  RectF last_view_rect_{};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
