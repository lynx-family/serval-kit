// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_resource_loader_impl.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"

namespace lynx::markdown {
HarmonyResourceLoaderImpl::HarmonyResourceLoaderImpl(napi_env env)
    : env_(env) {}

OH_Drawing_Font* HarmonyResourceLoaderImpl::LoadFont(const char* family) {
  if (font_loader_.IsNull()) {
    return nullptr;
  }
  return nullptr;
}
ArkUI_NodeHandle HarmonyResourceLoaderImpl::LoadInlineView(const char* id,
                                                           float max_width,
                                                           float max_height) {
  if (inline_view_loader_.IsNull()) {
    return nullptr;
  }
  auto value = HarmonyValues::CallFunction(env_, nullptr,
                                           inline_view_loader_.GetValue(), id);
  if (value == nullptr) {
    return nullptr;
  }
  return HarmonyValues::ConvertValue<ArkUI_NodeHandle>(env_, value);
}
ArkUI_NodeHandle HarmonyResourceLoaderImpl::LoadImageView(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (image_loader_.IsNull()) {
    return nullptr;
  }
  return nullptr;
}
ArkUI_NodeHandle HarmonyResourceLoaderImpl::LoadReplacementView(
    void* ud, float max_width, float max_height) {
  if (replacement_view_loader_.IsNull()) {
    return nullptr;
  }
  auto value = HarmonyValues::CallFunction(
      env_, nullptr, replacement_view_loader_.GetValue(), ud);
  if (value == nullptr) {
    return nullptr;
  }
  return HarmonyValues::ConvertValue<ArkUI_NodeHandle>(env_, value);
}

void HarmonyResourceLoaderImpl::SetFontLoader(napi_value loader) {
  font_loader_ = HarmonyValueRef(env_, loader);
}
void HarmonyResourceLoaderImpl::SetImageLoader(napi_value loader) {
  image_loader_ = HarmonyValueRef(env_, loader);
}
void HarmonyResourceLoaderImpl::SetInlineViewLoader(napi_value loader) {
  inline_view_loader_ = HarmonyValueRef(env_, loader);
}
void HarmonyResourceLoaderImpl::SetReplacementViewLoader(napi_value loader) {
  replacement_view_loader_ = HarmonyValueRef(env_, loader);
}
}  // namespace lynx::markdown
