// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_MARKDOWN_MEASURER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_MARKDOWN_MEASURER_H_

#include <jni.h>

#include <memory>
#include <string_view>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/markdown_exposure_listener.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/platform/android/markdown_class_cache.h"
#include "markdown/view/markdown_view.h"

namespace serval::markdown {

class AndroidServalMarkdownView;

class AndroidMarkdownMeasurer : public MarkdownResourceLoader,
                                public MarkdownEventListener,
                                public MarkdownExposureListener,
                                public MarkdownViewMeasureHost {
 public:
  static void Initialize(JNIEnv* env);
  AndroidMarkdownMeasurer(JNIEnv* env, jobject measurer);
  ~AndroidMarkdownMeasurer() override;

  MarkdownView* GetMarkdownView() const { return view_.get(); }
  MeasureResult Measure(MeasureSpec spec) { return view_->Measure(spec); }

  void BindView(AndroidServalMarkdownView* view);
  void SetExposureListenerEnabled(bool enabled);
  void RequestMeasure() override;

  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) override;
  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override;
  void* LoadFont(const char* family, MarkdownFontWeight weight) override;
  MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                              float max_width,
                                              float max_height) override;

  void OnParseEnd() override;
  void OnTextOverflow(MarkdownTextOverflow overflow) override;
  void OnDrawStart() override;
  void OnDrawEnd() override;
  void OnAnimationStep(int32_t animation_step,
                       int32_t max_animation_step) override;
  void OnLinkClicked(const char* url, const char* content) override;
  void OnImageClicked(const char* url) override;
  void OnSelectionChanged(int32_t start_index, int32_t end_index,
                          SelectionHandleType handle,
                          SelectionState state) override;

  void OnLinkAppear(const char* url, const char* content) override;
  void OnLinkDisappear(const char* url, const char* content) override;
  void OnImageAppear(const char* url) override;
  void OnImageDisappear(const char* url) override;

 private:
  int LoadImage(const char* source);
  SizeF GetImageSize(int32_t id);
  std::shared_ptr<AndroidMarkdownView> LoadInlineView(const char* id);
  int64_t LoadFont(const char* family, int32_t weight, int32_t style);

  std::shared_ptr<MarkdownView> view_;
  AndroidServalMarkdownView* bound_view_{nullptr};
  std::vector<std::weak_ptr<AndroidMarkdownView>> pending_subviews_;
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> measurer_ref_;

  static struct Methods {
    jmethodID load_image_{};
    jmethodID get_image_size_{};
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
    jmethodID request_measure_{};
  } methods_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_ANDROID_MARKDOWN_MEASURER_H_
