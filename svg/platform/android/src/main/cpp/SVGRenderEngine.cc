// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <android/log.h>
#include <jni.h>
#include <mutex>
#include <string>
#include <vector>

#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/android/SrAndroidCanvas.h"
#include "renderer/SrSVGAnimationState.h"

using serval::svg::android::GetEnvForCurrentThread;
using serval::svg::android::InitVM;
using serval::svg::android::SrAndroidCanvas;
using serval::svg::parser::SrSVGDOM;
using serval::svg::renderer::SrSVGAnimationState;

int registerNativeMethod(JNIEnv* env);
jobjectArray renderWithDiagnostics(JNIEnv* env, jobject j_engine,
                                   jobject j_render, jstring j_str, jfloat left,
                                   jfloat top, jfloat width, jfloat height,
                                   jstring j_color);
jobjectArray renderAtTimeWithDiagnostics(JNIEnv* env, jobject j_engine,
                                         jobject j_render, jstring j_str,
                                         jfloat left, jfloat top, jfloat width,
                                         jfloat height, jstring j_color,
                                         jdouble seconds);
jlong createSession(JNIEnv* env, jobject j_engine, jstring j_str);
void destroySession(JNIEnv* env, jobject j_engine, jlong handle);
jboolean sessionHasAnimations(JNIEnv* env, jobject j_engine, jlong handle);
jdouble sessionAnimationTimelineEndSeconds(JNIEnv* env, jobject j_engine,
                                           jlong handle);
void sessionStartAnimation(JNIEnv* env, jobject j_engine, jlong handle);
void sessionStartAnimationAtFrameTimeNanos(JNIEnv* env, jobject j_engine,
                                           jlong handle,
                                           jlong frame_time_nanos);
void sessionStopAnimation(JNIEnv* env, jobject j_engine, jlong handle);
void sessionResetAnimationClock(JNIEnv* env, jobject j_engine, jlong handle);
jboolean sessionIsAnimationRunning(JNIEnv* env, jobject j_engine, jlong handle);
jboolean sessionNeedsAnimationFrame(JNIEnv* env, jobject j_engine,
                                    jlong handle);
jboolean sessionOnFrameTimeNanos(JNIEnv* env, jobject j_engine, jlong handle,
                                 jlong frame_time_nanos);
jdouble sessionCurrentAnimationSeconds(JNIEnv* env, jobject j_engine,
                                       jlong handle);
jobjectArray renderSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color, jdouble seconds);
void renderSessionAtTime(JNIEnv* env, jobject j_engine, jobject j_render,
                         jlong handle, jfloat left, jfloat top, jfloat width,
                         jfloat height, jstring j_color, jdouble seconds);
void renderSessionCurrentFrame(JNIEnv* env, jobject j_engine, jobject j_render,
                               jlong handle, jfloat left, jfloat top,
                               jfloat width, jfloat height, jstring j_color);
jfloatArray calculateViewBoxTransform(JNIEnv* env, jobject j_engine,
                                      jfloat vp_left, jfloat vp_top,
                                      jfloat vp_width, jfloat vp_height,
                                      jfloat vb_left, jfloat vb_top,
                                      jfloat vb_width, jfloat vb_height,
                                      jint align_x, jint align_y, jint scale);
std::unique_ptr<SrSVGDOM> CreateSVGDom(
    JNIEnv* env, jstring j_str,
    std::vector<serval::svg::parser::SrSVGDiagnostic>* diagnostics);
jint RenderSvgDom(JNIEnv* env, jobject j_engine, jobject j_render,
                  SrSVGDOM* svg_dom, jfloat left, jfloat top, jfloat width,
                  jfloat height);
jint RenderSvgDomAtTime(JNIEnv* env, jobject j_engine, jobject j_render,
                        SrSVGDOM* svg_dom, jfloat left, jfloat top,
                        jfloat width, jfloat height, jdouble seconds);
void ApplyDefaultColor(JNIEnv* env, SrSVGDOM* svg_dom, jstring j_color);
jobject CreateJavaDiagnostic(
    JNIEnv* env, const serval::svg::parser::SrSVGDiagnostic& diagnostic);
jobjectArray CreateJavaDiagnosticArray(
    JNIEnv* env,
    const std::vector<serval::svg::parser::SrSVGDiagnostic>& diagnostics);

namespace {

struct NativeSvgSession {
  mutable std::mutex mutex;
  std::unique_ptr<SrSVGDOM> dom;
  SrSVGAnimationState animation_state;
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
};

NativeSvgSession* AsSvgSession(jlong handle) {
  return reinterpret_cast<NativeSvgSession*>(handle);
}

}  // namespace

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
    __android_log_print(ANDROID_LOG_ERROR, "SrSVG", "Fail to find class: %s",
                        render_engine_class_name);
    return JNI_ERR;
  }
  static const JNINativeMethod render_engine_method[] = {
      {
          .name = "renderWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;Ljava/lang/"
                       "String;FFFFLjava/lang/String;)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(renderWithDiagnostics),
      },
      {
          .name = "renderAtTimeWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;Ljava/lang/"
                       "String;FFFFLjava/lang/String;D)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(renderAtTimeWithDiagnostics),
      },
      {
          .name = "createSession",
          .signature = "(Ljava/lang/String;)J",
          .fnPtr = reinterpret_cast<void*>(createSession),
      },
      {
          .name = "destroySession",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(destroySession),
      },
      {
          .name = "sessionHasAnimations",
          .signature = "(J)Z",
          .fnPtr = reinterpret_cast<void*>(sessionHasAnimations),
      },
      {
          .name = "sessionAnimationTimelineEndSeconds",
          .signature = "(J)D",
          .fnPtr = reinterpret_cast<void*>(sessionAnimationTimelineEndSeconds),
      },
      {
          .name = "sessionStartAnimation",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(sessionStartAnimation),
      },
      {
          .name = "sessionStartAnimationAtFrameTimeNanos",
          .signature = "(JJ)V",
          .fnPtr =
              reinterpret_cast<void*>(sessionStartAnimationAtFrameTimeNanos),
      },
      {
          .name = "sessionStopAnimation",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(sessionStopAnimation),
      },
      {
          .name = "sessionResetAnimationClock",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(sessionResetAnimationClock),
      },
      {
          .name = "sessionIsAnimationRunning",
          .signature = "(J)Z",
          .fnPtr = reinterpret_cast<void*>(sessionIsAnimationRunning),
      },
      {
          .name = "sessionNeedsAnimationFrame",
          .signature = "(J)Z",
          .fnPtr = reinterpret_cast<void*>(sessionNeedsAnimationFrame),
      },
      {
          .name = "sessionOnFrameTimeNanos",
          .signature = "(JJ)Z",
          .fnPtr = reinterpret_cast<void*>(sessionOnFrameTimeNanos),
      },
      {
          .name = "sessionCurrentAnimationSeconds",
          .signature = "(J)D",
          .fnPtr = reinterpret_cast<void*>(sessionCurrentAnimationSeconds),
      },
      {
          .name = "renderSessionAtTimeWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFLjava/lang/"
                       "String;D)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(renderSessionAtTimeWithDiagnostics),
      },
      {
          .name = "renderSessionAtTime",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFLjava/lang/"
                       "String;D)V",
          .fnPtr = reinterpret_cast<void*>(renderSessionAtTime),
      },
      {
          .name = "renderSessionCurrentFrame",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFLjava/lang/"
                       "String;)V",
          .fnPtr = reinterpret_cast<void*>(renderSessionCurrentFrame),
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

jobjectArray renderWithDiagnostics(JNIEnv* env, jobject j_engine,
                                   jobject j_render, jstring j_str, jfloat left,
                                   jfloat top, jfloat width, jfloat height,
                                   jstring j_color) {
  std::vector<serval::svg::parser::SrSVGDiagnostic> build_diagnostics;
  auto svg_dom = CreateSVGDom(env, j_str, &build_diagnostics);
  if (!svg_dom) {
    return CreateJavaDiagnosticArray(env, build_diagnostics);
  }
  ApplyDefaultColor(env, svg_dom.get(), j_color);
  RenderSvgDom(env, j_engine, j_render, svg_dom.get(), left, top, width,
               height);
  return CreateJavaDiagnosticArray(env, svg_dom->diagnostics());
}

jobjectArray renderAtTimeWithDiagnostics(JNIEnv* env, jobject j_engine,
                                         jobject j_render, jstring j_str,
                                         jfloat left, jfloat top, jfloat width,
                                         jfloat height, jstring j_color,
                                         jdouble seconds) {
  std::vector<serval::svg::parser::SrSVGDiagnostic> build_diagnostics;
  auto svg_dom = CreateSVGDom(env, j_str, &build_diagnostics);
  if (!svg_dom) {
    return CreateJavaDiagnosticArray(env, build_diagnostics);
  }
  ApplyDefaultColor(env, svg_dom.get(), j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, svg_dom.get(), left, top, width,
                     height, seconds);
  return CreateJavaDiagnosticArray(env, svg_dom->diagnostics());
}

jlong createSession(JNIEnv* env, jobject j_engine, jstring j_str) {
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
  auto svg_dom = CreateSVGDom(env, j_str, &diagnostics);
  (void)j_engine;
  if (!svg_dom) {
    return 0;
  }
  auto* session = new NativeSvgSession();
  session->dom = std::move(svg_dom);
  session->animation_state.SetHasAnimations(session->dom->HasAnimations());
  session->animation_state.SetAnimationTimelineEndSeconds(
      session->dom->AnimationTimelineEndSeconds());
  session->diagnostics = std::move(diagnostics);
  return reinterpret_cast<jlong>(session);
}

void destroySession(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  if (auto* svg_session = AsSvgSession(handle)) {
    delete svg_session;
    return;
  }
}

jboolean sessionHasAnimations(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return JNI_FALSE;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.HasAnimations() ? JNI_TRUE : JNI_FALSE;
}

jdouble sessionAnimationTimelineEndSeconds(JNIEnv* env, jobject j_engine,
                                           jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return 0.0;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.AnimationTimelineEndSeconds();
}

void sessionStartAnimation(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  session->animation_state.Start();
}

void sessionStartAnimationAtFrameTimeNanos(JNIEnv* env, jobject j_engine,
                                           jlong handle,
                                           jlong frame_time_nanos) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  session->animation_state.StartAtTimeNanos(
      static_cast<int64_t>(frame_time_nanos));
}

void sessionStopAnimation(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  session->animation_state.Stop();
}

void sessionResetAnimationClock(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  session->animation_state.ResetClock();
}

jboolean sessionIsAnimationRunning(JNIEnv* env, jobject j_engine,
                                   jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return JNI_FALSE;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.IsRunning() ? JNI_TRUE : JNI_FALSE;
}

jboolean sessionNeedsAnimationFrame(JNIEnv* env, jobject j_engine,
                                    jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return JNI_FALSE;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.NeedsAnimationFrame() ? JNI_TRUE : JNI_FALSE;
}

jboolean sessionOnFrameTimeNanos(JNIEnv* env, jobject j_engine, jlong handle,
                                 jlong frame_time_nanos) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return JNI_FALSE;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.OnFrameTimeNanos(
             static_cast<int64_t>(frame_time_nanos))
             ? JNI_TRUE
             : JNI_FALSE;
}

jdouble sessionCurrentAnimationSeconds(JNIEnv* env, jobject j_engine,
                                       jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsSvgSession(handle);
  if (!session) {
    return 0.0;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  return session->animation_state.CurrentSeconds();
}

jobjectArray renderSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color, jdouble seconds) {
  auto* session = AsSvgSession(handle);
  if (!session || !session->dom) {
    return CreateJavaDiagnosticArray(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  ApplyDefaultColor(env, session->dom.get(), j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, session->dom.get(), left, top,
                     width, height, seconds);
  return CreateJavaDiagnosticArray(env, session->dom->diagnostics());
}

void renderSessionAtTime(JNIEnv* env, jobject j_engine, jobject j_render,
                         jlong handle, jfloat left, jfloat top, jfloat width,
                         jfloat height, jstring j_color, jdouble seconds) {
  auto* session = AsSvgSession(handle);
  if (!session || !session->dom) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  ApplyDefaultColor(env, session->dom.get(), j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, session->dom.get(), left, top,
                     width, height, seconds);
}

void renderSessionCurrentFrame(JNIEnv* env, jobject j_engine, jobject j_render,
                               jlong handle, jfloat left, jfloat top,
                               jfloat width, jfloat height, jstring j_color) {
  auto* session = AsSvgSession(handle);
  if (!session || !session->dom) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  ApplyDefaultColor(env, session->dom.get(), j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, session->dom.get(), left, top,
                     width, height, session->animation_state.CurrentSeconds());
}

std::unique_ptr<SrSVGDOM> CreateSVGDom(
    JNIEnv* env, jstring j_str,
    std::vector<serval::svg::parser::SrSVGDiagnostic>* diagnostics) {
  if (!j_str) {
    return nullptr;
  }
  const char* str = env->GetStringUTFChars(j_str, JNI_FALSE);
  if (!str) {
    return nullptr;
  }
  jsize utf8_length = env->GetStringUTFLength(j_str);
  if (utf8_length <= 0) {
    env->ReleaseStringUTFChars(j_str, str);
    return nullptr;
  }
  auto svg_dom =
      SrSVGDOM::make(str, static_cast<size_t>(utf8_length), diagnostics);
  env->ReleaseStringUTFChars(j_str, str);
  if (!svg_dom) {
    __android_log_print(ANDROID_LOG_ERROR, "SrSVG",
                        "Fail to build SVG DOM from input content.");
    return nullptr;
  }
  return svg_dom;
}

jint RenderSvgDom(JNIEnv* env, jobject j_engine, jobject j_render,
                  SrSVGDOM* svg_dom, jfloat left, jfloat top, jfloat width,
                  jfloat height) {
  if (!j_engine || !j_render || !svg_dom) {
    return JNI_ERR;
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  svg_dom->Render(&sr_android_canvas, view_port);
  return JNI_OK;
}

jint RenderSvgDomAtTime(JNIEnv* env, jobject j_engine, jobject j_render,
                        SrSVGDOM* svg_dom, jfloat left, jfloat top,
                        jfloat width, jfloat height, jdouble seconds) {
  if (!j_engine || !j_render || !svg_dom) {
    return JNI_ERR;
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  svg_dom->RenderAtTime(&sr_android_canvas, view_port, seconds);
  return JNI_OK;
}

void ApplyDefaultColor(JNIEnv* env, SrSVGDOM* svg_dom, jstring j_color) {
  if (!svg_dom) {
    return;
  }
  if (j_color != nullptr) {
    const char* color_str = env->GetStringUTFChars(j_color, JNI_FALSE);
    if (color_str != nullptr) {
      uint32_t default_color = 0;
      if (parse_svg_color(color_str, &default_color)) {
        svg_dom->SetDefaultColor(default_color);
      } else {
        svg_dom->ResetDefaultColor();
      }
      env->ReleaseStringUTFChars(j_color, color_str);
      return;
    }
  }
  svg_dom->ResetDefaultColor();
}

jobject CreateJavaDiagnostic(
    JNIEnv* env, const serval::svg::parser::SrSVGDiagnostic& diagnostic) {
  jclass diagnostic_class =
      env->FindClass("com/lynx/serval/svg/SVGRender$SVGDiagnostic");
  if (!diagnostic_class) {
    return nullptr;
  }
  jmethodID constructor = env->GetMethodID(
      diagnostic_class, "<init>", "(ILjava/lang/String;Ljava/lang/String;Z)V");
  if (!constructor) {
    return nullptr;
  }
  jstring message = env->NewStringUTF(diagnostic.message.c_str());
  jstring subject = env->NewStringUTF(diagnostic.subject.c_str());
  jobject result = env->NewObject(
      diagnostic_class, constructor, static_cast<jint>(diagnostic.code),
      message, subject, static_cast<jboolean>(diagnostic.fatal));
  env->DeleteLocalRef(message);
  env->DeleteLocalRef(subject);
  return result;
}

jobjectArray CreateJavaDiagnosticArray(
    JNIEnv* env,
    const std::vector<serval::svg::parser::SrSVGDiagnostic>& diagnostics) {
  jclass diagnostic_class =
      env->FindClass("com/lynx/serval/svg/SVGRender$SVGDiagnostic");
  if (!diagnostic_class) {
    return nullptr;
  }
  jobjectArray result = env->NewObjectArray(
      static_cast<jsize>(diagnostics.size()), diagnostic_class, nullptr);
  for (jsize i = 0; i < static_cast<jsize>(diagnostics.size()); ++i) {
    jobject diagnostic = CreateJavaDiagnostic(env, diagnostics[i]);
    env->SetObjectArrayElement(result, i, diagnostic);
    env->DeleteLocalRef(diagnostic);
  }
  return result;
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
