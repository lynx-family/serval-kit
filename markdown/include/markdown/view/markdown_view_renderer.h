// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
#include <memory>
#include <unordered_map>
#include <vector>

#include "markdown/element/markdown_document.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown_view_measurer.h"
namespace serval::markdown {
class MarkdownPlatformView;
class MarkdownViewContainerHandle;

class MarkdownViewRenderer {
 public:
  MarkdownViewRenderer() = default;
  ~MarkdownViewRenderer() = default;

  void Draw(tttext::ICanvasHelper* canvas, float left, float top) const;
  void SetDocument(std::shared_ptr<MarkdownDocument> document);
  const std::shared_ptr<MarkdownDocument>& GetDocument() const;
  void SetViewContainerHandle(MarkdownViewContainerHandle* handle);
  void SetMarkdownAnimationType(MarkdownAnimationType type);
  void SetMarkdownAnimationStep(int32_t step);
  void SetContentComplete(bool complete);
  void OnNextFrame();
  void RequestDrawRegion(uint32_t region_index);

 private:
  struct RegionViewEntry {
    std::shared_ptr<MarkdownPlatformView> view_;
    bool scroll_x_{false};
  };
  static void UpdateSubViewRect(MarkdownPlatformView* view, const RectF& rect);
  std::shared_ptr<MarkdownPlatformView> CreateRegionView(bool scroll_x);
  void RecycleRegionView(RegionViewEntry& entry);
  bool NeedUseRegionView() const;
  bool NeedUpdateVisibleRegionViews(const RectF& view_rect) const;
  void RemoveAllRegionViews();
  void UpdateRegionViewsByViewRect();
  void UpdateRegionViewsByAnimationStep(int32_t previous_step);
  void UpdateTypewriterCursorBounds();
  void UpdateVisibleRegionViews(RectF view_rect);
  void ClearRegionViewPool();

  std::shared_ptr<MarkdownDocument> document_;
  MarkdownViewContainerHandle* handle_{nullptr};
  MarkdownAnimationType animation_type_{MarkdownAnimationType::kNone};
  int32_t animation_step_{0};
  std::unordered_map<int32_t, RegionViewEntry> region_views_;
  std::unordered_map<int32_t, std::shared_ptr<MarkdownPlatformView>>
      border_views_;
  std::vector<std::shared_ptr<MarkdownPlatformView>> region_view_pool_;
  std::vector<std::shared_ptr<MarkdownPlatformView>> scroll_x_region_view_pool_;
  bool region_views_dirty_{true};
  bool full_redraw_required_{true};
  bool has_last_view_rect_{false};
  bool content_complete_{true};
  RectF last_view_rect_{};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_RENDERER_H_
