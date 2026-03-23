// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "markdown/draw/markdown_drawer.h"
#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/element/markdown_document.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_view_measurer.h"

namespace serval::markdown {
namespace {
constexpr float kViewVisibilityTolerant = 5.f;

class MarkdownRegionDrawable final : public MarkdownDrawable {
 public:
  MarkdownRegionDrawable(std::shared_ptr<MarkdownDocument> document,
                         uint32_t region_index,
                         const MarkdownAnimationType* animation_type,
                         const int32_t* animation_step,
                         const std::shared_ptr<MarkdownDrawable>* cursor)
      : document_(std::move(document)),
        region_index_(region_index),
        animation_type_(animation_type),
        animation_step_(animation_step),
        cursor_(cursor) {}
  ~MarkdownRegionDrawable() override = default;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    if (document_ == nullptr || canvas == nullptr) {
      return;
    }
    auto page = document_->GetPage();
    if (page == nullptr) {
      return;
    }
    const auto region_rect = page->GetRegionRect(region_index_);
    canvas->Save();
    canvas->Translate(-region_rect.GetLeft(), -region_rect.GetTop());
    if (animation_type_ != nullptr &&
        *animation_type_ == MarkdownAnimationType::kTypewriter) {
      MarkdownCharTypewriterDrawer drawer(
          canvas, animation_step_ == nullptr ? 0 : *animation_step_,
          document_->GetResourceLoader(),
          document_->GetStyle().typewriter_cursor_, false,
          cursor_ == nullptr ? nullptr : cursor_->get());
      drawer.DrawRegion(*page, region_index_);
    } else {
      MarkdownDrawer drawer(canvas);
      drawer.DrawRegion(*page, region_index_);
    }
    canvas->Restore();
  }

 private:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    if (document_ == nullptr) {
      return {};
    }
    auto page = document_->GetPage();
    if (page == nullptr) {
      return {};
    }
    auto rect = page->GetRegionRect(region_index_);
    const float width = rect.GetWidth();
    const float height = rect.GetHeight();
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }

 private:
  std::shared_ptr<MarkdownDocument> document_;
  uint32_t region_index_{0};
  const MarkdownAnimationType* animation_type_{nullptr};
  const int32_t* animation_step_{nullptr};
  const std::shared_ptr<MarkdownDrawable>* cursor_{nullptr};
};

class MarkdownBorderDrawable final : public MarkdownDrawable {
 public:
  MarkdownBorderDrawable(std::shared_ptr<MarkdownDocument> document,
                         uint32_t border_index)
      : document_(std::move(document)), border_index_(border_index) {}
  ~MarkdownBorderDrawable() override = default;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    if (document_ == nullptr || canvas == nullptr) {
      return;
    }
    auto page = document_->GetPage();
    if (page == nullptr) {
      return;
    }
    MarkdownDrawer drawer(canvas);
    drawer.DrawQuoteBorder(*page, border_index_);
  }

 private:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    if (document_ == nullptr) {
      return {};
    }
    auto page = document_->GetPage();
    if (page == nullptr) {
      return {};
    }
    auto* border = page->GetExtraBorder(border_index_);
    if (border == nullptr) {
      return {};
    }
    auto rect = border->rect_;
    const float width = rect.GetWidth();
    const float height = rect.GetHeight();
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }

 private:
  std::shared_ptr<MarkdownDocument> document_;
  uint32_t border_index_{0};
};
}  // namespace

void MarkdownViewRenderer::SetDocument(
    std::shared_ptr<MarkdownDocument> document) {
  if (document_ != document) {
    RemoveAllRegionViews();
  }
  document_ = std::move(document);
  region_views_dirty_ = true;
  full_redraw_required_ = true;
  has_last_view_rect_ = false;
}

void MarkdownViewRenderer::SetViewContainerHandle(
    MarkdownViewContainerHandle* handle) {
  if (handle_ == handle) {
    return;
  }
  RemoveAllRegionViews();
  handle_ = handle;
  region_views_dirty_ = true;
  has_last_view_rect_ = false;
}

void MarkdownViewRenderer::SetMarkdownAnimationType(
    MarkdownAnimationType type) {
  if (animation_type_ == type) {
    return;
  }
  animation_type_ = type;
  region_views_dirty_ = true;
  full_redraw_required_ = true;
}

void MarkdownViewRenderer::SetMarkdownAnimationStep(int32_t step) {
  const int32_t previous_step = animation_step_;
  animation_step_ = step;
  UpdateRegionViewsByAnimationStep(previous_step);
}

void MarkdownViewRenderer::UpdateSubViewRect(MarkdownPlatformView* view,
                                             const RectF& rect) {
  if (view == nullptr) {
    return;
  }
  view->SetMeasuredSize({rect.GetWidth(), rect.GetHeight()});
  view->Align(rect.GetLeft(), rect.GetTop());
}

bool MarkdownViewRenderer::NeedUseRegionView() const {
  return handle_ != nullptr;
}

bool MarkdownViewRenderer::NeedUpdateVisibleRegionViews(
    const RectF& view_rect) const {
  if (region_views_dirty_ || !has_last_view_rect_) {
    return true;
  }
  const float top_diff = view_rect.GetTop() - last_view_rect_.GetTop();
  const float bottom_diff = view_rect.GetBottom() - last_view_rect_.GetBottom();
  return std::abs(top_diff) >= kViewVisibilityTolerant ||
         std::abs(bottom_diff) >= kViewVisibilityTolerant;
}

void MarkdownViewRenderer::RemoveAllRegionViews() {
  if (handle_ != nullptr) {
    for (const auto& pair : region_views_) {
      handle_->RemoveSubView(pair.second.get());
    }
    for (const auto& pair : border_views_) {
      handle_->RemoveSubView(pair.second.get());
    }
  }
  region_views_.clear();
  border_views_.clear();
}

void MarkdownViewRenderer::UpdateVisibleRegionViews(RectF view_rect) {
  if (document_ == nullptr || handle_ == nullptr) {
    return;
  }
  auto page = document_->GetPage();
  if (page == nullptr) {
    RemoveAllRegionViews();
    return;
  }
  const float visible_top = view_rect.GetTop() - kViewVisibilityTolerant;
  const float visible_bottom = view_rect.GetBottom() + kViewVisibilityTolerant;

  const int32_t region_count = static_cast<int32_t>(page->GetRegionCount());
  const auto visible_regions =
      document_->GetShowedRegions(visible_top, visible_bottom);
  int32_t visible_region_start =
      std::max(0, std::min(visible_regions.start_, region_count - 1));
  int32_t visible_region_end =
      std::max(0, std::min(visible_regions.end_, region_count - 1));
  if (region_count <= 0 || visible_region_start > visible_region_end) {
    visible_region_start = 1;
    visible_region_end = 0;
  } else {
    for (int32_t i = visible_region_start; i <= visible_region_end; ++i) {
      auto iter = region_views_.find(i);
      bool created = false;
      if (iter == region_views_.end()) {
        const auto view = handle_->CreateRegionSubView();
        if (view != nullptr && view->GetCustomViewHandle() != nullptr) {
          auto drawable = std::make_unique<MarkdownRegionDrawable>(
              document_, static_cast<uint32_t>(i), &animation_type_,
              &animation_step_, &cursor_);
          view->GetCustomViewHandle()->AttachDrawable(std::move(drawable));
          iter = region_views_.emplace(i, view).first;
          created = true;
        }
      }
      if (iter != region_views_.end()) {
        UpdateSubViewRect(iter->second.get(),
                          page->GetRegionRect(static_cast<uint32_t>(i)));
        if (created) {
          iter->second->RequestDraw();
        }
      }
    }
  }

  for (auto iter = region_views_.begin(); iter != region_views_.end();) {
    const bool keep = iter->first >= visible_region_start &&
                      iter->first <= visible_region_end &&
                      iter->first < region_count;
    if (!keep) {
      handle_->RemoveSubView(iter->second.get());
      iter = region_views_.erase(iter);
    } else {
      ++iter;
    }
  }

  const int32_t border_count =
      static_cast<int32_t>(page->GetExtraBorderCount());
  const auto visible_borders =
      document_->GetShowedExtraContents(visible_top, visible_bottom);
  int32_t visible_border_start =
      std::max(0, std::min(visible_borders.start_, border_count - 1));
  int32_t visible_border_end =
      std::max(0, std::min(visible_borders.end_, border_count - 1));
  if (border_count <= 0 || visible_border_start > visible_border_end) {
    visible_border_start = 1;
    visible_border_end = 0;
  } else {
    for (int32_t i = visible_border_start; i <= visible_border_end; ++i) {
      auto iter = border_views_.find(i);
      bool created = false;
      if (iter == border_views_.end()) {
        const auto view = handle_->CreateRegionSubView();
        if (view != nullptr && view->GetCustomViewHandle() != nullptr) {
          auto drawable =
              std::make_unique<MarkdownBorderDrawable>(document_, i);
          view->GetCustomViewHandle()->AttachDrawable(std::move(drawable));
          iter = border_views_.emplace(i, view).first;
          created = true;
        }
      }
      if (iter != border_views_.end()) {
        auto* border = page->GetExtraBorder(i);
        if (border != nullptr) {
          UpdateSubViewRect(iter->second.get(), border->rect_);
          if (created) {
            iter->second->RequestDraw();
          }
        }
      }
    }
  }

  for (auto iter = border_views_.begin(); iter != border_views_.end();) {
    const bool keep = iter->first >= visible_border_start &&
                      iter->first <= visible_border_end &&
                      iter->first < border_count;
    if (!keep) {
      handle_->RemoveSubView(iter->second.get());
      iter = border_views_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void MarkdownViewRenderer::UpdateRegionViewsByViewRect() {
  if (!NeedUseRegionView()) {
    return;
  }
  if (document_ == nullptr || document_->GetPage() == nullptr) {
    RemoveAllRegionViews();
    return;
  }
  const auto view_rect = handle_->GetViewRectInScreen();
  if (NeedUpdateVisibleRegionViews(view_rect)) {
    UpdateVisibleRegionViews(view_rect);
    last_view_rect_ = view_rect;
    has_last_view_rect_ = true;
    region_views_dirty_ = false;
  }
  if (full_redraw_required_) {
    for (auto& pair : region_views_) {
      pair.second->RequestDraw();
    }
    for (auto& pair : border_views_) {
      pair.second->RequestDraw();
    }
    full_redraw_required_ = false;
  }
}

void MarkdownViewRenderer::UpdateRegionViewsByAnimationStep(
    int32_t previous_step) {
  if (animation_type_ == MarkdownAnimationType::kNone) {
    return;
  }
  if (animation_type_ != MarkdownAnimationType::kTypewriter ||
      previous_step == animation_step_) {
    return;
  }
  const auto range = document_->GetChangedRegionsWhenAnimationUpdated(
      previous_step, animation_step_);
  for (auto& [index, view] : region_views_) {
    if (index >= range.start_ && index <= range.end_) {
      view->RequestDraw();
    }
  }
}

void MarkdownViewRenderer::OnNextFrame() {
  UpdateRegionViewsByViewRect();
}

void MarkdownViewRenderer::Draw(tttext::ICanvasHelper* canvas, float left,
                                float top) const {
  if (document_ == nullptr) {
    return;
  }
  if (NeedUseRegionView()) {
    return;
  }
  if (animation_type_ == MarkdownAnimationType::kNone) {
    MarkdownDrawer drawer(canvas);
    auto page = document_->GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
  }
  if (animation_type_ == MarkdownAnimationType::kTypewriter) {
    MarkdownCharTypewriterDrawer drawer(
        canvas, animation_step_, document_->GetResourceLoader(),
        document_->GetStyle().typewriter_cursor_, false, cursor_.get());
    auto page = document_->GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
  }
}

}  // namespace serval::markdown
