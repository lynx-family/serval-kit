// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_CLASS_CACHE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_CLASS_CACHE_H_

#include <jni.h>

#include <list>
#include <memory>

#include "base/include/platform/android/scoped_java_ref.h"
#include "markdown/view/markdown_platform_view.h"
class MarkdownJNIUtils {
 public:
  static jbyteArray CreateByteArray(JNIEnv* env, const jbyte* bytes,
                                    jsize length) {
    auto array = env->NewByteArray(length);  // NO LINT
    env->SetByteArrayRegion(array, 0, length, bytes);
    return array;
  }
  static int64_t PackIntPair(int32_t left, int32_t right) {
    int64_t result = 0;
    return ((result | left) << 32) | right;
  }
  static int32_t GetIntPackFirst(int64_t value) {
    return static_cast<int32_t>(value >> 32);
  }
  static int32_t GetIntPackSecond(int64_t value) {
    return static_cast<int32_t>(value & 0xffffffff);
  }
};

class AndroidMarkdownView : public lynx::markdown::MarkdownPlatformView {
 public:
  static void Initialize(JNIEnv* env);
  AndroidMarkdownView(JNIEnv* env, jobject ref);
  void RequestMeasure() override;
  void RequestAlign() override;
  void RequestDraw() override;
  lynx::markdown::SizeF Measure(lynx::markdown::MeasureSpec spec) override;
  void Align(float left, float top) final;
  void Draw(tttext::ICanvasHelper* canvas) override {}
  lynx::markdown::PointF GetAlignedPosition() final;
  lynx::markdown::SizeF GetMeasuredSize() override;
  void SetMeasuredSize(lynx::markdown::SizeF size) final;
  void SetAlignPosition(lynx::markdown::PointF position) final;
  void SetVisibility(bool visible) final;
  jobject GetObject() const { return ref_.Get(); }

 protected:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> ref_;

 protected:
  static struct Methods {
    jmethodID request_measure_{};
    jmethodID request_align_{};
    jmethodID request_draw_{};
    jmethodID measure_{};
    jmethodID align_{};
    jmethodID get_size_{};
    jmethodID get_position_{};
    jmethodID set_size_{};
    jmethodID set_position_{};
    jmethodID set_visibility_{};
  } methods_;
};

class AndroidCustomView : public AndroidMarkdownView,
                          public lynx::markdown::MarkdownCustomViewHandle {
 public:
  static void Initialize(JNIEnv* env);
  AndroidCustomView(JNIEnv* env, jobject ref);
  void AttachDrawable(
      std::unique_ptr<lynx::markdown::MarkdownDrawable> drawable) final;
  lynx::markdown::MarkdownCustomViewHandle* GetCustomViewHandle() final {
    return this;
  }
  lynx::markdown::SizeF Measure(lynx::markdown::MeasureSpec spec) override;
  lynx::markdown::SizeF GetMeasuredSize() override;

 protected:
  static struct Methods {
    jmethodID attach_drawable_{};
  } methods_;
};

class AndroidMainView : public AndroidCustomView,
                        public lynx::markdown::MarkdownViewContainerHandle {
 public:
  static void Initialize(JNIEnv* env);
  AndroidMainView(JNIEnv* env, jobject ref);
  MarkdownPlatformView* CreateCustomSubView() final;
  void RemoveSubView(lynx::markdown::MarkdownPlatformView* subview) final;
  void RemoveAllSubViews() final;
  lynx::markdown::RectF GetViewRectInScreen() final;
  lynx::markdown::MarkdownViewContainerHandle* GetViewContainerHandle() final {
    return this;
  }

 protected:
  std::list<std::unique_ptr<AndroidMarkdownView>> subviews_;

 protected:
  static struct Methods {
    jmethodID create_custom_subview_{};
    jmethodID remove_subview_{};
    jmethodID remove_all_subviews_{};
    jmethodID get_view_rect_in_screen_{};
  } methods_;
};

class MarkdownClassCache {
 public:
  void Initial(JNIEnv* env);
  static MarkdownClassCache& GetInstance() {
    static MarkdownClassCache instance;
    return instance;
  }
  static JNIEnv* GetEnv() { return GetInstance().GetCurrentJNIEnv(); }
  JavaVM* GetJavaVm() const { return java_vm_; }

  JNIEnv* GetCurrentJNIEnv() const {
    thread_local JNIEnv* env = nullptr;
    if (env == nullptr) {
      java_vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    }
    return env;
  }

 public:
  JavaVM* java_vm_;
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_CLASS_CACHE_H_
