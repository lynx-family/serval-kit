// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/android_serval_markdown_view.h"
#include <memory>

AndroidServalMarkdownView::AndroidServalMarkdownView(JNIEnv* env, jobject view)
    : AndroidMainView(env, view) {
  AttachDrawable(std::make_unique<lynx::markdown::MarkdownView>(this));
  auto* markdown_view = GetMarkdownView();
  markdown_view->SetPlatformLoader(resource_loader_android_.get());
  markdown_view->SetEventListener(event_android_.get());
}
