// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_RESOURCE_LOADER_IMPL_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_RESOURCE_LOADER_IMPL_H_
#include "markdown/platform/harmony/harmony_resource_loader.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/platform/harmony/internal/harmony_value_ref.h"
#include "napi/native_api.h"
namespace lynx::markdown {
class HarmonyResourceLoaderImpl : public IHarmonyResourceLoader {
 public:
  explicit HarmonyResourceLoaderImpl(napi_env env);
  ~HarmonyResourceLoaderImpl() = default;

  OH_Drawing_Font* LoadFont(const char* family) override;
  ArkUI_NodeHandle LoadImageView(const char* src, float desire_width,
                                 float desire_height, float max_width,
                                 float max_height,
                                 float border_radius) override;
  ArkUI_NodeHandle LoadInlineView(const char* id, float max_width,
                                  float max_height) override;
  ArkUI_NodeHandle LoadReplacementView(void* ud, float max_width,
                                       float max_height) override;

  void SetFontLoader(napi_value loader);
  void SetImageLoader(napi_value loader);
  void SetInlineViewLoader(napi_value loader);
  void SetReplacementViewLoader(napi_value loader);

 private:
  napi_env env_{nullptr};
  HarmonyValueRef font_loader_{};
  HarmonyValueRef inline_view_loader_{};
  HarmonyValueRef image_loader_{};
  HarmonyValueRef replacement_view_loader_{};
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_RESOURCE_LOADER_IMPL_H_
