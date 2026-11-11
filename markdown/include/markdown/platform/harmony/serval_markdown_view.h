// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "arkui/native_gesture.h"
#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include "markdown/utils/markdown_marco.h"
#include "markdown/utils/markdown_value.h"
#include "markdown/view/markdown_view.h"
#include "markdown/view/markdown_view_gesture.h"
namespace serval::markdown {
class IHarmonyResourceLoader;
class NativeMarkdownMeasurer;
class L_EXPORT NativeServalMarkdownView
    : public HarmonyCustomView,
      public MarkdownViewContainerHandle,
      public HarmonyVSyncCallback,
      public std::enable_shared_from_this<NativeServalMarkdownView> {
 public:
  static void InitEnv(napi_env env);
  static std::shared_ptr<NativeServalMarkdownView> Create(
      bool create_measurer = true);
  ~NativeServalMarkdownView() override;
  bool SetMeasurer(NativeMarkdownMeasurer* measurer);
  void DetachMeasurer();
  // UI-thread API. Disable the built-in clock when the host drives frames.
  void DisableInternalVSync(bool disable);
  void OnRendererFrame(int64_t frame_time_nanos);
  // UI-thread API. When enabled, frames retain the host-supplied render region.
  // Disabling restores automatic region calculation on the next frame.
  void SetExternalRenderRegion(bool external);
  // Supply the visible rectangle in view-local physical pixels before rendering
  // a frame with external render-region control enabled.
  void SetRenderRegion(RectF region);
  // The host supplies content dimensions in physical pixels, from its layout
  // thread. ArkUI then consumes this snapshot without measuring the document.
  void SetExternalMeasure(bool external);
  void SetExternalMeasuredSize(MeasureResult size);
  void SetContent(const std::string& content) const;
  void SetStyle(const ValueMap& style) const;
  void SetConfig(const ValueMap& config);
  void SetResourceLoader(IHarmonyResourceLoader* loader);
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

  explicit NativeServalMarkdownView(bool create_measurer);
  MeasureResult MeasureDrawable(MeasureSpec spec) override;

  RectF CalculateViewRectInScreen();
  std::shared_ptr<MarkdownPlatformView> InsertEtsView(
      ArkUI_NodeHandle handle,
      std::shared_ptr<MarkdownPlatformView> existing_view = nullptr);

  static void UpdateDisplayMetrics();
  static ArkUI_GestureInterruptResult GestureInterruptDispatcher(
      ArkUI_GestureInterruptInfo* info);

  // Call from the factory once the view has a shared owner.
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
  void* gesture_token_{nullptr};
  ArkUI_GestureRecognizer* long_press_pan_group_{nullptr};
  ArkUI_GestureRecognizer* long_press_{nullptr};
  ArkUI_GestureRecognizer* tap_{nullptr};
  ArkUI_GestureRecognizer* pan_{nullptr};
  bool pan_tracking_{false};
  std::unique_ptr<NativeMarkdownMeasurer> owned_measurer_;
  NativeMarkdownMeasurer* measurer_{nullptr};
  bool internal_vsync_disabled_{false};
  bool external_render_region_{false};
  bool external_measure_{false};
  MeasureResult external_measured_size_{};
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_SERVAL_MARKDOWN_VIEW_H_
