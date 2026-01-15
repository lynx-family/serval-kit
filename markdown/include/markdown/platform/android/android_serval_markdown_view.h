// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
#include <memory>
#include "markdown/platform/android/markdown_class_cache.h"
#include "markdown/platform/android/markdown_event_android.h"
#include "markdown/platform/android/markdown_resource_loader_android.h"
#include "markdown/view/markdown_view.h"
class AndroidServalMarkdownView : public AndroidMainView {
 public:
  AndroidServalMarkdownView(JNIEnv* env, jobject view);
  lynx::markdown::MarkdownView* GetMarkdownView() {
    return static_cast<lynx::markdown::MarkdownView*>(drawable_.get());
  }

 protected:
  std::unique_ptr<MarkdownResourceLoaderAndroid> resource_loader_android_;
  std::unique_ptr<MarkdownEventAndroid> event_android_;
};

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
