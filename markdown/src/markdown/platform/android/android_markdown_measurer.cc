// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/android_markdown_measurer.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include "base/include/platform/android/jni_convert_helper.h"
#include "markdown/element/markdown_context.h"
#include "markdown/platform/android/android_serval_markdown_view.h"
#include "markdown/platform/android/markdown_platform_android.h"
#include "markdown/platform/android/markdown_run_delegate.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_view.h"

namespace serval::markdown {
namespace {

int32_t ConvertToAndroidFontWeight(MarkdownFontWeight weight) {
  switch (weight) {
    case MarkdownFontWeight::kBold:
      return 700;
    case MarkdownFontWeight::k100:
      return 100;
    case MarkdownFontWeight::k200:
      return 200;
    case MarkdownFontWeight::k300:
      return 300;
    case MarkdownFontWeight::k400:
      return 400;
    case MarkdownFontWeight::k500:
      return 500;
    case MarkdownFontWeight::k600:
      return 600;
    case MarkdownFontWeight::k700:
      return 700;
    case MarkdownFontWeight::k800:
      return 800;
    case MarkdownFontWeight::k900:
      return 900;
    case MarkdownFontWeight::kNormal:
    default:
      return 400;
  }
}

}  // namespace

AndroidMarkdownMeasurer::Methods AndroidMarkdownMeasurer::methods_{};

void AndroidMarkdownMeasurer::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/MarkdownMeasurer");
  methods_.load_image_ =
      env->GetMethodID(clazz, "loadImage", "(Ljava/lang/String;)I");
  methods_.get_image_size_ = env->GetMethodID(clazz, "getImageSize", "(I)J");
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
  methods_.request_measure_ = env->GetMethodID(clazz, "requestMeasure", "()V");
  env->DeleteLocalRef(clazz);
}

AndroidMarkdownMeasurer::AndroidMarkdownMeasurer(JNIEnv* env, jobject measurer)
    : measurer_ref_(env, measurer) {
  view_ = std::make_shared<MarkdownView>(
      nullptr, this,
      std::make_shared<MarkdownContext>(CreateAndroidMarkdownPlatform()));
  auto* markdown_view = GetMarkdownView();
  markdown_view->SetSelectionHandleSize(MarkdownScreenMetrics::DPToPx(15));
  markdown_view->SetSelectionHandleTouchMargin(
      MarkdownScreenMetrics::DPToPx(20));
  markdown_view->SetResourceLoader(this);
  markdown_view->SetEventListener(this);
}

AndroidMarkdownMeasurer::~AndroidMarkdownMeasurer() {
  auto* markdown_view = GetMarkdownView();
  markdown_view->SetExposureListener(nullptr);
  markdown_view->SetEventListener(nullptr);
  markdown_view->SetResourceLoader(nullptr);
}

void AndroidMarkdownMeasurer::SetExposureListenerEnabled(bool enabled) {
  GetMarkdownView()->SetExposureListener(enabled ? this : nullptr);
}

void AndroidMarkdownMeasurer::BindView(AndroidServalMarkdownView* view) {
  if (view == nullptr || bound_view_ != nullptr) {
    return;
  }
  bound_view_ = view;
  for (const auto& pending_subview : pending_subviews_) {
    if (auto subview = pending_subview.lock(); subview != nullptr) {
      bound_view_->AddSubView(subview);
    }
  }
  pending_subviews_.clear();
  view->AttachDrawable(view_);
  view_->SetView(view);
}

void AndroidMarkdownMeasurer::RequestMeasure() {
  auto* env = MarkdownClassCache::GetEnv();
  const auto measurer = measurer_ref_.Get();
  if (env == nullptr || measurer == nullptr) {
    return;
  }
  env->CallVoidMethod(measurer, methods_.request_measure_);
}

int AndroidMarkdownMeasurer::LoadImage(const char* source) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr || source == nullptr) {
    return 0;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, source);
  return env->CallIntMethod(measurer_ref_.Get(), methods_.load_image_,
                            jstr.Get());
}

SizeF AndroidMarkdownMeasurer::GetImageSize(int32_t id) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr || id <= 0) {
    return {};
  }
  const auto packed = env->CallLongMethod(
      measurer_ref_.Get(), methods_.get_image_size_, static_cast<jint>(id));
  return {
      .width_ = static_cast<float>(
          std::max(0, MarkdownJNIUtils::GetIntPackFirst(packed))),
      .height_ = static_cast<float>(
          std::max(0, MarkdownJNIUtils::GetIntPackSecond(packed))),
  };
}

std::shared_ptr<AndroidMarkdownView> AndroidMarkdownMeasurer::LoadInlineView(
    const char* id) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr || id == nullptr) {
    return nullptr;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, id);
  auto object = env->CallObjectMethod(measurer_ref_.Get(),
                                      methods_.load_inline_view_, jstr.Get());
  if (object == nullptr) {
    return nullptr;
  }
  auto result = std::make_shared<AndroidMarkdownView>(env, object);
  if (bound_view_ != nullptr) {
    bound_view_->AddSubView(result);
  } else {
    pending_subviews_.erase(
        std::remove_if(pending_subviews_.begin(), pending_subviews_.end(),
                       [](const auto& subview) { return subview.expired(); }),
        pending_subviews_.end());
    pending_subviews_.emplace_back(result);
  }
  env->DeleteLocalRef(object);
  return result;
}

int64_t AndroidMarkdownMeasurer::LoadFont(const char* family, int32_t weight,
                                          int32_t style) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr || family == nullptr) {
    return 0;
  }
  auto jstr =
      lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, family);
  return env->CallLongMethod(measurer_ref_.Get(), methods_.load_font_,
                             jstr.Get(), static_cast<jint>(weight),
                             static_cast<jint>(style));
}

std::shared_ptr<MarkdownDrawable> AndroidMarkdownMeasurer::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  const int id = LoadImage(src);
  if (id == 0) {
    return nullptr;
  }
  const auto size = GetImageSize(id);
  return std::make_shared<MarkdownRunDelegate>(
      id, size.width_, size.height_, desire_width, desire_height, max_width,
      max_height, border_radius);
}

std::shared_ptr<MarkdownDrawable> AndroidMarkdownMeasurer::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  return std::static_pointer_cast<MarkdownDrawable>(
      LoadInlineView(id_selector));
}

void* AndroidMarkdownMeasurer::LoadFont(const char* family,
                                        MarkdownFontWeight weight) {
  const auto index = LoadFont(family, ConvertToAndroidFontWeight(weight), 0);
  return reinterpret_cast<void*>(static_cast<uintptr_t>(index));
}

MarkdownReplacementView AndroidMarkdownMeasurer::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  return {};
}

void AndroidMarkdownMeasurer::OnParseEnd() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_parse_end_);
  }
}

void AndroidMarkdownMeasurer::OnTextOverflow(MarkdownTextOverflow overflow) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_text_overflow_,
                        static_cast<jint>(overflow));
  }
}

void AndroidMarkdownMeasurer::OnDrawStart() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_draw_start_);
  }
}

void AndroidMarkdownMeasurer::OnDrawEnd() {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_draw_end_);
  }
}

void AndroidMarkdownMeasurer::OnAnimationStep(int32_t animation_step,
                                              int32_t max_animation_step) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_animation_step_,
                        static_cast<jint>(animation_step),
                        static_cast<jint>(max_animation_step));
  }
}

void AndroidMarkdownMeasurer::OnLinkClicked(const char* url,
                                            const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
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
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_link_clicked_,
                      j_url.Get(), j_content.Get());
}

void AndroidMarkdownMeasurer::OnImageClicked(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_image_clicked_,
                      j_url.Get());
}

void AndroidMarkdownMeasurer::OnSelectionChanged(int32_t start_index,
                                                 int32_t end_index,
                                                 SelectionHandleType handle,
                                                 SelectionState state) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env != nullptr && measurer_ref_.Get() != nullptr) {
    env->CallVoidMethod(measurer_ref_.Get(), methods_.on_selection_changed_,
                        static_cast<jint>(start_index),
                        static_cast<jint>(end_index), static_cast<jint>(handle),
                        static_cast<jint>(state));
  }
}

void AndroidMarkdownMeasurer::OnLinkAppear(const char* url,
                                           const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
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
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_link_appear_,
                      j_url.Get(), j_content.Get());
}

void AndroidMarkdownMeasurer::OnLinkDisappear(const char* url,
                                              const char* content) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
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
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_link_disappear_,
                      j_url.Get(), j_content.Get());
}

void AndroidMarkdownMeasurer::OnImageAppear(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_image_appear_,
                      j_url.Get());
}

void AndroidMarkdownMeasurer::OnImageDisappear(const char* url) {
  auto* env = MarkdownClassCache::GetEnv();
  if (env == nullptr || measurer_ref_.Get() == nullptr) {
    return;
  }
  lynx::base::android::ScopedLocalJavaRef<jstring> j_url;
  if (url != nullptr) {
    j_url =
        lynx::base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, url);
  }
  env->CallVoidMethod(measurer_ref_.Get(), methods_.on_image_disappear_,
                      j_url.Get());
}

}  // namespace serval::markdown
