// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <string>
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_event_listener.h"
#include "markdown/platform/harmony/internal/harmony_exposure_listener.h"
#include "markdown/platform/harmony/internal/harmony_resource_loader_impl.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/utils/markdown_value.h"
#include "napi/native_api.h"

using lynx::markdown::Value;
using MarkdownValue = std::unique_ptr<Value>;
using lynx::markdown::HarmonyEventListener;
using lynx::markdown::HarmonyExposureListener;
using lynx::markdown::HarmonyResourceLoaderImpl;
using lynx::markdown::HarmonyValueRef;
using lynx::markdown::HarmonyValues;
using lynx::markdown::NativeServalMarkdownView;
using lynx::markdown::ValueType;
struct NativeMarkdownViewHolder {
  explicit NativeMarkdownViewHolder(napi_env env) {
    view_ = std::make_unique<NativeServalMarkdownView>();
    view_->SetClipByParent(false);
    resource_loader_ = std::make_unique<HarmonyResourceLoaderImpl>(env);
    event_listener_ = std::make_unique<HarmonyEventListener>(env);
    exposure_listener_ = std::make_unique<HarmonyExposureListener>(env);
    view_->SetResourceLoader(resource_loader_.get());
    view_->GetMarkdownView()->SetEventListener(event_listener_.get());
  }
  void Destroy() {
    view_ = nullptr;
    resource_loader_ = nullptr;
    event_listener_ = nullptr;
    exposure_listener_ = nullptr;
  }
  void ListenExposure() const {
    view_->GetMarkdownView()->SetExposureListener(exposure_listener_.get());
  }
  std::unique_ptr<NativeServalMarkdownView> view_;
  std::unique_ptr<HarmonyResourceLoaderImpl> resource_loader_;
  std::unique_ptr<HarmonyEventListener> event_listener_;
  std::unique_ptr<HarmonyExposureListener> exposure_listener_;
};
static napi_value NAPI_Global_createNativeMarkdownNode(
    napi_env env, napi_callback_info info) {
  const auto& [content_value] = HarmonyValues::GetValueFromParams<1>(env, info);
  auto content_handle =
      HarmonyValues::ConvertValue<ArkUI_NodeContentHandle>(env, content_value);
  auto* holder = HarmonyValues::WrapObject<NativeMarkdownViewHolder>(
      env, content_value, nullptr, env);
  holder->view_->AttachToNodeContent(content_handle);
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownContent(napi_env env,
                                                 napi_callback_info info) {
  const auto& [node_value, content] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto content_string = HarmonyValues::ConvertValue<std::string>(env, content);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, node_value);
      holder != nullptr) {
    holder->view_->SetContent(content_string);
  }
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownStyle(napi_env env,
                                               napi_callback_info info) {
  const auto& [content_value, style_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto style = HarmonyValues::ConvertValue<MarkdownValue>(env, style_value);
  if (style->GetType() != ValueType::kMap) {
    return nullptr;
  }
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->view_->SetStyle(style->AsMap());
  }
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownConfig(napi_env env,
                                                napi_callback_info info) {
  const auto& [content_value, config_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto config = HarmonyValues::ConvertValue<MarkdownValue>(env, config_value);
  if (config->GetType() != ValueType::kMap) {
    return nullptr;
  }
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->view_->SetConfig(config->AsMap());
  }
  return nullptr;
}

static napi_value NAPI_Global_registerImageLoader(napi_env env,
                                                  napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->resource_loader_->SetImageLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerFontLoader(napi_env env,
                                                 napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->resource_loader_->SetFontLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerInlineViewLoader(
    napi_env env, napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->resource_loader_->SetInlineViewLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerReplacementViewLoader(
    napi_env env, napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    holder->resource_loader_->SetReplacementViewLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_bindEvent(napi_env env, napi_callback_info info) {
  const auto& [content_value, name, function] =
      HarmonyValues::GetValueFromParams<3>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    auto str_name = HarmonyValues::ConvertValue<std::string>(env, name);
    holder->event_listener_->BindEvent(str_name,
                                       HarmonyValueRef(env, function));
  }
  return nullptr;
}
static napi_value NAPI_Global_bindExposure(napi_env env,
                                           napi_callback_info info) {
  const auto& [content_value, name, function] =
      HarmonyValues::GetValueFromParams<3>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    auto str_name = HarmonyValues::ConvertValue<std::string>(env, name);
    holder->exposure_listener_->BindExposure(str_name,
                                             HarmonyValueRef(env, function));
    holder->ListenExposure();
  }
  return nullptr;
}

static napi_value NAPI_Global_applyStyleInRange(napi_env env,
                                                napi_callback_info info) {
  const auto& [content_value, style, start, end] =
      HarmonyValues::GetValueFromParams<4>(env, info);
  if (auto* holder = HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(
          env, content_value);
      holder != nullptr) {
    auto style_var = HarmonyValues::ConvertValue<MarkdownValue>(env, style);
    if (style_var->GetType() != ValueType::kMap) {
      return nullptr;
    }
    auto start_var = HarmonyValues::ConvertValue<int32_t>(env, start);
    auto end_var = HarmonyValues::ConvertValue<int32_t>(env, end);
    holder->view_->GetMarkdownView()->ApplyStyleInRange(style_var->AsMap(),
                                                        start_var, end_var);
  }
  return nullptr;
}
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"createNativeMarkdownNode", nullptr,
       NAPI_Global_createNativeMarkdownNode, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setMarkdownContent", nullptr, NAPI_Global_setMarkdownContent, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"setMarkdownStyle", nullptr, NAPI_Global_setMarkdownStyle, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"setMarkdownConfig", nullptr, NAPI_Global_setMarkdownConfig, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"registerImageLoader", nullptr, NAPI_Global_registerImageLoader, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"registerFontLoader", nullptr, NAPI_Global_registerFontLoader, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"registerInlineViewLoader", nullptr,
       NAPI_Global_registerInlineViewLoader, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"registerReplacementViewLoader", nullptr,
       NAPI_Global_registerReplacementViewLoader, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"bindEvent", nullptr, NAPI_Global_bindEvent, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"bindExposure", nullptr, NAPI_Global_bindExposure, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"applyStyleInRange", nullptr, NAPI_Global_applyStyleInRange, nullptr,
       nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  NativeServalMarkdownView::InitEnv(env);
  return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "serval_markdown",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterServal_markdownModule(
    void) {
  napi_module_register(&demoModule);
}
