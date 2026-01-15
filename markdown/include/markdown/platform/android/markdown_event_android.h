// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_EVENT_ANDROID_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_EVENT_ANDROID_H_
#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/platform/android/markdown_class_cache.h"
class MarkdownEventAndroid final
    : public lynx::markdown::MarkdownEventListener {
 public:
  MarkdownEventAndroid(JNIEnv* env, jobject ref) : ref_(env, ref) {}
  ~MarkdownEventAndroid() override = default;
  void OnTextOverflow(lynx::markdown::MarkdownTextOverflow overflow) override {
    if (ref_.Get() != nullptr) {
      const char* type;
      if (overflow == lynx::markdown::MarkdownTextOverflow::kEllipsis) {
        type = "ellipsis";
      } else {
        type = "clip";
      }
      auto env = MarkdownClassCache::GetInstance().GetCurrentJNIEnv();
      auto jstr = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, type);
    }
  }
  void OnImageClicked(const char* url) override {}
  void OnLinkClicked(const char* url, const char* content) override {}
  void OnDrawStart() override {}
  void OnDrawEnd() override {}
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override {}
  void OnParseEnd() override {}
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          lynx::markdown::SelectionHandleType handle,
                          lynx::markdown::SelectionState state) override {}

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> ref_;
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_EVENT_ANDROID_H_
