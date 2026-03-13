// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
#include <memory>
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_exposure_listener.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/platform/android/markdown_class_cache.h"
#include "markdown/view/markdown_view.h"
class AndroidServalMarkdownView
    : public AndroidMainView,
      public lynx::markdown::MarkdownResourceLoader,
      public lynx::markdown::MarkdownEventListener,
      public lynx::markdown::MarkdownExposureListener {
 public:
  static void Initialize(JNIEnv* env);
  AndroidServalMarkdownView(JNIEnv* env, jobject view);
  lynx::markdown::MarkdownView* GetMarkdownView() {
    return static_cast<lynx::markdown::MarkdownView*>(drawable_.get());
  }
  int LoadImage(const char* source);
  std::shared_ptr<AndroidMarkdownView> LoadInlineView(const char* id);
  int64_t LoadFont(const char* family, int32_t weight, int32_t style);
  void SetExposureListenerEnabled(bool enabled);

 public:
  std::shared_ptr<lynx::markdown::MarkdownDrawable> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) override;
  std::shared_ptr<lynx::markdown::MarkdownDrawable> LoadInlineView(
      const char* id_selector, float max_width, float max_height) override;
  void* LoadFont(const char* family,
                 lynx::markdown::MarkdownFontWeight weight) override;
  std::shared_ptr<lynx::markdown::MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override;

 public:
  void OnParseEnd() override;
  void OnTextOverflow(lynx::markdown::MarkdownTextOverflow overflow) override;
  void OnDrawStart() override;
  void OnDrawEnd() override;
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override;
  void OnLinkClicked(const char* url, const char* content) override;
  void OnImageClicked(const char* url) override;
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          lynx::markdown::SelectionHandleType handle,
                          lynx::markdown::SelectionState state) override;

 public:
  void OnLinkAppear(const char* url, const char* content) override;
  void OnLinkDisappear(const char* url, const char* content) override;
  void OnImageAppear(const char* url) override;
  void OnImageDisappear(const char* url) override;

 protected:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> view_ref_;

 protected:
  bool exposure_listener_enabled_{false};

 protected:
  static struct Methods {
    jmethodID load_image_{};
    jmethodID load_inline_view_{};
    jmethodID load_font_{};
    jmethodID on_parse_end_{};
    jmethodID on_text_overflow_{};
    jmethodID on_draw_start_{};
    jmethodID on_draw_end_{};
    jmethodID on_animation_step_{};
    jmethodID on_link_clicked_{};
    jmethodID on_image_clicked_{};
    jmethodID on_selection_changed_{};
    jmethodID on_link_appear_{};
    jmethodID on_link_disappear_{};
    jmethodID on_image_appear_{};
    jmethodID on_image_disappear_{};
  } methods_;
};

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_SERVAL_MARKDOWN_VIEW_H_
