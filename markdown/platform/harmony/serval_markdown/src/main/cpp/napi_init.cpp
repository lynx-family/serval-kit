// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <string>
#include "markdown/platform/harmony/harmony_markdown_measurer.h"
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_event_listener.h"
#include "markdown/platform/harmony/internal/harmony_exposure_listener.h"
#include "markdown/platform/harmony/internal/harmony_resource_loader_impl.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/utils/markdown_value.h"
#include "napi/native_api.h"

using serval::markdown::Value;
using MarkdownValue = std::unique_ptr<Value>;
using serval::markdown::HarmonyEventListener;
using serval::markdown::HarmonyExposureListener;
using serval::markdown::HarmonyResourceLoaderImpl;
using serval::markdown::HarmonyValueRef;
using serval::markdown::HarmonyValues;
using serval::markdown::NativeMarkdownMeasurer;
using serval::markdown::NativeServalMarkdownView;
using serval::markdown::ValueType;
struct NativeMarkdownMeasurerHolder {
  explicit NativeMarkdownMeasurerHolder(napi_env env)
      : resource_loader_(std::make_unique<HarmonyResourceLoaderImpl>(env)),
        event_listener_(std::make_unique<HarmonyEventListener>(env)),
        exposure_listener_(std::make_unique<HarmonyExposureListener>(env)),
        measurer_(std::make_unique<NativeMarkdownMeasurer>()) {
    measurer_->SetResourceLoader(resource_loader_.get());
    measurer_->GetMarkdownView()->SetEventListener(event_listener_.get());
  }
  void ListenExposure() const {
    measurer_->GetMarkdownView()->SetExposureListener(exposure_listener_.get());
  }
  std::unique_ptr<HarmonyResourceLoaderImpl> resource_loader_;
  std::unique_ptr<HarmonyEventListener> event_listener_;
  std::unique_ptr<HarmonyExposureListener> exposure_listener_;
  std::unique_ptr<NativeMarkdownMeasurer> measurer_;
};
struct NativeMarkdownViewHolder {
  NativeMarkdownViewHolder()
      : view_(std::make_unique<NativeServalMarkdownView>()) {
    view_->SetClipByParent(false);
  }
  std::unique_ptr<NativeServalMarkdownView> view_;
};

NativeMarkdownMeasurerHolder* UnwrapMeasurer(napi_env env, napi_value value) {
  return HarmonyValues::UnwrapObject<NativeMarkdownMeasurerHolder>(env, value);
}

static napi_value NAPI_Global_createNativeMarkdownMeasurer(
    napi_env env, napi_callback_info info) {
  const auto& [measurer_value] =
      HarmonyValues::GetValueFromParams<1>(env, info);
  HarmonyValues::WrapObject<NativeMarkdownMeasurerHolder>(env, measurer_value,
                                                          nullptr, env);
  return nullptr;
}

static napi_value NAPI_Global_setNativeMarkdownMeasurer(
    napi_env env, napi_callback_info info) {
  const auto& [node_value, measurer_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto* view_holder =
      HarmonyValues::UnwrapObject<NativeMarkdownViewHolder>(env, node_value);
  auto* measurer_holder = UnwrapMeasurer(env, measurer_value);
  if (view_holder == nullptr || measurer_holder == nullptr) {
    napi_value result;
    napi_get_boolean(env, false, &result);
    return result;
  }
  napi_value result;
  napi_get_boolean(
      env, view_holder->view_->SetMeasurer(measurer_holder->measurer_.get()),
      &result);
  return result;
}
static napi_value NAPI_Global_createNativeMarkdownNode(
    napi_env env, napi_callback_info info) {
  const auto& [content_value] = HarmonyValues::GetValueFromParams<1>(env, info);
  auto content_handle =
      HarmonyValues::ConvertValue<ArkUI_NodeContentHandle>(env, content_value);
  auto* holder = HarmonyValues::WrapObject<NativeMarkdownViewHolder>(
      env, content_value, nullptr);
  holder->view_->AttachToNodeContent(content_handle);
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownContent(napi_env env,
                                                 napi_callback_info info) {
  const auto& [node_value, content] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto content_string = HarmonyValues::ConvertValue<std::string>(env, content);
  if (auto* holder = UnwrapMeasurer(env, node_value); holder != nullptr) {
    holder->measurer_->SetContent(content_string);
  }
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownStyle(napi_env env,
                                               napi_callback_info info) {
  const auto& [content_value, style_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto style = HarmonyValues::ConvertValue<MarkdownValue>(env, style_value);
  if (style == nullptr || style->GetType() != ValueType::kMap) {
    return nullptr;
  }
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->measurer_->SetStyle(style->AsMap());
  }
  return nullptr;
}
static napi_value NAPI_Global_markDirty(napi_env env, napi_callback_info info) {
  const auto& [content_value] = HarmonyValues::GetValueFromParams<1>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->measurer_->MarkDirty();
  }
  return nullptr;
}
static napi_value NAPI_Global_setMarkdownConfig(napi_env env,
                                                napi_callback_info info) {
  const auto& [content_value, config_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto config = HarmonyValues::ConvertValue<MarkdownValue>(env, config_value);
  if (config == nullptr || config->GetType() != ValueType::kMap) {
    return nullptr;
  }
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->measurer_->SetConfig(config->AsMap());
  }
  return nullptr;
}

static napi_value NAPI_Global_registerImageLoader(napi_env env,
                                                  napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->resource_loader_->SetImageLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerFontLoader(napi_env env,
                                                 napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->resource_loader_->SetFontLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerInlineViewLoader(
    napi_env env, napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->resource_loader_->SetInlineViewLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_registerReplacementViewLoader(
    napi_env env, napi_callback_info info) {
  const auto& [content_value, function] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    holder->resource_loader_->SetReplacementViewLoader(function);
  }
  return nullptr;
}
static napi_value NAPI_Global_bindEvent(napi_env env, napi_callback_info info) {
  const auto& [content_value, name, function] =
      HarmonyValues::GetValueFromParams<3>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
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
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    auto str_name = HarmonyValues::ConvertValue<std::string>(env, name);
    holder->exposure_listener_->BindExposure(str_name,
                                             HarmonyValueRef(env, function));
    holder->ListenExposure();
  }
  return nullptr;
}

static napi_value NAPI_Global_setRequestMeasureCallback(
    napi_env env, napi_callback_info info) {
  const auto& [measurer_value, callback_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto* holder = UnwrapMeasurer(env, measurer_value);
  if (holder == nullptr) {
    return nullptr;
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, callback_value, &type);
  if (type != napi_function) {
    holder->measurer_->SetRequestMeasureCallback(nullptr);
    return nullptr;
  }
  holder->measurer_->SetRequestMeasureCallback(
      [env, callback = HarmonyValueRef(env, callback_value)]() mutable {
        HarmonyValues::CallFunction(env, nullptr, callback.GetValue());
      });
  return nullptr;
}

static napi_value NAPI_Global_applyStyleInRange(napi_env env,
                                                napi_callback_info info) {
  const auto& [content_value, style, start, end] =
      HarmonyValues::GetValueFromParams<4>(env, info);
  if (auto* holder = UnwrapMeasurer(env, content_value); holder != nullptr) {
    auto style_var = HarmonyValues::ConvertValue<MarkdownValue>(env, style);
    if (style_var == nullptr || style_var->GetType() != ValueType::kMap) {
      return nullptr;
    }
    auto start_var = HarmonyValues::ConvertValue<int32_t>(env, start);
    auto end_var = HarmonyValues::ConvertValue<int32_t>(env, end);
    holder->measurer_->GetMarkdownView()->ApplyStyleInRange(style_var->AsMap(),
                                                            start_var, end_var);
  }
  return nullptr;
}

static napi_value NAPI_Global_measureMarkdown(napi_env env,
                                              napi_callback_info info) {
  const auto& [measurer_value, spec_value] =
      HarmonyValues::GetValueFromParams<2>(env, info);
  auto* holder = UnwrapMeasurer(env, measurer_value);
  auto spec = HarmonyValues::ConvertValue<MarkdownValue>(env, spec_value);
  if (holder == nullptr || spec == nullptr ||
      spec->GetType() != ValueType::kMap) {
    return nullptr;
  }
  const auto& map = spec->AsMap();
  const auto get_number = [&map](const char* key, double fallback) {
    auto iter = map.find(key);
    return iter == map.end() ? fallback : iter->second->AsDouble();
  };
  const auto result = holder->measurer_->Measure(
      {.width_ = static_cast<float>(get_number("width", 0)),
       .width_mode_ = static_cast<tttext::LayoutMode>(
           static_cast<int32_t>(get_number("widthMode", 0))),
       .height_ = static_cast<float>(get_number("height", 0)),
       .height_mode_ = static_cast<tttext::LayoutMode>(
           static_cast<int32_t>(get_number("heightMode", 0)))});

  napi_value object;
  napi_create_object(env, &object);
  napi_value width;
  napi_value height;
  napi_value baseline;
  napi_create_double(env, result.width_, &width);
  napi_create_double(env, result.height_, &height);
  napi_create_double(env, result.baseline_, &baseline);
  napi_set_named_property(env, object, "width", width);
  napi_set_named_property(env, object, "height", height);
  napi_set_named_property(env, object, "baseline", baseline);
  return object;
}
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"createNativeMarkdownMeasurer", nullptr,
       NAPI_Global_createNativeMarkdownMeasurer, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"createNativeMarkdownNode", nullptr,
       NAPI_Global_createNativeMarkdownNode, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setNativeMarkdownMeasurer", nullptr,
       NAPI_Global_setNativeMarkdownMeasurer, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setMarkdownContent", nullptr, NAPI_Global_setMarkdownContent, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"setMarkdownStyle", nullptr, NAPI_Global_setMarkdownStyle, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"markDirty", nullptr, NAPI_Global_markDirty, nullptr, nullptr, nullptr,
       napi_default, nullptr},
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
      {"setRequestMeasureCallback", nullptr,
       NAPI_Global_setRequestMeasureCallback, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"applyStyleInRange", nullptr, NAPI_Global_applyStyleInRange, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"measureMarkdown", nullptr, NAPI_Global_measureMarkdown, nullptr,
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
