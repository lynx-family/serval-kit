// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_UTILS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_UTILS_H_
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "arkui/native_gesture.h"
#include "arkui/native_interface.h"
#include "arkui/native_node.h"
#include "arkui/native_node_napi.h"
#include "arkui/native_type.h"
#include "markdown/utils/markdown_value.h"
#include "napi/native_api.h"
namespace lynx::markdown {
class HarmonyEnv {
 public:
  static napi_env GetEnv();
  static void SetEnv(napi_env env);
};
class HarmonyUIThread {
 public:
  static void Init(napi_env env);
  static void PostTask(std::function<void()> task);
  static void PostDelayedTask(std::function<void()> task,
                              int64_t micro_seconds);
};
template <typename Object>
class HarmonyDefaultDeleter {
 public:
  static void Delete(napi_env env, void* data, void* hint) {
    delete reinterpret_cast<Object*>(data);
  }
};
class HarmonyValues {
 public:
  template <size_t N>
  static std::array<napi_value, N> GetValueFromParams(napi_env env,
                                                      napi_callback_info info) {
    size_t num = N;
    std::array<napi_value, N> result;
    napi_get_cb_info(env, info, &num, result.data(), nullptr, nullptr);
    return result;
  }
  template <typename Object, typename... Args,
            typename Deleter = HarmonyDefaultDeleter<Object>>
  static Object* WrapObject(napi_env env, napi_value value, void* finalize_hint,
                            Args&&... args) {
    auto* object = new Object(args...);
    auto status =
        napi_wrap(env, value, object, Deleter::Delete, finalize_hint, nullptr);
    if (status != napi_ok) {
      delete object;
      return nullptr;
    }
    return object;
  }
  template <typename Object>
  static Object* UnwrapObject(napi_env env, napi_value value) {
    Object* result = nullptr;
    auto status = napi_unwrap(env, value, reinterpret_cast<void**>(&result));
    if (status != napi_ok) {
      return nullptr;
    }
    return result;
  }
  template <typename ValueType>
  static ValueType ConvertValue(napi_env env, napi_value value) {
    static_assert(ValueType::value, "no impl for this value type");
    return {};
  }
  template <>
  ArkUI_NodeContentHandle ConvertValue(napi_env env, napi_value value) {
    ArkUI_NodeContentHandle contentHandle = 0;
    OH_ArkUI_GetNodeContentFromNapiValue(env, value, &contentHandle);
    return contentHandle;
  }
  template <>
  ArkUI_NodeHandle ConvertValue(napi_env env, napi_value value) {
    ArkUI_NodeHandle contentHandle = 0;
    auto status =
        OH_ArkUI_GetNodeHandleFromNapiValue(env, value, &contentHandle);
    if (status != napi_ok) {
      return nullptr;
    }
    return contentHandle;
  }
  template <>
  std::string ConvertValue(napi_env env, napi_value value) {
    size_t str_len = 0;
    std::string str;
    auto status = napi_get_value_string_utf8(env, value, nullptr, 0,
                                             &str_len);  // 获取字符串长度
    if (status != napi_ok) {
      return "";
    }
    str.reserve(str_len + 1);
    str.resize(str_len);
    napi_get_value_string_utf8(env, value, str.data(), str_len + 1, &str_len);
    return str;
  }
  template <>
  int32_t ConvertValue(napi_env env, napi_value value) {
    int32_t result{};
    napi_get_value_int32(env, value, &result);
    return result;
  }
  template <>
  uint32_t ConvertValue(napi_env env, napi_value value) {
    return ConvertValue<int32_t>(env, value);
  }
  template <>
  int64_t ConvertValue(napi_env env, napi_value value) {
    int64_t result{};
    napi_get_value_int64(env, value, &result);
    return result;
  }
  template <>
  uint64_t ConvertValue(napi_env env, napi_value value) {
    return ConvertValue<int64_t>(env, value);
  }
  template <>
  double ConvertValue(napi_env env, napi_value value) {
    double result{};
    napi_get_value_double(env, value, &result);
    return result;
  }
  template <>
  float ConvertValue(napi_env env, napi_value value) {
    return static_cast<float>(ConvertValue<double>(env, value));
  }
  template <>
  bool ConvertValue(napi_env env, napi_value value) {
    bool result{};
    napi_get_value_bool(env, value, &result);
    return result;
  }
  template <>
  std::unique_ptr<lynx::markdown::Value> ConvertValue(napi_env env,
                                                      napi_value value) {
    bool is_array = false;
    napi_is_array(env, value, &is_array);
    if (is_array) {
      return lynx::markdown::Value::MakeArray(
          ConvertArray<std::unique_ptr<lynx::markdown::Value>>(env, value));
    } else {
      napi_valuetype type;
      napi_typeof(env, value, &type);
      switch (type) {
        case napi_boolean:
          return lynx::markdown::Value::MakeBool(
              ConvertValue<bool>(env, value));
        case napi_number:
          return lynx::markdown::Value::MakeDouble(
              ConvertValue<double>(env, value));
        case napi_string:
          return lynx::markdown::Value::MakeString(
              ConvertValue<std::string>(env, value));
        case napi_object:
          return ConvertObjectValue(env, value);
        case napi_undefined:
          return nullptr;
        case napi_symbol:
        case napi_function:
        case napi_external:
        case napi_bigint:
        case napi_null:
        default:
          return lynx::markdown::Value::MakeNull();
      }
    }
  }
  static std::unique_ptr<lynx::markdown::Value> ConvertObjectValue(
      napi_env env, napi_value value) {
    napi_value result;
    auto status = napi_get_property_names(env, value, &result);
    if (status != napi_ok) {
      return lynx::markdown::Value::MakeMap();
    }
    auto names = ConvertArray<std::string>(env, result);
    lynx::markdown::ValueMap map;
    for (auto& name : names) {
      napi_value element;
      napi_get_named_property(env, value, name.c_str(), &element);
      auto value =
          ConvertValue<std::unique_ptr<lynx::markdown::Value>>(env, element);
      if (value == nullptr) {
        continue;
      }
      map[name] =
          ConvertValue<std::unique_ptr<lynx::markdown::Value>>(env, element);
    }
    return lynx::markdown::Value::MakeMap(std::move(map));
  }
  template <typename Value>
  static std::vector<Value> ConvertArray(napi_env env, napi_value value) {
    uint32_t array_len;
    auto status = napi_get_array_length(env, value, &array_len);
    if (status != napi_ok) {
      return {};
    }
    std::vector<Value> array;
    array.reserve(array_len);
    for (auto i = 0u; i < array_len; i++) {
      napi_value element;
      status = napi_get_element(env, value, i, &element);
      if (status != napi_ok) {
        return array;
      } else {
        array.emplace_back(ConvertValue<Value>(env, element));
      }
    }
    return array;
  }

  template <typename Value>
  static napi_value MakeNapiValue(napi_env env, Value v) {
    static_assert(Value::value, "no impl for this value type");
    return nullptr;
  }

  template <>
  napi_value MakeNapiValue(napi_env env, int64_t v) {
    napi_value result;
    napi_create_int64(env, v, &result);
    return result;
  }
  template <>
  napi_value MakeNapiValue(napi_env env, int32_t v) {
    napi_value result;
    napi_create_int32(env, v, &result);
    return result;
  }
  template <>
  napi_value MakeNapiValue(napi_env env, void* v) {
    return MakeNapiValue<int64_t>(env, reinterpret_cast<int64_t>(v));
  }
  template <>
  napi_value MakeNapiValue(napi_env env, std::string_view v) {
    napi_value result;
    napi_create_string_utf8(env, v.data(), v.length(), &result);
    return result;
  }
  template <>
  napi_value MakeNapiValue(napi_env env, const char* v) {
    if (v == nullptr)
      return nullptr;
    return MakeNapiValue<std::string_view>(env, std::string_view(v, strlen(v)));
  }

  template <typename... Args>
  static napi_value CallFunction(napi_env env, napi_value recv,
                                 napi_value function, Args... args) {
    napi_value result{nullptr};
    constexpr int N = sizeof...(Args);
    std::array<napi_value, N> args_value;
    FillValueArray(env, args_value.data(), args...);
    napi_call_function(env, recv, function, N, args_value.data(), &result);
    return result;
  }

 private:
  static void FillValueArray(napi_env env, napi_value* result) {}
  template <typename Var>
  static void FillValueArray(napi_env env, napi_value* result, Var var1) {
    *result = MakeNapiValue(env, var1);
  }
  template <typename Var, typename... Args>
  static void FillValueArray(napi_env env, napi_value* result, Var var1,
                             Args... other) {
    *result = MakeNapiValue(env, var1);
    FillValueArray(env, result + 1, other...);
  }
};

class ArkUINativeAPI {
 public:
  ArkUINativeAPI() {
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_NODE, ArkUI_NativeNodeAPI_1,
                                node_api_);
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_GESTURE, ArkUI_NativeGestureAPI_1,
                                gesture_api_);
  }

  static ArkUI_NativeNodeAPI_1* GetNodeApi() { return GetApi().node_api_; }
  static ArkUI_NativeGestureAPI_1* GetGestureApi() {
    return GetApi().gesture_api_;
  }

 private:
  static const ArkUINativeAPI& GetApi() {
    static ArkUINativeAPI api;
    return api;
  }
  ArkUI_NativeNodeAPI_1* node_api_;
  ArkUI_NativeGestureAPI_1* gesture_api_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_UTILS_H_
