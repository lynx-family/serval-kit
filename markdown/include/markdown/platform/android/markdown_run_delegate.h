// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_

#include <textra/i_canvas_helper.h>

#include "markdown/platform/android/tttext_run_delegate.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"

class MarkdownRunDelegate final : public TTTextRunDelegate {
 public:
  MarkdownRunDelegate(
      int id, lynx::base::android::ScopedWeakGlobalJavaRef<jobject> manager)
      : TTTextRunDelegate(id, 0, 0, 0), manager_(manager) {}
  ~MarkdownRunDelegate() override = default;

 public:
  void Layout() override {
    auto* env = MarkdownClassCache::GetInstance().GetCurrentJNIEnv();
  }

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> manager_;
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
