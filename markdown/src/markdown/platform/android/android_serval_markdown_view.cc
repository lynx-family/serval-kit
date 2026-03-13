// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/android_serval_markdown_view.h"
#include <cstdint>
#include <memory>

#include "base/include/platform/android/jni_convert_helper.h"
#include "markdown/platform/android/markdown_run_delegate.h"

AndroidServalMarkdownView::Methods AndroidServalMarkdownView::methods_{};

void AndroidServalMarkdownView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/ServalMarkdownView");
  methods_.load_image_ =
      env->GetMethodID(clazz, "loadImage", "(Ljava/lang/String;)I");
  methods_.load_inline_view_ = env->GetMethodID(
      clazz, "loadInlineView",
      "(Ljava/lang/String;)Lcom/lynx/markdown/IMarkdownViewHandle;");
  methods_.load_font_ =
      env->GetMethodID(clazz, "loadFont", "(Ljava/lang/String;II)J");
  methods_.on_parse_end_ = env->GetMethodID(clazz, "onParseEnd", "()V");
  methods_.on_text_overflow_ =
      env->GetMethodID(clazz, "onTextOverflow", "(I)V");
  methods_.on_draw_start_ = env->GetMethodID(clazz, "onDrawStart", "()V");
  methods_.on_draw_end_ = env->GetMethodID(clazz, "onDrawEnd", "()V");
  methods_.on_animation_step_ =
      env->GetMethodID(clazz, "onAnimationStep", "(II)V");
  methods_.on_link_clicked_ = env->GetMethodID(
      clazz, "onLinkClicked", "(Ljava/lang/String;Ljava/lang/String;)V");
  methods_.on_image_clicked_ =
      env->GetMethodID(clazz, "onImageClicked", "(Ljava/lang/String;)V");
  methods_.on_selection_changed_ =
      env->GetMethodID(clazz, "onSelectionChanged", "(IIII)V");
  methods_.on_link_appear_ = env->GetMethodID(
      clazz, "onLinkAppear", "(Ljava/lang/String;Ljava/lang/String;)V");
  methods_.on_link_disappear_ = env->GetMethodID(
      clazz, "onLinkDisappear", "(Ljava/lang/String;Ljava/lang/String;)V");
  methods_.on_image_appear_ =
      env->GetMethodID(clazz, "onImageAppear", "(Ljava/lang/String;)V");
  methods_.on_image_disappear_ =
      env->GetMethodID(clazz, "onImageDisappear", "(Ljava/lang/String;)V");
}

AndroidServalMarkdownView::AndroidServalMarkdownView(JNIEnv* env, jobject view)
    : AndroidMainView(env, view), view_ref_(env, view) {
  AttachDrawable(std::make_unique<lynx::markdown::MarkdownView>(this));
  auto* markdown_view = GetMarkdownView();
  markdown_view->SetResourceLoader(
      static_cast<lynx::markdown::MarkdownResourceLoader*>(this));
  markdown_view->SetEventListener(
      static_cast<lynx::markdown::MarkdownEventListener*>(this));
}

int AndroidServalMarkdownView::LoadImage(const char* source) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr || source == nullptr) {
    return 0;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, source);
  return env->CallIntMethod(view_ref_.Get(), methods_.load_image_, jstr.Get());
}

std::shared_ptr<AndroidMarkdownView> AndroidServalMarkdownView::LoadInlineView(
    const char* id) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr || id == nullptr) {
    return nullptr;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, id);
  auto object = env->CallObjectMethod(view_ref_.Get(),
                                      methods_.load_inline_view_, jstr.Get());
  if (object == nullptr) {
    return nullptr;
  }
  auto subview = std::make_shared<AndroidMarkdownView>(env, object);
  subviews_.insert(subviews_.end(), subview);
  env->DeleteLocalRef(object);
  return subview;
}

int64_t AndroidServalMarkdownView::LoadFont(const char* family, int32_t weight,
                                            int32_t style) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr || family == nullptr) {
    return 0;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, family);
  return env->CallLongMethod(view_ref_.Get(), methods_.load_font_, jstr.Get(),
                             static_cast<jint>(weight),
                             static_cast<jint>(style));
}

void AndroidServalMarkdownView::SetExposureListenerEnabled(bool enabled) {
  if (exposure_listener_enabled_ == enabled) {
    return;
  }
  exposure_listener_enabled_ = enabled;
  auto* markdown_view = GetMarkdownView();
  if (markdown_view == nullptr) {
    return;
  }
  if (enabled) {
    markdown_view->SetExposureListener(
        static_cast<lynx::markdown::MarkdownExposureListener*>(this));
  } else {
    markdown_view->SetExposureListener(nullptr);
  }
}

std::shared_ptr<lynx::markdown::MarkdownDrawable>
AndroidServalMarkdownView::LoadImage(const char* src, float desire_width,
                                     float desire_height, float max_width,
                                     float max_height, float border_radius) {
  int id = LoadImage(src);
  if (id == 0) {
    return nullptr;
  }
  return std::make_shared<MarkdownRunDelegate>(id, desire_width, desire_height,
                                               border_radius);
}

std::shared_ptr<lynx::markdown::MarkdownDrawable>
AndroidServalMarkdownView::LoadInlineView(const char* id_selector,
                                          float max_width, float max_height) {
  auto view = LoadInlineView(id_selector);
  return std::static_pointer_cast<lynx::markdown::MarkdownDrawable>(
      std::move(view));
}

int32_t ConvertToAndroidFontWeight(lynx::markdown::MarkdownFontWeight weight) {
  switch (weight) {
    case lynx::markdown::MarkdownFontWeight::kBold:
      return 700;
    case lynx::markdown::MarkdownFontWeight::k100:
      return 100;
    case lynx::markdown::MarkdownFontWeight::k200:
      return 200;
    case lynx::markdown::MarkdownFontWeight::k300:
      return 300;
    case lynx::markdown::MarkdownFontWeight::k400:
      return 400;
    case lynx::markdown::MarkdownFontWeight::k500:
      return 500;
    case lynx::markdown::MarkdownFontWeight::k600:
      return 600;
    case lynx::markdown::MarkdownFontWeight::k700:
      return 700;
    case lynx::markdown::MarkdownFontWeight::k800:
      return 800;
    case lynx::markdown::MarkdownFontWeight::k900:
      return 900;
    case lynx::markdown::MarkdownFontWeight::kNormal:
    default:
      return 400;
  }
}

void* AndroidServalMarkdownView::LoadFont(
    const char* family, lynx::markdown::MarkdownFontWeight weight) {
  const int32_t w = ConvertToAndroidFontWeight(weight);
  const auto idx = LoadFont(family, w, 0);
  return reinterpret_cast<void*>(static_cast<uintptr_t>(idx));
}

std::shared_ptr<lynx::markdown::MarkdownDrawable>
AndroidServalMarkdownView::LoadReplacementView(void* ud, int32_t id,
                                               float max_width,
                                               float max_height) {
  return nullptr;
}

void AndroidServalMarkdownView::OnParseEnd() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_parse_end_);
}

void AndroidServalMarkdownView::OnTextOverflow(
    lynx::markdown::MarkdownTextOverflow overflow) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_text_overflow_,
                      static_cast<jint>(overflow));
}

void AndroidServalMarkdownView::OnDrawStart() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_draw_start_);
}

void AndroidServalMarkdownView::OnDrawEnd() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_draw_end_);
}

void AndroidServalMarkdownView::OnAnimationStep(int32_t animation_step,
                                                int32_t max_animation_step) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_animation_step_,
                      static_cast<jint>(animation_step),
                      static_cast<jint>(max_animation_step));
}

void AndroidServalMarkdownView::OnLinkClicked(const char* url,
                                              const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  lynx::base::android::ScopedLocalJavaRef<jstring> j_content;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  if (content != nullptr) {
    j_content = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, content);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_link_clicked_, j_url.Get(),
                      j_content.Get());
}

void AndroidServalMarkdownView::OnImageClicked(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_image_clicked_, j_url.Get());
}

void AndroidServalMarkdownView::OnSelectionChanged(
    int32_t start_index, int32_t end_index,
    lynx::markdown::SelectionHandleType handle,
    lynx::markdown::SelectionState state) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_selection_changed_,
                      static_cast<jint>(start_index),
                      static_cast<jint>(end_index), static_cast<jint>(handle),
                      static_cast<jint>(state));
}

void AndroidServalMarkdownView::OnLinkAppear(const char* url,
                                             const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  lynx::base::android::ScopedLocalJavaRef<jstring> j_content;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  if (content != nullptr) {
    j_content = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, content);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_link_appear_, j_url.Get(),
                      j_content.Get());
}

void AndroidServalMarkdownView::OnLinkDisappear(const char* url,
                                                const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  lynx::base::android::ScopedLocalJavaRef<jstring> j_content;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  if (content != nullptr) {
    j_content = lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, content);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_link_disappear_, j_url.Get(),
                      j_content.Get());
}

void AndroidServalMarkdownView::OnImageAppear(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_image_appear_, j_url.Get());
}

void AndroidServalMarkdownView::OnImageDisappear(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || view_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(view_ref_.Get(), methods_.on_image_disappear_,
                      j_url.Get());
}
