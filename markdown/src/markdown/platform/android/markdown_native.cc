// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <string>

#include "base/include/platform/android/jni_convert_helper.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/platform/android/android_serval_markdown_view.h"
#include "markdown/platform/android/markdown_java_canvas_helper.h"
#include "markdown/platform/android/markdown_resource_loader_android.h"
using lynx::markdown::MarkdownDrawable;
using lynx::markdown::MeasureSpec;
extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_CustomDrawable_measure(JNIEnv* env, jclass clazz,
                                              jlong drawable, jfloat width,
                                              jint width_mode, jfloat height,
                                              jint height_mode) {
  auto* drawable_object = reinterpret_cast<MarkdownDrawable*>(drawable);
  MeasureSpec spec{
      .width_ = width,
      .width_mode_ = static_cast<tttext::LayoutMode>(width_mode),
      .height_ = height,
      .height_mode_ = static_cast<tttext::LayoutMode>(height_mode),
  };
  auto size = drawable_object->Measure(spec);
  return MarkdownJNIUtils::PackIntPair(static_cast<int32_t>(size.width_),
                                       static_cast<int32_t>(size.height_));
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_lynx_markdown_CustomDrawable_nativeDrawCustomDrawable(JNIEnv* env,
                                                               jclass clazz,
                                                               jlong drawable) {
  auto* drawable_object = reinterpret_cast<MarkdownDrawable*>(drawable);
  MarkdownJavaCanvasHelper helper;
  drawable_object->Draw(&helper, 0, 0);
  auto& buffer = helper.GetBuffer();
  return MarkdownJNIUtils::CreateByteArray(
      env, reinterpret_cast<const jbyte*>(buffer.GetBuffer()),
      static_cast<jsize>(buffer.GetSize()));
}

AndroidServalMarkdownView* ConvertView(jlong instance) {
  return reinterpret_cast<AndroidServalMarkdownView*>(instance);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeCreateInstance(JNIEnv* env,
                                                               jobject thiz) {
  return reinterpret_cast<jlong>(new AndroidServalMarkdownView(env, thiz));
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeDestroyInstance(
    JNIEnv* env, jobject thiz, jlong instance) {
  delete ConvertView(instance);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetContent(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong instance,
                                                           jstring content) {
  auto* view = ConvertView(instance);
  auto length = env->GetStringUTFLength(content);
  auto* chars = env->GetStringUTFChars(content, nullptr);
  view->GetMarkdownView()->SetContent({chars, static_cast<size_t>(length)});
  view->GetMarkdownView()->SetStyle({});
  env->ReleaseStringUTFChars(content, chars);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_Markdown_initialClassCache(JNIEnv* env, jclass clazz) {
  MarkdownClassCache::GetInstance().Initial(env);
}
