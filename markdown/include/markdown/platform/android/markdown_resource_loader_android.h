// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RESOURCE_LOADER_ANDROID_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RESOURCE_LOADER_ANDROID_H_

#include <memory>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "markdown/markdown_resource_loader.h"
#include "markdown/platform/android/markdown_class_cache.h"
#include "markdown/platform/android/markdown_run_delegate.h"
#include "markdown/platform/android/tttext_run_delegate.h"

class MarkdownResourceLoaderAndroid
    : public lynx::markdown::MarkdownResourceLoader {
 public:
  explicit MarkdownResourceLoaderAndroid(jobject loader) {
    loader_ = lynx::base::android::ScopedWeakGlobalJavaRef<jobject>(
        lynx::base::android::AttachCurrentThread(), loader);
  }

 public:
  void* LoadFont(const char* family) override { return nullptr; }
  lynx::markdown::MarkdownPlatformView* LoadImageView(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) override {
    return nullptr;
  }
  std::shared_ptr<lynx::markdown::MarkdownDrawable> LoadBackgroundDrawable(
      lynx::markdown::MarkdownBackgroundStylePart* background_style,
      float border_radius, float font_size, float root_font_size) override {
    return nullptr;
  }
  lynx::markdown::MarkdownPlatformView* LoadInlineView(
      const char* id_selector, float max_width, float max_height) override {
    return nullptr;
  }
  lynx::markdown::MarkdownPlatformView* LoadReplacementView(
      void* ud, int id, float max_width, float max_height) override {
    return nullptr;
  }

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> loader_;
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RESOURCE_LOADER_ANDROID_H_
