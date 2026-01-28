// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/android/markdown_class_cache.h"

#include <memory>
#include <utility>

#include "base/include/platform/android/jni_utils.h"
void MarkdownClassCache::Initial(JNIEnv* env) {
  env->GetJavaVM(&java_vm_);
  AndroidMarkdownView::Initialize(env);
  AndroidCustomView::Initialize(env);
  AndroidMainView::Initialize(env);
  lynx::base::android::InitVM(java_vm_);
}
AndroidMarkdownView::Methods AndroidMarkdownView::methods_{};
void AndroidMarkdownView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/MarkdownViewHandle");
  methods_.request_measure_ = env->GetMethodID(clazz, "requestMeasure", "()V");
  methods_.request_align_ = env->GetMethodID(clazz, "requestAlign", "()V");
  methods_.request_draw_ = env->GetMethodID(clazz, "requestDraw", "()V");
  methods_.measure_ = env->GetMethodID(clazz, "measure", "(IIII)J");
  methods_.align_ = env->GetMethodID(clazz, "align", "(II)V");
  methods_.get_position_ = env->GetMethodID(clazz, "getPosition", "()J");
  methods_.get_size_ = env->GetMethodID(clazz, "getSize", "()J");
  methods_.set_size_ = env->GetMethodID(clazz, "setSize", "(II)V");
  methods_.set_position_ = env->GetMethodID(clazz, "setPosition", "(II)V");
  methods_.set_visibility_ = env->GetMethodID(clazz, "setVisibility", "(Z)V");
}
AndroidMarkdownView::AndroidMarkdownView(JNIEnv* env, jobject ref)
    : ref_(env, ref) {}
void AndroidMarkdownView::RequestMeasure() {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.request_measure_);
}
void AndroidMarkdownView::RequestAlign() {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.request_align_);
}
void AndroidMarkdownView::RequestDraw() {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.request_draw_);
}
lynx::markdown::SizeF AndroidMarkdownView::Measure(
    lynx::markdown::MeasureSpec spec) {
  auto* env = MarkdownClassCache::GetEnv();
  jlong result =
      env->CallLongMethod(ref_.Get(), methods_.measure_, spec.width_,
                          static_cast<jint>(spec.width_mode_), spec.height_,
                          static_cast<jint>(spec.height_mode_));
  return {
      .width_ = static_cast<float>(MarkdownJNIUtils::GetIntPackFirst(result)),
      .height_ = static_cast<float>(MarkdownJNIUtils::GetIntPackSecond(result)),
  };
}
void AndroidMarkdownView::Align(float left, float top) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.align_, left, top);
}
lynx::markdown::PointF AndroidMarkdownView::GetAlignedPosition() {
  auto* env = MarkdownClassCache::GetEnv();
  jlong result = env->CallLongMethod(ref_.Get(), methods_.get_position_);
  return {
      .x_ = static_cast<float>(MarkdownJNIUtils::GetIntPackFirst(result)),
      .y_ = static_cast<float>(MarkdownJNIUtils::GetIntPackSecond(result)),
  };
}
lynx::markdown::SizeF AndroidMarkdownView::GetMeasuredSize() {
  auto* env = MarkdownClassCache::GetEnv();
  jlong result = env->CallLongMethod(ref_.Get(), methods_.get_size_);
  return {
      .width_ = static_cast<float>(MarkdownJNIUtils::GetIntPackFirst(result)),
      .height_ = static_cast<float>(MarkdownJNIUtils::GetIntPackSecond(result)),
  };
}
void AndroidMarkdownView::SetMeasuredSize(lynx::markdown::SizeF size) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.set_size_, size.width_,
                      size.height_);
}
void AndroidMarkdownView::SetAlignPosition(lynx::markdown::PointF position) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.set_position_, position.x_,
                      position.y_);
}
void AndroidMarkdownView::SetVisibility(bool visible) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.set_visibility_, visible);
}

void AndroidCustomView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/MarkdownViewHandle");
  methods_.attach_drawable_ = env->GetMethodID(clazz, "attachDrawable", "(J)V");
}
AndroidCustomView::Methods AndroidCustomView::methods_{};
AndroidCustomView::AndroidCustomView(JNIEnv* env, jobject ref)
    : AndroidMarkdownView(env, ref) {}
void AndroidCustomView::AttachDrawable(
    std::unique_ptr<lynx::markdown::MarkdownDrawable> drawable) {
  lynx::markdown::MarkdownCustomViewHandle::AttachDrawable(std::move(drawable));
  auto* drawable_ptr = drawable_.get();
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.attach_drawable_,
                      reinterpret_cast<int64_t>(drawable_ptr));
}
lynx::markdown::SizeF AndroidCustomView::Measure(
    lynx::markdown::MeasureSpec spec) {
  auto size = drawable_->Measure(spec);
  SetMeasuredSize(size);
  return size;
}
lynx::markdown::SizeF AndroidCustomView::GetMeasuredSize() {
  return {drawable_->GetWidth(), drawable_->GetHeight()};
}
void AndroidMainView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/MarkdownViewHandle");
  methods_.create_custom_subview_ =
      env->GetMethodID(clazz, "createCustomDrawView",
                       "()Lcom/lynx/markdown/MarkdownViewHandle;");
  methods_.remove_subview_ = env->GetMethodID(
      clazz, "removeSubview", "(Lcom/lynx/markdown/MarkdownViewHandle;)V");
  methods_.remove_all_subviews_ =
      env->GetMethodID(clazz, "removeAllSubviews", "()V");
  methods_.get_view_rect_in_screen_ =
      env->GetMethodID(clazz, "getRectInScreen", "()[F");
}
AndroidMainView::Methods AndroidMainView::methods_{};
AndroidMainView::AndroidMainView(JNIEnv* env, jobject ref)
    : AndroidCustomView(env, ref) {}
lynx::markdown::MarkdownPlatformView* AndroidMainView::CreateCustomSubView() {
  auto* env = MarkdownClassCache::GetEnv();
  auto object =
      env->CallObjectMethod(ref_.Get(), methods_.create_custom_subview_);
  auto subview = std::make_unique<AndroidCustomView>(env, object);
  auto subview_ptr = subview.get();
  subviews_.insert(subviews_.end(), std::move(subview));
  return subview_ptr;
}
void AndroidMainView::RemoveSubView(
    lynx::markdown::MarkdownPlatformView* subview) {
  auto iter =
      std::find_if(subviews_.begin(), subviews_.end(),
                   [subview](const std::unique_ptr<AndroidMarkdownView>& view) {
                     return subview == view.get();
                   });
  if (iter != subviews_.end()) {
    auto* env = MarkdownClassCache::GetEnv();
    env->CallVoidMethod(ref_.Get(), methods_.remove_subview_,
                        (*iter)->GetObject());
    subviews_.erase(iter);
  }
}
void AndroidMainView::RemoveAllSubViews() {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.remove_all_subviews_);
}
lynx::markdown::RectF AndroidMainView::GetViewRectInScreen() {
  auto* env = MarkdownClassCache::GetEnv();
  jobject result =
      env->CallObjectMethod(ref_.Get(), methods_.get_view_rect_in_screen_);
  auto array = static_cast<jfloatArray>(result);
  float values[4]{0, 0, 0, 0};
  env->GetFloatArrayRegion(array, 0, 4, values);
  return lynx::markdown::RectF::MakeLTRB(values[0], values[1], values[2],
                                         values[3]);
}
