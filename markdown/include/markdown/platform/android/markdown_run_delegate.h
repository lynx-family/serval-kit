// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_

#include <textra/i_canvas_helper.h>

#include "markdown/platform/android/tttext_run_delegate.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"

class MarkdownResourceLoaderAndroid;
class MarkdownRunDelegate final : public TTTextRunDelegate {
 public:
  MarkdownRunDelegate(int id, MarkdownResourceLoaderAndroid* loader,
                      float radius)
      : TTTextRunDelegate(id, 0, 0, 0), loader_(loader), radius_(radius) {}
  ~MarkdownRunDelegate() override = default;

 public:
  void Layout() override {}
  float GetRadius() const { return radius_; }

 private:
  MarkdownResourceLoaderAndroid* loader_;
  float radius_{0};
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
