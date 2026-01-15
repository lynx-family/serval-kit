// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <android/log.h>
#include <jni.h>

#include "parser/SrSVGDOM.h"
#include "platform/android/SrAndroidCanvas.h"

using serval::svg::android::GetEnvForCurrentThread;
using serval::svg::android::InitVM;
using serval::svg::android::SrAndroidCanvas;
using serval::svg::parser::SrSVGDOM;

int registerNativeMethod(JNIEnv* env);
jint render(JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str,
            jfloat left, jfloat top, jfloat width, jfloat height);
jfloatArray calculateViewBoxTransform(JNIEnv* env, jobject j_engine,
                                      jfloat vp_left, jfloat vp_top,
                                      jfloat vp_width, jfloat vp_height,
                                      jfloat vb_left, jfloat vb_top,
                                      jfloat vb_width, jfloat vb_height,
                                      jint align_x, jint align_y, jint scale);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  InitVM(vm);
  JNIEnv* env = GetEnvForCurrentThread();
  if (!env) {
    __android_log_print(ANDROID_LOG_ERROR, "SrSVG", "JNI load fail.");
    return JNI_ERR;
  }
  if (registerNativeMethod(env) != JNI_OK) {
    __android_log_print(ANDROID_LOG_ERROR, "SrSVG",
                        "Register native method fail.");
    return JNI_ERR;
  }
  return JNI_VERSION_1_2;
}

int registerNativeMethod(JNIEnv* env) {
  static const char* render_engine_class_name =
      "com/lynx/serval/svg/SVGRenderEngine";
  jclass render_engine_class = env->FindClass(render_engine_class_name);
  if (render_engine_class == nullptr) {
    __android_log_print(ANDROID_LOG_ERROR, "SrSVG",
                        "Fail to find class: ", render_engine_class_name);
    return JNI_ERR;
  }
  static const JNINativeMethod render_engine_method[] = {
      {
          .name = "render",
          .signature =
              "(Lcom/lynx/serval/svg/SVGRender;Ljava/lang/String;FFFF)I",
          .fnPtr = reinterpret_cast<void*>(render),
      },
      {
          .name = "calculateViewBoxTransform",
          .signature = "(FFFFFFFFIII)[F",
          .fnPtr = reinterpret_cast<void*>(calculateViewBoxTransform),
      },
  };
  if (env->RegisterNatives(render_engine_class, render_engine_method,
                           sizeof(render_engine_method) /
                               sizeof(render_engine_method[0])) != JNI_OK) {
    return JNI_ERR;
  }
  return JNI_OK;
}

jint render(JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str,
            jfloat left, jfloat top, jfloat width, jfloat height) {
  if (!j_str || !j_engine || !j_render) {
    return JNI_ERR;
  }
  int length = env->GetStringLength(j_str);
  if (length <= 0) {
    return JNI_ERR;
  }
  const char* str = env->GetStringUTFChars(j_str, JNI_FALSE);
  if (!str) {
    return JNI_ERR;
  }
  auto svg_dom = SrSVGDOM::make(str, length);
  env->ReleaseStringUTFChars(j_str, str);
  if (!svg_dom) {
    // TODO(dingwang.wxx) add error log
    return JNI_ERR;
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  svg_dom->Render(&sr_android_canvas, view_port);
  return JNI_OK;
}

jfloatArray calculateViewBoxTransform(JNIEnv* env, jobject j_engine,
                                      jfloat vp_left, jfloat vp_top,
                                      jfloat vp_width, jfloat vp_height,
                                      jfloat vb_left, jfloat vb_top,
                                      jfloat vb_width, jfloat vb_height,
                                      jint align_x, jint align_y, jint scale) {
  SrSVGBox view_port{vp_left, vp_top, vp_width, vp_height};
  SrSVGBox view_box{vb_left, vb_top, vb_width, vb_height};
  float xform[6];
  SrSVGPreserveAspectRatio preserve_aspect_ratio{
      static_cast<SrSVGAlign>(align_x), static_cast<SrSVGAlign>(align_y),
      static_cast<SrSVGScale>(scale)};
  calculate_view_box_transform(&view_port, &view_box, preserve_aspect_ratio,
                               xform);
  jfloatArray j_transform = env->NewFloatArray(6);
  env->SetFloatArrayRegion(j_transform, 0, 6, xform);
  return j_transform;
}
