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

namespace serval::markdown {

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

  static int64_t PackMeasureResult(int32_t width, int32_t height,
                                   int32_t baseline) {
    constexpr int64_t mask = (1LL << 21) - 1LL;
    return ((static_cast<int64_t>(width) & mask) << 42) |
           ((static_cast<int64_t>(height) & mask) << 21) |
           (static_cast<int64_t>(baseline) & mask);
  }

  static int32_t GetMeasurePackWidth(int64_t value) {
    constexpr int64_t mask = (1LL << 21) - 1LL;
    return static_cast<int32_t>((value >> 42) & mask);
  }

  static int32_t GetMeasurePackHeight(int64_t value) {
    constexpr int64_t mask = (1LL << 21) - 1LL;
    return static_cast<int32_t>((value >> 21) & mask);
  }

  static int32_t GetMeasurePackBaseline(int64_t value) {
    constexpr int64_t mask = (1LL << 21) - 1LL;
    return static_cast<int32_t>(value & mask);
  }
};

class AndroidMarkdownView : public MarkdownPlatformView {
 public:
  static void Initialize(JNIEnv* env);
  AndroidMarkdownView();
  AndroidMarkdownView(JNIEnv* env, jobject ref);
  ~AndroidMarkdownView() override = default;
  void UpdateObject(JNIEnv* env, jobject ref);
  void RequestMeasure() override;
  void RequestAlign() override;
  void RequestDraw() override;
  void Align(float left, float top) final;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    SetVisibility(true);
  }
  bool DispatchTap(PointF position, GestureEventType event) {
    if (!tap_gesture_listener_) {
      return false;
    }
    return tap_gesture_listener_(position, event);
  }
  bool DispatchLongPress(PointF position, GestureEventType event) {
    if (!long_press_gesture_listener_) {
      return false;
    }
    return long_press_gesture_listener_(position, event);
  }
  bool DispatchPan(PointF position, PointF motion, GestureEventType event) {
    if (!pan_gesture_listener_) {
      return false;
    }
    return pan_gesture_listener_(position, motion, event);
  }
  PointF GetAlignedPosition() final;
  SizeF GetMeasuredSize() override;
  void SetMeasuredSize(SizeF size) final;
  void SetAlignPosition(PointF position) final;
  void SetVisibility(bool visible) final;
  jobject GetObject() const { return ref_.Get(); }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
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
    jmethodID get_vertical_align_{};
    jmethodID set_size_{};
    jmethodID set_position_{};
    jmethodID set_visibility_{};
  } methods_;
};

class AndroidCustomView : public AndroidMarkdownView,
                          public MarkdownCustomViewHandle {
 public:
  static void Initialize(JNIEnv* env);
  AndroidCustomView();
  AndroidCustomView(JNIEnv* env, jobject ref);
  void AttachDrawable(std::unique_ptr<MarkdownDrawable> drawable) final;
  MarkdownCustomViewHandle* GetCustomViewHandle() final { return this; }
  SizeF GetMeasuredSize() override;

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;
  static struct Methods {
    jmethodID attach_drawable_{};
  } methods_;
};

class AndroidMainView : public AndroidCustomView,
                        public MarkdownViewContainerHandle {
 public:
  static void Initialize(JNIEnv* env);
  AndroidMainView(JNIEnv* env, jobject ref);
  std::shared_ptr<MarkdownPlatformView> CreateCustomSubView() final;
  std::shared_ptr<MarkdownPlatformView> CreateRegionSubView() final;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHandleSubView(
      SelectionHandleType type, float size, float margin, uint32_t color) final;
  std::shared_ptr<MarkdownPlatformView> CreateSelectionHighlightSubView(
      uint32_t color) final;

 public:
  void RemoveSubView(MarkdownPlatformView* subview) final;
  void RemoveAllSubViews() final;
  RectF GetViewRectInScreen() final;
  void UpdateCachedViewRectInScreen();
  MarkdownViewContainerHandle* GetViewContainerHandle() final { return this; }

 protected:
  RectF CalculateViewRectInScreen();
  std::list<std::shared_ptr<AndroidMarkdownView>> subviews_;
  RectF cached_view_rect_in_screen_{};

 protected:
  static struct Methods {
    jmethodID create_custom_subview_{};
    jmethodID create_region_subview_{};
    jmethodID create_selection_handle_subview_{};
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
  jclass GetStringClass() const { return string_class_.Get(); }

  JNIEnv* GetCurrentJNIEnv() const {
    thread_local JNIEnv* env = nullptr;
    if (env == nullptr) {
      java_vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    }
    return env;
  }

 public:
  JavaVM* java_vm_{nullptr};
  lynx::base::android::ScopedGlobalJavaRef<jclass> string_class_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_CLASS_CACHE_H_
