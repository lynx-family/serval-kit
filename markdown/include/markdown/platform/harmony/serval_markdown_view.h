// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view.h"
namespace lynx::markdown {
class NativeServalMarkdownView final : public HarmonyCustomView,
                                       public MarkdownViewContainerHandle,
                                       public MarkdownResourceLoader,
                                       public HarmonyVSyncCallback {
 public:
  static void InitEnv(napi_env env);

 public:
  NativeServalMarkdownView();
  ~NativeServalMarkdownView() override;
  void SetContent(const std::string& content) const {
    GetMarkdownView()->SetContent(content);
  }
  void SetStyle(const ValueMap& style) const {
    GetMarkdownView()->SetStyle(style);
  }
  void SetConfig(const ValueMap& config);
  MarkdownView* GetMarkdownView() const {
    return static_cast<MarkdownView*>(drawable_.get());
  }
  void AttachToNodeContent(ArkUI_NodeContentHandle handle);
  void DetachFromNodeContent();
  void SetResourceLoader(IHarmonyResourceLoader* loader) { loader_ = loader; }

  // MarkdownMainViewHandle
  void RemoveSubView(MarkdownPlatformView* view) override;
  void RemoveAllSubViews() override {
    view_cache_.clear();
    handle_cache_.clear();
    RemoveAllChildren();
  }
  RectF GetViewRectInScreen() override;
  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin,
      uint32_t color) override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override;
  void OnLayout(int32_t offset_x, int32_t offset_y) override;
  // end
  void* LoadFont(const char* family, MarkdownFontWeight wieght) override;
  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override;
  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) override;
  std::shared_ptr<MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override;
  // end

  // HarmonyVSyncCallback
  void OnVSync(int64_t time_stamp) override;
  // end

  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return this;
  }

 protected:
  RectF CalculateViewRectInScreen();
  std::shared_ptr<MarkdownPlatformView> InsertEtsView(ArkUI_NodeHandle handle);

  static void UpdateDisplayMetrics();

  ArkUI_NodeContentHandle node_content_handle_{nullptr};
  IHarmonyResourceLoader* loader_;

  std::unordered_map<ArkUI_NodeHandle, std::shared_ptr<MarkdownPlatformView>>
      view_cache_;
  std::unordered_map<MarkdownPlatformView*, ArkUI_NodeHandle> handle_cache_;
  RectF cached_view_rect_in_screen_{};
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
