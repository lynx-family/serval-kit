// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_exposure_listener.h"
#include <string>
#include <utility>
#include "markdown/platform/harmony/internal/harmony_utils.h"
namespace lynx::markdown {
HarmonyExposureListener::HarmonyExposureListener(napi_env env) : env_(env) {}
void HarmonyExposureListener::BindExposure(const std::string& name,
                                           HarmonyValueRef function) {
  if (name == "linkAppear") {
    link_appear_ = std::move(function);
  } else if (name == "linkDisappear") {
    link_disappear_ = std::move(function);
  } else if (name == "imageAppear") {
    image_appear_ = std::move(function);
  } else if (name == "imageDisappear") {
    image_disappear_ = std::move(function);
  }
}
void HarmonyExposureListener::OnLinkAppear(const char* url,
                                           const char* content) {
  if (link_appear_.IsNull())
    return;
  HarmonyUIThread::PostTask(
      [this, u = std::string(url), c = std::string(content)]() {
        HarmonyValues::CallFunction(env_, nullptr, link_appear_.GetValue(),
                                    u.c_str(), c.c_str());
      });
}
void HarmonyExposureListener::OnLinkDisappear(const char* url,
                                              const char* content) {
  if (link_disappear_.IsNull())
    return;
  HarmonyUIThread::PostTask(
      [this, u = std::string(url), c = std::string(content)]() {
        HarmonyValues::CallFunction(env_, nullptr, link_disappear_.GetValue(),
                                    u.c_str(), c.c_str());
      });
}
void HarmonyExposureListener::OnImageAppear(const char* url) {
  if (image_appear_.IsNull())
    return;
  HarmonyUIThread::PostTask([this, u = std::string(url)]() {
    HarmonyValues::CallFunction(env_, nullptr, image_appear_.GetValue(),
                                u.c_str());
  });
}
void HarmonyExposureListener::OnImageDisappear(const char* url) {
  if (image_disappear_.IsNull())
    return;
  HarmonyUIThread::PostTask([this, u = std::string(url)]() {
    HarmonyValues::CallFunction(env_, nullptr, image_disappear_.GetValue(),
                                u.c_str());
  });
}
}  // namespace lynx::markdown
