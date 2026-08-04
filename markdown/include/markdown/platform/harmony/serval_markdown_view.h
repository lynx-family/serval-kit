// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "arkui/native_gesture.h"
#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include "markdown/utils/markdown_marco.h"
#include "markdown/view/markdown_view.h"
#include "markdown/view/markdown_view_gesture.h"
namespace serval::markdown {
class NativeMarkdownMeasurer;
class L_EXPORT NativeServalMarkdownView : public HarmonyCustomView,
                                          public MarkdownViewContainerHandle,
                                          public HarmonyVSyncCallback {
 public:
  static void InitEnv(napi_env env);

 public:
  NativeServalMarkdownView();
  ~NativeServalMarkdownView() override;
  bool SetMeasurer(NativeMarkdownMeasurer* measurer);
  MarkdownView* GetMarkdownView() const {
    return static_cast<MarkdownView*>(drawable_.get());
  }
  void AttachToNodeContent(ArkUI_NodeContentHandle handle);
  void DetachFromNodeContent();

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
  std::shared_ptr<MarkdownPlatformView> CreateScrollXRegionView() override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, uint32_t color) override;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) override;
  void OnLayout(int32_t offset_x, int32_t offset_y) override;
  // end
  // HarmonyVSyncCallback
  void OnVSync(int64_t time_stamp) override;
  // end

  MarkdownViewContainerHandle* GetViewContainerHandle() override {
    return this;
  }

  void RequestMeasure();

 protected:
  friend class NativeMarkdownMeasurer;

  RectF CalculateViewRectInScreen();
  std::shared_ptr<MarkdownPlatformView> InsertEtsView(
      ArkUI_NodeHandle handle,
      std::shared_ptr<MarkdownPlatformView> existing_view = nullptr);

  static void UpdateDisplayMetrics();
  static ArkUI_GestureInterruptResult GestureInterruptDispatcher(
      ArkUI_GestureInterruptInfo* info);

  void SetupGestures();
  void DisposeGestures();
  virtual bool OnTapGesture(PointF position, GestureEventType event);
  virtual bool OnLongPressGesture(PointF position, GestureEventType event);
  virtual bool ShouldBeginPan(PointF position, PointF motion);
  virtual bool OnPanGesture(PointF position, PointF motion,
                            GestureEventType event);

  ArkUI_NodeContentHandle node_content_handle_{nullptr};

  std::unordered_map<ArkUI_NodeHandle, std::shared_ptr<MarkdownPlatformView>>
      view_cache_;
  std::unordered_map<MarkdownPlatformView*, ArkUI_NodeHandle> handle_cache_;
  RectF cached_view_rect_in_screen_{};
  ArkUI_GestureRecognizer* long_press_pan_group_{nullptr};
  ArkUI_GestureRecognizer* long_press_{nullptr};
  ArkUI_GestureRecognizer* tap_{nullptr};
  ArkUI_GestureRecognizer* pan_{nullptr};
  bool pan_tracking_{false};
  NativeMarkdownMeasurer* measurer_{nullptr};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
