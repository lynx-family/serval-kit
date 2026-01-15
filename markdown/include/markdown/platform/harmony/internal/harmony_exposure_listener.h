// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EXPOSURE_LISTENER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EXPOSURE_LISTENER_H_
#include <string>
#include "markdown/markdown_exposure_listener.h"
#include "markdown/platform/harmony/internal/harmony_value_ref.h"
#include "napi/native_api.h"
namespace lynx::markdown {
class HarmonyExposureListener : public MarkdownExposureListener {
 public:
  explicit HarmonyExposureListener(napi_env env);
  ~HarmonyExposureListener() override = default;
  void BindExposure(const std::string& name, HarmonyValueRef function);
  void OnLinkAppear(const char* url, const char* content) override;
  void OnLinkDisappear(const char* url, const char* content) override;
  void OnImageAppear(const char* url) override;
  void OnImageDisappear(const char* url) override;

 private:
  napi_env env_;
  HarmonyValueRef link_appear_;
  HarmonyValueRef link_disappear_;
  HarmonyValueRef image_appear_;
  HarmonyValueRef image_disappear_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_EXPOSURE_LISTENER_H_
