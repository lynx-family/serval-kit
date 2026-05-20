// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/serval_markdown_view.h"

#include <arkui/native_gesture.h>
#include <arkui/ui_input_event.h>
#include <window_manager/oh_display_manager.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "markdown/draw/markdown_drawer.h"
#include "markdown/element/markdown_context.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/parser/markdown_parser.h"
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include "markdown/platform/harmony/markdown_platform_harmony.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "textra/platform_helper.h"
namespace serval::markdown {
namespace {
std::mutex g_recognizer_to_view_mutex;
std::unordered_map<ArkUI_GestureRecognizer*, NativeServalMarkdownView*>
    g_recognizer_to_view;

void RegisterRecognizer(ArkUI_GestureRecognizer* recognizer,
                        NativeServalMarkdownView* view) {
  std::lock_guard<std::mutex> lock(g_recognizer_to_view_mutex);
  g_recognizer_to_view[recognizer] = view;
}

void UnregisterRecognizer(ArkUI_GestureRecognizer* recognizer) {
  std::lock_guard<std::mutex> lock(g_recognizer_to_view_mutex);
  g_recognizer_to_view.erase(recognizer);
}
}  // namespace

class MarkdownRegionView : public HarmonyCustomView {
 public:
  ~MarkdownRegionView() override = default;
  void SetMeasuredSize(SizeF size) override {
    MarkdownDrawable::Measure(MeasureSpec{});
    HarmonyView::SetMeasuredSize(size);
  }
  void SetAlignPosition(PointF position) override {
    serval::markdown::HarmonyView::Align(position.x_, position.y_);
    HarmonyView::SetAlignPosition(position);
  }
};

void NativeServalMarkdownView::InitEnv(napi_env env) {
  HarmonyEnv::SetEnv(env);
  HarmonyUIThread::Init(env);
  UpdateDisplayMetrics();
}
NativeServalMarkdownView::NativeServalMarkdownView() : loader_(nullptr) {
  AttachDrawable(std::make_shared<MarkdownView>(
      this,
      std::make_shared<MarkdownContext>(CreateHarmonyMarkdownPlatform())));
  GetMarkdownView()->SetResourceLoader(this);
  HarmonyVSyncManager::AddVSyncCallback(this);
  ArkUINativeAPI::GetGestureApi()->setGestureInterrupterToNode(
      GetHandle(), GestureInterruptDispatcher);
  SetupGestures();
}
NativeServalMarkdownView::~NativeServalMarkdownView() {
  DisposeGestures();
  HarmonyVSyncManager::RemoveVSyncCallback(this);
}
void NativeServalMarkdownView::UpdateDisplayMetrics() {
  float density;
  auto err = OH_NativeDisplayManager_GetDefaultDisplayScaledDensity(&density);
  if (err == DISPLAY_MANAGER_OK) {
    MarkdownScreenMetrics::SetDensity(density);
  }
  int32_t width, height;
  err = OH_NativeDisplayManager_GetDefaultDisplayWidth(&width);
  if (err == DISPLAY_MANAGER_OK) {
    MarkdownScreenMetrics::SetScreenWidth(width);
  }
  err = OH_NativeDisplayManager_GetDefaultDisplayHeight(&height);
  if (err == DISPLAY_MANAGER_OK) {
    MarkdownScreenMetrics::SetScreenHeight(height);
  }
}
void NativeServalMarkdownView::AttachToNodeContent(
    ArkUI_NodeContentHandle handle) {
  node_content_handle_ = handle;
  if (node_content_handle_ == nullptr) {
    return;
  }
  OH_ArkUI_NodeContent_AddNode(node_content_handle_, GetHandle());
}
void NativeServalMarkdownView::DetachFromNodeContent() {
  OH_ArkUI_NodeContent_RemoveNode(node_content_handle_, GetHandle());
  node_content_handle_ = nullptr;
}
RectF NativeServalMarkdownView::GetViewRectInScreen() {
  return cached_view_rect_in_screen_;
}

RectF NativeServalMarkdownView::CalculateViewRectInScreen() {
  ArkUI_IntOffset offset;
  OH_ArkUI_NodeUtils_GetLayoutPositionInScreen(handle_, &offset);
  offset.x = -offset.x;
  offset.y = -offset.y;
  auto size = GetMeasuredSize();
  float screen_w = static_cast<float>(MarkdownScreenMetrics::GetScreenWidth());
  float screen_h = static_cast<float>(MarkdownScreenMetrics::GetScreenHeight());
  float left = static_cast<float>(std::max(0, offset.x));
  float top = static_cast<float>(std::max(0, offset.y));
  float right = std::min(static_cast<float>(offset.x) + screen_w, size.width_);
  float bottom =
      std::min(static_cast<float>(offset.y) + screen_h, size.height_);
  return RectF::MakeLTRB(left, top, right, bottom);
}
void* NativeServalMarkdownView::LoadFont(const char* family,
                                         MarkdownFontWeight weight) {
  if (loader_ == nullptr)
    return nullptr;
  return loader_->LoadFont(family);
}
void NativeServalMarkdownView::RemoveSubView(MarkdownPlatformView* view) {
  auto find = handle_cache_.find(view);
  if (find != handle_cache_.end()) {
    view_cache_.erase(find->second);
    handle_cache_.erase(find);
  }
  RemoveChild(static_cast<HarmonyView*>(view));
}
std::shared_ptr<MarkdownPlatformView> NativeServalMarkdownView::InsertEtsView(
    ArkUI_NodeHandle handle) {
  if (handle == nullptr) {
    return nullptr;
  }
  auto find = view_cache_.find(handle);
  if (find != view_cache_.end()) {
    return find->second;
  }
  auto view = std::make_shared<EtsViewHolder>(handle);
  auto* view_ptr = view.get();
  AddChild(view);
  view_cache_.emplace(handle, view);
  handle_cache_.emplace(view_ptr, handle);
  return view;
}
std::shared_ptr<MarkdownDrawable> NativeServalMarkdownView::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  if (loader_ == nullptr)
    return nullptr;
  return InsertEtsView(
      loader_->LoadInlineView(id_selector, max_width, max_height));
}
std::shared_ptr<MarkdownDrawable> NativeServalMarkdownView::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (loader_ == nullptr)
    return nullptr;
  return InsertEtsView(loader_->LoadImageView(
      src, desire_width, desire_height, max_width, max_height, border_radius));
}

std::shared_ptr<MarkdownDrawable> NativeServalMarkdownView::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  if (loader_ == nullptr) {
    return nullptr;
  }
  return InsertEtsView(
      loader_->LoadReplacementView(ud, id, max_width, max_height));
}
void NativeServalMarkdownView::OnVSync(int64_t time_stamp) {
  cached_view_rect_in_screen_ = CalculateViewRectInScreen();
  GetMarkdownView()->OnLayoutFrame(time_stamp / 1000 / 1000);
  GetMarkdownView()->OnRendererFrame(time_stamp / 1000 / 1000);
}

std::string GetStringValue(const ValueMap& config, const std::string& key,
                           const std::string& default_value = "") {
  if (auto iter = config.find(key);
      iter != config.end() && iter->second->GetType() == ValueType::kString) {
    return iter->second->GetString();
  }
  return default_value;
}

double GetNumberValue(const ValueMap& config, const std::string& key,
                      double default_value = 0) {
  if (auto iter = config.find(key);
      iter != config.end() && (iter->second->GetType() == ValueType::kDouble)) {
    return iter->second->GetDouble();
  }
  return default_value;
}

bool GetBooleanValue(const ValueMap& config, const std::string& key,
                     bool default_value = false) {
  if (auto iter = config.find(key);
      iter != config.end() && (iter->second->GetType() == ValueType::kBool)) {
    return iter->second->GetBool();
  }
  return default_value;
}

void NativeServalMarkdownView::SetConfig(const ValueMap& config) {
  if (auto animation_type = GetStringValue(config, "animationType");
      !animation_type.empty()) {
    if (animation_type == "typewriter") {
      GetMarkdownView()->SetAnimationType(MarkdownAnimationType::kTypewriter);
    } else if (animation_type == "line-expand") {
      GetMarkdownView()->SetAnimationType(MarkdownAnimationType::kLineExpand);
    } else {
      GetMarkdownView()->SetAnimationType(MarkdownAnimationType::kNone);
    }
  }
  if (auto velocity = GetNumberValue(config, "animationVelocity", -1);
      velocity >= 0) {
    GetMarkdownView()->SetAnimationVelocity(static_cast<float>(velocity));
  }
  if (auto iter = config.find("typewriterDynamicHeight");
      iter != config.end() && iter->second->GetType() == ValueType::kBool) {
    GetMarkdownView()->SetTypewriterDynamicHeight(iter->second->AsBool());
  }
  GetMarkdownView()->SetEnableSelection(
      GetBooleanValue(config, "enableSelection"));
  if (auto type = GetStringValue(config, "sourceType"); !type.empty()) {
    if (type == "plainText") {
      GetMarkdownView()->SetSourceType(SourceType::kPlainText);
    }
  }
  if (auto parser = GetStringValue(config, "parser"); !parser.empty()) {
    GetMarkdownView()->SetParserType(parser, nullptr);
  }
  if (auto color = GetStringValue(config, "selectionHandleColor");
      !color.empty()) {
    GetMarkdownView()->SetSelectionHandleColor(
        MarkdownStyleReader::ReadColor(color));
  }
  if (auto color = GetStringValue(config, "selectionHighlightColor");
      !color.empty()) {
    GetMarkdownView()->SetSelectionHighlightColor(
        MarkdownStyleReader::ReadColor(color));
  }
  if (auto size = GetNumberValue(config, "selectionHandleSize"); size > 0) {
    GetMarkdownView()->SetSelectionHandleSize(static_cast<float>(size));
  }
  if (auto size = GetNumberValue(config, "selectionHandleTouchMargin");
      size > 0) {
    GetMarkdownView()->SetSelectionHandleTouchMargin(static_cast<float>(size));
  }
}
std::shared_ptr<MarkdownPlatformView>
NativeServalMarkdownView::CreateCustomSubView() {
  auto custom_view = std::make_shared<HarmonyCustomView>();
  AddChild(custom_view);
  return std::static_pointer_cast<MarkdownPlatformView>(custom_view);
}

std::shared_ptr<MarkdownPlatformView>
NativeServalMarkdownView::CreateRegionSubView() {
  auto custom_view = std::make_shared<MarkdownRegionView>();
  InsertChild(custom_view, 0);
  return std::static_pointer_cast<MarkdownPlatformView>(custom_view);
}

std::shared_ptr<MarkdownPlatformView>
NativeServalMarkdownView::CreateScrollXRegionView() {
  return CreateRegionSubView();
}

ArkUI_GestureInterruptResult
NativeServalMarkdownView::GestureInterruptDispatcher(
    ArkUI_GestureInterruptInfo* info) {
  auto* recognizer = OH_ArkUI_GestureInterruptInfo_GetRecognizer(info);
  auto* event = OH_ArkUI_GestureInterruptInfo_GetGestureEvent(info);
  if (recognizer == nullptr || event == nullptr) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  NativeServalMarkdownView* view = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_recognizer_to_view_mutex);
    auto iter = g_recognizer_to_view.find(recognizer);
    if (iter != g_recognizer_to_view.end()) {
      view = iter->second;
    }
  }
  if (view == nullptr) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  if (recognizer == view->tap_ || recognizer == view->long_press_ ||
      recognizer == view->long_press_pan_group_) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  if (recognizer != view->pan_) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
  if (input_event == nullptr) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  const auto x = OH_ArkUI_PointerEvent_GetXByIndex(input_event, 0);
  const auto y = OH_ArkUI_PointerEvent_GetYByIndex(input_event, 0);
  const auto ox = OH_ArkUI_PanGesture_GetOffsetX(event);
  const auto oy = OH_ArkUI_PanGesture_GetOffsetY(event);
  if (!view->ShouldBeginPan({x, y}, {ox, oy})) {
    view->pan_tracking_ = false;
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  view->pan_tracking_ = true;
  return GESTURE_INTERRUPT_RESULT_CONTINUE;
}

void NativeServalMarkdownView::SetupGestures() {
  auto api = ArkUINativeAPI::GetGestureApi();
  tap_ = api->createTapGestureWithDistanceThreshold(1, 1, 1);
  auto tap_callback = [](ArkUI_GestureEvent* event, void* ud) {
    auto* view = reinterpret_cast<NativeServalMarkdownView*>(ud);
    auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
    if (view == nullptr || input_event == nullptr) {
      return;
    }
    auto x = OH_ArkUI_PointerEvent_GetX(input_event);
    auto y = OH_ArkUI_PointerEvent_GetY(input_event);
    auto type = OH_ArkUI_GestureEvent_GetActionType(event);
    if (type & GESTURE_EVENT_ACTION_ACCEPT) {
      (void)view->OnTapGesture({x, y}, GestureEventType::kDown);
    }
    if (type & GESTURE_EVENT_ACTION_END) {
      (void)view->OnTapGesture({x, y}, GestureEventType::kUp);
    }
  };
  api->setGestureEventTarget(tap_,
                             GESTURE_EVENT_ACTION_ACCEPT |
                                 GESTURE_EVENT_ACTION_UPDATE |
                                 GESTURE_EVENT_ACTION_END,
                             this, tap_callback);
  RegisterRecognizer(tap_, this);
  api->addGestureToNode(GetHandle(), tap_, NORMAL, NORMAL_GESTURE_MASK);

  long_press_pan_group_ = api->createGroupGesture(PARALLEL_GROUP);
  RegisterRecognizer(long_press_pan_group_, this);

  long_press_ = api->createLongPressGesture(1, false, 500);
  auto long_press_callback = [](ArkUI_GestureEvent* event, void* ud) {
    auto* view = reinterpret_cast<NativeServalMarkdownView*>(ud);
    auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
    if (view == nullptr || input_event == nullptr) {
      return;
    }
    auto x = OH_ArkUI_PointerEvent_GetX(input_event);
    auto y = OH_ArkUI_PointerEvent_GetY(input_event);
    auto type = OH_ArkUI_GestureEvent_GetActionType(event);
    if (type & GESTURE_EVENT_ACTION_ACCEPT) {
      (void)view->OnLongPressGesture({x, y}, GestureEventType::kDown);
    }
    if (type & GESTURE_EVENT_ACTION_END) {
      (void)view->OnLongPressGesture({x, y}, GestureEventType::kUp);
    }
  };
  api->setGestureEventTarget(long_press_,
                             GESTURE_EVENT_ACTION_ACCEPT |
                                 GESTURE_EVENT_ACTION_UPDATE |
                                 GESTURE_EVENT_ACTION_END,
                             this, long_press_callback);
  RegisterRecognizer(long_press_, this);
  api->addChildGesture(long_press_pan_group_, long_press_);

  pan_ = api->createPanGesture(1, GESTURE_DIRECTION_ALL, 5);
  auto pan_callback = [](ArkUI_GestureEvent* event, void* ud) {
    auto* view = reinterpret_cast<NativeServalMarkdownView*>(ud);
    auto* input_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
    if (view == nullptr || input_event == nullptr) {
      return;
    }
    auto x = OH_ArkUI_PointerEvent_GetX(input_event);
    auto y = OH_ArkUI_PointerEvent_GetY(input_event);
    auto ox = OH_ArkUI_PanGesture_GetOffsetX(event);
    auto oy = OH_ArkUI_PanGesture_GetOffsetY(event);
    auto type = OH_ArkUI_GestureEvent_GetActionType(event);
    if (type & GESTURE_EVENT_ACTION_ACCEPT) {
      if (!view->pan_tracking_) {
        view->pan_tracking_ = view->ShouldBeginPan({x, y}, {ox, oy});
      }
      if (!view->pan_tracking_) {
        return;
      }
      (void)view->OnPanGesture({x, y}, {ox, oy}, GestureEventType::kDown);
    }
    if (view->pan_tracking_ && type & GESTURE_EVENT_ACTION_UPDATE) {
      (void)view->OnPanGesture({x, y}, {ox, oy}, GestureEventType::kMove);
    }
    if (view->pan_tracking_ && type & GESTURE_EVENT_ACTION_END) {
      (void)view->OnPanGesture({x, y}, {ox, oy}, GestureEventType::kUp);
      view->pan_tracking_ = false;
    }
  };
  api->setGestureEventTarget(pan_,
                             GESTURE_EVENT_ACTION_ACCEPT |
                                 GESTURE_EVENT_ACTION_UPDATE |
                                 GESTURE_EVENT_ACTION_END,
                             this, pan_callback);
  RegisterRecognizer(pan_, this);
  api->addChildGesture(long_press_pan_group_, pan_);
  api->addGestureToNode(GetHandle(), long_press_pan_group_, NORMAL,
                        NORMAL_GESTURE_MASK);
}

void NativeServalMarkdownView::DisposeGestures() {
  auto api = ArkUINativeAPI::GetGestureApi();
  if (tap_ != nullptr) {
    api->removeGestureFromNode(GetHandle(), tap_);
    UnregisterRecognizer(tap_);
  }
  if (long_press_pan_group_ != nullptr) {
    api->removeGestureFromNode(GetHandle(), long_press_pan_group_);
    UnregisterRecognizer(long_press_pan_group_);
  }
  if (long_press_ != nullptr) {
    UnregisterRecognizer(long_press_);
  }
  if (pan_ != nullptr) {
    UnregisterRecognizer(pan_);
  }
  if (long_press_pan_group_ != nullptr) {
    api->dispose(long_press_pan_group_);
  }
}

void NativeServalMarkdownView::OnLayout(int32_t offset_x, int32_t offset_y) {
  HarmonyCustomView::OnLayout(offset_x, offset_y);
  GetMarkdownView()->Align(static_cast<float>(offset_x),
                           static_cast<float>(offset_y));
}

bool NativeServalMarkdownView::OnTapGesture(PointF position,
                                            GestureEventType event) {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view == nullptr) {
    return false;
  }
  return markdown_view->OnTap(position, event);
}

bool NativeServalMarkdownView::OnLongPressGesture(PointF position,
                                                  GestureEventType event) {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view == nullptr) {
    return false;
  }
  return markdown_view->OnLongPress(position, event);
}

bool NativeServalMarkdownView::ShouldBeginPan(PointF position, PointF motion) {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view == nullptr) {
    return false;
  }
  return markdown_view->ShouldBeginPan(position, motion);
}

bool NativeServalMarkdownView::OnPanGesture(PointF position, PointF motion,
                                            GestureEventType event) {
  auto* markdown_view = GetMarkdownView();
  if (markdown_view == nullptr) {
    return false;
  }
  return markdown_view->OnPan(position, motion, event);
}
}  // namespace serval::markdown
