// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_

#include <jni.h>

#include "markdown/platform/android/markdown_class_cache.h"

namespace serval::markdown {

class AndroidMarkdownMeasurer;
class MarkdownView;

class AndroidServalMarkdownView : public AndroidMainView {
 public:
  AndroidServalMarkdownView(JNIEnv* env, jobject view);
  ~AndroidServalMarkdownView() override;

  void SetMeasurer(AndroidMarkdownMeasurer* measurer);
  MarkdownView* GetMarkdownView();

  void OnLayoutFrame(int64_t time);
  void OnRendererFrame(int64_t time);

 private:
  AndroidMarkdownMeasurer* measurer_{nullptr};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
