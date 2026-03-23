// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/android/markdown_class_cache.h"

#include <memory>
#include <utility>

#include "base/include/platform/android/jni_utils.h"
#include "markdown/platform/android/android_serval_markdown_view.h"
#include "markdown/view/markdown_selection_view.h"

void MarkdownClassCache::Initial(JNIEnv* env) {
  env->GetJavaVM(&java_vm_);
  AndroidMarkdownView::Initialize(env);
  AndroidCustomView::Initialize(env);
  AndroidMainView::Initialize(env);
  AndroidServalMarkdownView::Initialize(env);
  lynx::base::android::InitVM(java_vm_);
}
AndroidMarkdownView::Methods AndroidMarkdownView::methods_{};
void AndroidMarkdownView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/IMarkdownViewHandle");
  methods_.request_measure_ = env->GetMethodID(clazz, "requestMeasure", "()V");
  methods_.request_align_ = env->GetMethodID(clazz, "requestAlign", "()V");
  methods_.request_draw_ = env->GetMethodID(clazz, "requestDraw", "()V");
  methods_.measure_ = env->GetMethodID(clazz, "measure", "(IIII)J");
  methods_.align_ = env->GetMethodID(clazz, "align", "(II)V");
  methods_.get_position_ = env->GetMethodID(clazz, "getPosition", "()J");
  methods_.get_size_ = env->GetMethodID(clazz, "getSize", "()J");
  methods_.get_vertical_align_ =
      env->GetMethodID(clazz, "getVerticalAlign", "()I");
  methods_.set_size_ = env->GetMethodID(clazz, "setSize", "(II)V");
  methods_.set_position_ = env->GetMethodID(clazz, "setPosition", "(II)V");
  methods_.set_visibility_ = env->GetMethodID(clazz, "setVisibility", "(Z)V");
}
AndroidMarkdownView::AndroidMarkdownView() = default;
AndroidMarkdownView::AndroidMarkdownView(JNIEnv* env, jobject ref)
    : ref_(env, ref) {}
void AndroidMarkdownView::UpdateObject(JNIEnv* env, jobject ref) {
  ref_.Reset(env, ref);
}
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
lynx::markdown::MeasureResult AndroidMarkdownView::OnMeasure(
    lynx::markdown::MeasureSpec spec) {
  auto* env = MarkdownClassCache::GetEnv();
  jlong result = env->CallLongMethod(
      ref_.Get(), methods_.measure_, static_cast<jint>(spec.width_),
      static_cast<jint>(spec.width_mode_), static_cast<jint>(spec.height_),
      static_cast<jint>(spec.height_mode_));
  const auto w =
      static_cast<float>(MarkdownJNIUtils::GetMeasurePackWidth(result));
  const auto h =
      static_cast<float>(MarkdownJNIUtils::GetMeasurePackHeight(result));
  const auto baseline =
      static_cast<float>(MarkdownJNIUtils::GetMeasurePackBaseline(result));
  return {.width_ = w, .height_ = h, .baseline_ = baseline};
}
void AndroidMarkdownView::Align(float left, float top) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.align_, static_cast<jint>(left),
                      static_cast<jint>(top));
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
  env->CallVoidMethod(ref_.Get(), methods_.set_size_,
                      static_cast<jint>(size.width_),
                      static_cast<jint>(size.height_));
}
void AndroidMarkdownView::SetAlignPosition(lynx::markdown::PointF position) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.set_position_,
                      static_cast<jint>(position.x_),
                      static_cast<jint>(position.y_));
}
void AndroidMarkdownView::SetVisibility(bool visible) {
  auto* env = MarkdownClassCache::GetEnv();
  env->CallVoidMethod(ref_.Get(), methods_.set_visibility_, visible);
}

void AndroidCustomView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/CustomDrawView");
  methods_.attach_drawable_ = env->GetMethodID(clazz, "attachDrawable", "(J)V");
}
AndroidCustomView::Methods AndroidCustomView::methods_{};
AndroidCustomView::AndroidCustomView() : AndroidMarkdownView() {}
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
lynx::markdown::MeasureResult AndroidCustomView::OnMeasure(
    lynx::markdown::MeasureSpec spec) {
  if (drawable_ == nullptr) {
    return {};
  }
  const auto result = drawable_->Measure(spec);
  SetMeasuredSize({.width_ = result.width_, .height_ = result.height_});
  return result;
}
lynx::markdown::SizeF AndroidCustomView::GetMeasuredSize() {
  return {drawable_->GetAdvance(),
          drawable_->GetDescent() - drawable_->GetAscent()};
}
void AndroidMainView::Initialize(JNIEnv* env) {
  auto clazz = env->FindClass("com/lynx/markdown/ServalMarkdownView");
  methods_.create_custom_subview_ = env->GetMethodID(
      clazz, "createCustomView", "()Lcom/lynx/markdown/CustomDrawView;");
  methods_.create_region_subview_ = env->GetMethodID(
      clazz, "createRegionView", "()Lcom/lynx/markdown/CustomDrawView;");
  methods_.create_selection_handle_subview_ =
      env->GetMethodID(clazz, "createSelectionHandleView",
                       "(J)Lcom/lynx/markdown/SelectionHandleView;");
  methods_.remove_subview_ =
      env->GetMethodID(clazz, "removeSubView", "(Landroid/view/View;)V");
  methods_.remove_all_subviews_ =
      env->GetMethodID(clazz, "removeAllSubviews", "()V");
  methods_.get_view_rect_in_screen_ =
      env->GetMethodID(clazz, "getVisibleVerticalRangeInScreen", "()J");
}
AndroidMainView::Methods AndroidMainView::methods_{};
AndroidMainView::AndroidMainView(JNIEnv* env, jobject ref)
    : AndroidCustomView(env, ref) {}
std::shared_ptr<lynx::markdown::MarkdownPlatformView>
AndroidMainView::CreateCustomSubView() {
  auto* env = MarkdownClassCache::GetEnv();
  auto object =
      env->CallObjectMethod(ref_.Get(), methods_.create_custom_subview_);
  auto subview = std::make_shared<AndroidCustomView>(env, object);
  subviews_.insert(subviews_.end(), subview);
  return std::static_pointer_cast<lynx::markdown::MarkdownPlatformView>(
      subview);
}

std::shared_ptr<lynx::markdown::MarkdownPlatformView>
AndroidMainView::CreateRegionSubView() {
  auto* env = MarkdownClassCache::GetEnv();
  auto object =
      env->CallObjectMethod(ref_.Get(), methods_.create_region_subview_);
  auto subview = std::make_shared<AndroidCustomView>(env, object);
  subviews_.insert(subviews_.end(), subview);
  return std::static_pointer_cast<lynx::markdown::MarkdownPlatformView>(
      subview);
}

std::shared_ptr<lynx::markdown::MarkdownPlatformView>
AndroidMainView::CreateSelectionHandleSubView(
    lynx::markdown::SelectionHandleType type, float size, float margin,
    uint32_t color) {
  auto* env = MarkdownClassCache::GetEnv();
  auto subview = std::make_shared<AndroidCustomView>();
  subviews_.insert(subviews_.end(), subview);
  auto object = env->CallObjectMethod(ref_.Get(),
                                      methods_.create_selection_handle_subview_,
                                      reinterpret_cast<jlong>(subview.get()));
  subview->UpdateObject(env, object);
  const auto view =
      std::static_pointer_cast<lynx::markdown::MarkdownPlatformView>(subview);
  auto selection_handle =
      std::make_unique<lynx::markdown::MarkdownSelectionHandle>(size, margin,
                                                                type, color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(selection_handle));
  return view;
}
std::shared_ptr<lynx::markdown::MarkdownPlatformView>
AndroidMainView::CreateSelectionHighlightSubView(uint32_t color) {
  const auto view = CreateCustomSubView();
  auto highlight =
      std::make_unique<lynx::markdown::MarkdownSelectionHighlight>();
  highlight->SetColor(color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(highlight));
  return view;
}
void AndroidMainView::RemoveSubView(
    lynx::markdown::MarkdownPlatformView* subview) {
  auto iter =
      std::find_if(subviews_.begin(), subviews_.end(),
                   [subview](const std::shared_ptr<AndroidMarkdownView>& view) {
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
  subviews_.clear();
}
lynx::markdown::RectF AndroidMainView::CalculateViewRectInScreen() {
  auto* env = MarkdownClassCache::GetEnv();
  jlong packed =
      env->CallLongMethod(ref_.Get(), methods_.get_view_rect_in_screen_);
  const float top =
      static_cast<float>(MarkdownJNIUtils::GetIntPackFirst(packed));
  const float bottom =
      static_cast<float>(MarkdownJNIUtils::GetIntPackSecond(packed));
  const auto size = AndroidMarkdownView::GetMeasuredSize();
  return lynx::markdown::RectF::MakeLTRB(0.0f, top, size.width_, bottom);
}

void AndroidMainView::UpdateCachedViewRectInScreen() {
  cached_view_rect_in_screen_ = CalculateViewRectInScreen();
}

lynx::markdown::RectF AndroidMainView::GetViewRectInScreen() {
  return cached_view_rect_in_screen_;
}
