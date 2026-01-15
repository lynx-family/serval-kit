// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VALUE_REF_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VALUE_REF_H_
#include "napi/native_api.h"
namespace lynx::markdown {
class HarmonyValueRef {
 public:
  HarmonyValueRef() = default;
  HarmonyValueRef(napi_env env, napi_value value) {
    env_ = env;
    ref_ = nullptr;
    napi_create_reference(env, value, 1, &ref_);
  }
  HarmonyValueRef(napi_env env, napi_ref ref) {
    env_ = env;
    ref_ = ref;
    uint32_t count;
    napi_reference_ref(env_, ref_, &count);
  }
  HarmonyValueRef(const HarmonyValueRef& ref)
      : HarmonyValueRef(ref.env_, ref.ref_) {}
  HarmonyValueRef(HarmonyValueRef&& ref) noexcept {
    env_ = ref.env_;
    ref_ = ref.ref_;
    ref.env_ = nullptr;
    ref.ref_ = nullptr;
  }
  HarmonyValueRef& operator=(const HarmonyValueRef& ref) {
    Destroy();
    ref_ = ref.ref_;
    env_ = ref.env_;
    uint32_t count;
    napi_reference_ref(env_, ref_, &count);
    return *this;
  }
  HarmonyValueRef& operator=(HarmonyValueRef&& ref) noexcept {
    Destroy();
    ref_ = ref.ref_;
    env_ = ref.env_;
    ref.ref_ = nullptr;
    ref.env_ = nullptr;
    return *this;
  }
  ~HarmonyValueRef() { Destroy(); }
  napi_ref GetRef() { return ref_; }
  napi_value GetValue() {
    if (IsNull()) {
      return nullptr;
    }
    napi_value value{nullptr};
    napi_get_reference_value(env_, ref_, &value);
    return value;
  }
  bool IsNull() const { return ref_ == nullptr; }

 private:
  void Destroy() {
    if (ref_ != nullptr) {
      uint32_t count;
      napi_reference_unref(env_, ref_, &count);
      if (count == 0) {
        napi_delete_reference(env_, ref_);
      }
    }
  }

 private:
  napi_env env_{nullptr};
  napi_ref ref_{nullptr};
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VALUE_REF_H_
