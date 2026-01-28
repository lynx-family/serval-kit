// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/serval_markdown_view.h"

#include <arkui/ui_input_event.h>
#include <window_manager/oh_display_manager.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "markdown/draw/markdown_drawer.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/layout/markdown_layout.h"
#include "markdown/markdown_resource_loader.h"
#include "markdown/parser/markdown_parser.h"
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include "markdown/style/markdown_style_reader.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "textra/platform_helper.h"
namespace lynx::markdown {
void NativeServalMarkdownView::InitEnv(napi_env env) {
  HarmonyEnv::SetEnv(env);
  HarmonyUIThread::Init(env);
  UpdateDisplayMetrics();
}
NativeServalMarkdownView::NativeServalMarkdownView() : loader_(nullptr) {
  AttachDrawable(std::make_unique<MarkdownView>(this));
  GetMarkdownView()->SetResourceLoader(this);
  HarmonyVSyncManager::AddVSyncCallback(this);
  EnableTapEvent(true, NORMAL);
  EnableLongPressEvent(true, NORMAL);
  EnablePanEvent(true, GESTURE_DIRECTION_HORIZONTAL, NORMAL);
  GetMarkdownView()->SetSelectionHandleSize(MarkdownScreenMetrics::DPToPx(10));
  GetMarkdownView()->SetSelectionHandleTouchMargin(
      MarkdownScreenMetrics::DPToPx(5));
}
NativeServalMarkdownView::~NativeServalMarkdownView() {
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
void* NativeServalMarkdownView::LoadFont(const char* family) {
  if (loader_ == nullptr) return nullptr;
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
MarkdownPlatformView* NativeServalMarkdownView::InsertEtsView(
    ArkUI_NodeHandle handle) {
  if (handle == nullptr) {
    return nullptr;
  }
  auto find = view_cache_.find(handle);
  if (find != view_cache_.end()) {
    return find->second;
  }
  auto view = std::make_unique<EtsViewHolder>(handle);
  auto view_ptr = view.get();
  AddChild(std::move(view));
  view_cache_.emplace(handle, view_ptr);
  handle_cache_.emplace(view_ptr, handle);
  return view_ptr;
}
MarkdownPlatformView* NativeServalMarkdownView::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  if (loader_ == nullptr) return nullptr;
  return InsertEtsView(
      loader_->LoadInlineView(id_selector, max_width, max_height));
}
MarkdownPlatformView* NativeServalMarkdownView::LoadImageView(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (loader_ == nullptr) return nullptr;
  return InsertEtsView(loader_->LoadImageView(
      src, desire_width, desire_height, max_width, max_height, border_radius));
}
std::shared_ptr<MarkdownDrawable>
NativeServalMarkdownView::LoadBackgroundDrawable(
    MarkdownBackgroundStylePart* background_style, float border_radius,
    float font_size, float root_font_size) {
  if (loader_ == nullptr) return nullptr;
  return nullptr;
}
MarkdownPlatformView* NativeServalMarkdownView::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  if (loader_ == nullptr) {
    return nullptr;
  }
  return InsertEtsView(
      loader_->LoadReplacementView(ud, id, max_width, max_height));
}
void NativeServalMarkdownView::OnVSync(int64_t time_stamp) {
  GetMarkdownView()->OnNextFrame(time_stamp);
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
MarkdownPlatformView* NativeServalMarkdownView::CreateCustomSubView() {
  auto custom_view = std::make_unique<HarmonyCustomView>();
  auto view = custom_view.get();
  AddChild(std::move(custom_view));
  return view;
}
void NativeServalMarkdownView::OnLayout(int32_t offset_x, int32_t offset_y) {
  HarmonyCustomView::OnLayout(offset_x, offset_y);
  GetMarkdownView()->Align();
}
}  // namespace lynx::markdown
