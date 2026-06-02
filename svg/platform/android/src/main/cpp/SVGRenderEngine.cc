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

using serval::svg::android::GetEnvForCurrentThread;
using serval::svg::android::InitVM;
using serval::svg::android::SrAndroidCanvas;
using serval::svg::parser::SrSVGDOM;
using serval::svg::parser::SrSVGDOMIncrementalBuilder;
using serval::svg::parser::SrSVGDOMStreamBuilder;

int registerNativeMethod(JNIEnv* env);
jobjectArray renderWithDiagnostics(JNIEnv* env, jobject j_engine,
                                   jobject j_render, jstring j_str, jfloat left,
                                   jfloat top, jfloat width, jfloat height,
                                   jstring j_color);
jobjectArray renderAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds);
jobjectArray parseStreamingWithDiagnostics(JNIEnv* env, jobject j_engine,
                                           jobjectArray j_chunks);
jobject hitTest(JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str,
                jfloat left, jfloat top, jfloat width, jfloat height, jfloat x,
                jfloat y);
jlong createSession(JNIEnv* env, jobject j_engine, jstring j_str);
void destroySession(JNIEnv* env, jobject j_engine, jlong handle);
jobjectArray renderSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds);
jobject hitTestSession(JNIEnv* env, jobject j_engine, jobject j_render,
                       jlong handle, jfloat left, jfloat top, jfloat width,
                       jfloat height, jfloat x, jfloat y);
jlong createStreamingSession(JNIEnv* env, jobject j_engine);
void destroyStreamingSession(JNIEnv* env, jobject j_engine, jlong handle);
jobjectArray appendStreamingSession(JNIEnv* env, jobject j_engine,
                                    jlong handle, jstring j_chunk);
jobjectArray finishStreamingSession(JNIEnv* env, jobject j_engine,
                                    jlong handle);
jobjectArray renderStreamingSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds);
jint getStreamingSessionLayerCount(JNIEnv* env, jobject j_engine,
                                   jlong handle);
jboolean isStreamingSessionLayerAnimated(JNIEnv* env, jobject j_engine,
                                         jlong handle, jint index);
void beginStreamingSessionFrame(JNIEnv* env, jobject j_engine, jlong handle,
                                jdouble seconds);
void endStreamingSessionFrame(JNIEnv* env, jobject j_engine, jlong handle);
jobjectArray renderStreamingSessionLayerAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jint index,
    jfloat left, jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds);
jobjectArray renderStreamingSessionLayerWithCurrentFrameDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jint index,
    jfloat left, jfloat top, jfloat width, jfloat height, jstring j_color);
jobject hitTestStreamingSession(JNIEnv* env, jobject j_engine, jobject j_render,
                                jlong handle, jfloat left, jfloat top,
                                jfloat width, jfloat height, jfloat x,
                                jfloat y);
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
jobject CreateJavaHitTestResult(
    JNIEnv* env, const serval::svg::parser::SrSVGHitTestResult& hit_result);

namespace {

struct NativeSvgSession {
  std::unique_ptr<SrSVGDOM> dom;
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
};

struct NativeStreamingSvgSession {
  mutable std::mutex mutex;
  std::unique_ptr<SrSVGDOMIncrementalBuilder> builder{
      std::make_unique<SrSVGDOMIncrementalBuilder>()};
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
  bool append_failed{false};
  bool finished{false};
};

NativeSvgSession* AsSvgSession(jlong handle) {
  return reinterpret_cast<NativeSvgSession*>(handle);
}

NativeStreamingSvgSession* AsStreamingSession(jlong handle) {
  return reinterpret_cast<NativeStreamingSvgSession*>(handle);
}

SrSVGDOM* EnsureStreamingPreviewDom(NativeStreamingSvgSession* session) {
  if (!session || !session->builder) {
    return nullptr;
  }
  return session->finished ? session->builder->Final()
                           : session->builder->Preview();
}

const std::vector<serval::svg::parser::SrSVGDiagnostic>&
RenderableStreamingDiagnostics(NativeStreamingSvgSession* session) {
  if (!session) {
    static const std::vector<serval::svg::parser::SrSVGDiagnostic> empty;
    return empty;
  }
  if (session->builder) {
    return session->builder->diagnostics();
  }
  return session->diagnostics;
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
          .name = "parseStreamingWithDiagnostics",
          .signature =
              "([Ljava/lang/String;)[Lcom/lynx/serval/svg/SVGRender$"
              "SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(parseStreamingWithDiagnostics),
      },
      {
          .name = "hitTest",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;Ljava/lang/"
                       "String;FFFFFF)Lcom/lynx/serval/svg/SVGRender$"
                       "SVGHitTestResult;",
          .fnPtr = reinterpret_cast<void*>(hitTest),
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
          .name = "renderSessionAtTimeWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFLjava/lang/"
                       "String;D)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr =
              reinterpret_cast<void*>(renderSessionAtTimeWithDiagnostics),
      },
      {
          .name = "hitTestSession",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFFF)Lcom/lynx/"
                       "serval/svg/SVGRender$SVGHitTestResult;",
          .fnPtr = reinterpret_cast<void*>(hitTestSession),
      },
      {
          .name = "createStreamingSession",
          .signature = "()J",
          .fnPtr = reinterpret_cast<void*>(createStreamingSession),
      },
      {
          .name = "destroyStreamingSession",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(destroyStreamingSession),
      },
      {
          .name = "appendStreamingSession",
          .signature =
              "(JLjava/lang/String;)[Lcom/lynx/serval/svg/SVGRender$"
              "SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(appendStreamingSession),
      },
      {
          .name = "finishStreamingSession",
          .signature =
              "(J)[Lcom/lynx/serval/svg/SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(finishStreamingSession),
      },
      {
          .name = "renderStreamingSessionAtTimeWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFLjava/lang/"
                       "String;D)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(
              renderStreamingSessionAtTimeWithDiagnostics),
      },
      {
          .name = "getStreamingSessionLayerCount",
          .signature = "(J)I",
          .fnPtr = reinterpret_cast<void*>(getStreamingSessionLayerCount),
      },
      {
          .name = "isStreamingSessionLayerAnimated",
          .signature = "(JI)Z",
          .fnPtr = reinterpret_cast<void*>(isStreamingSessionLayerAnimated),
      },
      {
          .name = "beginStreamingSessionFrame",
          .signature = "(JD)V",
          .fnPtr = reinterpret_cast<void*>(beginStreamingSessionFrame),
      },
      {
          .name = "endStreamingSessionFrame",
          .signature = "(J)V",
          .fnPtr = reinterpret_cast<void*>(endStreamingSessionFrame),
      },
      {
          .name = "renderStreamingSessionLayerAtTimeWithDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JIFFFFLjava/lang/"
                       "String;D)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(
              renderStreamingSessionLayerAtTimeWithDiagnostics),
      },
      {
          .name = "renderStreamingSessionLayerWithCurrentFrameDiagnostics",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JIFFFFLjava/lang/"
                       "String;)[Lcom/lynx/serval/svg/"
                       "SVGRender$SVGDiagnostic;",
          .fnPtr = reinterpret_cast<void*>(
              renderStreamingSessionLayerWithCurrentFrameDiagnostics),
      },
      {
          .name = "hitTestStreamingSession",
          .signature = "(Lcom/lynx/serval/svg/SVGRender;JFFFFFF)Lcom/lynx/"
                       "serval/svg/SVGRender$SVGHitTestResult;",
          .fnPtr = reinterpret_cast<void*>(hitTestStreamingSession),
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

jobjectArray renderAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
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

jobjectArray parseStreamingWithDiagnostics(JNIEnv* env, jobject j_engine,
                                           jobjectArray j_chunks) {
  std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
  if (!j_chunks) {
    return CreateJavaDiagnosticArray(env, diagnostics);
  }
  SrSVGDOMStreamBuilder builder;
  const jsize chunk_count = env->GetArrayLength(j_chunks);
  bool append_ok = true;
  for (jsize i = 0; i < chunk_count; ++i) {
    auto j_chunk =
        static_cast<jstring>(env->GetObjectArrayElement(j_chunks, i));
    if (!j_chunk) {
      append_ok = false;
      continue;
    }
    const char* chunk = env->GetStringUTFChars(j_chunk, JNI_FALSE);
    const jsize chunk_length = env->GetStringUTFLength(j_chunk);
    if (chunk) {
      append_ok = builder.Append(chunk, static_cast<size_t>(chunk_length)) &&
                  append_ok;
      env->ReleaseStringUTFChars(j_chunk, chunk);
    } else {
      append_ok = false;
    }
    env->DeleteLocalRef(j_chunk);
  }
  auto svg_dom = builder.Finish();
  diagnostics = builder.diagnostics();
  (void)j_engine;
  (void)append_ok;
  (void)svg_dom;
  return CreateJavaDiagnosticArray(env, diagnostics);
}

jobject hitTest(JNIEnv* env, jobject j_engine, jobject j_render, jstring j_str,
                jfloat left, jfloat top, jfloat width, jfloat height, jfloat x,
                jfloat y) {
  std::vector<serval::svg::parser::SrSVGDiagnostic> build_diagnostics;
  auto svg_dom = CreateSVGDom(env, j_str, &build_diagnostics);
  if (!svg_dom) {
    return CreateJavaHitTestResult(env, {});
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  auto result =
      svg_dom->HitTest(sr_android_canvas.PathFactory(), view_port, x, y);
  return CreateJavaHitTestResult(env, result);
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

jobjectArray renderSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds) {
  auto* session = AsSvgSession(handle);
  if (!session || !session->dom) {
    return CreateJavaDiagnosticArray(env, {});
  }
  ApplyDefaultColor(env, session->dom.get(), j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, session->dom.get(), left, top,
                     width, height, seconds);
  return CreateJavaDiagnosticArray(env, session->dom->diagnostics());
}

jobject hitTestSession(JNIEnv* env, jobject j_engine, jobject j_render,
                       jlong handle, jfloat left, jfloat top, jfloat width,
                       jfloat height, jfloat x, jfloat y) {
  auto* session = AsSvgSession(handle);
  if (!session || !session->dom) {
    return CreateJavaHitTestResult(env, {});
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  auto result =
      session->dom->HitTest(sr_android_canvas.PathFactory(), view_port, x, y);
  return CreateJavaHitTestResult(env, result);
}

jlong createStreamingSession(JNIEnv* env, jobject j_engine) {
  (void)env;
  (void)j_engine;
  return reinterpret_cast<jlong>(new NativeStreamingSvgSession());
}

void destroyStreamingSession(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  if (auto* session = AsStreamingSession(handle)) {
    delete session;
  }
}

jobjectArray appendStreamingSession(JNIEnv* env, jobject j_engine,
                                    jlong handle, jstring j_chunk) {
  auto* session = AsStreamingSession(handle);
  if (!session || !j_chunk) {
    return CreateJavaDiagnosticArray(env, {});
  }
  const char* chunk = env->GetStringUTFChars(j_chunk, JNI_FALSE);
  const jsize chunk_length = env->GetStringUTFLength(j_chunk);
  std::lock_guard<std::mutex> lock(session->mutex);
  if (session->finished) {
    if (chunk) {
      env->ReleaseStringUTFChars(j_chunk, chunk);
    }
    return CreateJavaDiagnosticArray(env, session->diagnostics);
  }
  if (chunk) {
    if (session->builder) {
      session->append_failed =
          !session->builder->Append(chunk, static_cast<size_t>(chunk_length)) ||
          session->append_failed;
      session->diagnostics = session->builder->diagnostics();
    }
    env->ReleaseStringUTFChars(j_chunk, chunk);
  } else {
    session->append_failed = true;
  }
  (void)j_engine;
  return CreateJavaDiagnosticArray(env, session->diagnostics);
}

jobjectArray finishStreamingSession(JNIEnv* env, jobject j_engine,
                                    jlong handle) {
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return CreateJavaDiagnosticArray(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  if (!session->finished) {
    session->finished = true;
    if (session->builder && !session->append_failed) {
      session->builder->Finish();
      session->diagnostics = session->builder->diagnostics();
    }
  }
  (void)j_engine;
  return CreateJavaDiagnosticArray(env, session->diagnostics);
}

jobjectArray renderStreamingSessionAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jfloat left,
    jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds) {
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return CreateJavaDiagnosticArray(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom) {
    return CreateJavaDiagnosticArray(env,
                                     RenderableStreamingDiagnostics(session));
  }
  ApplyDefaultColor(env, svg_dom, j_color);
  RenderSvgDomAtTime(env, j_engine, j_render, svg_dom, left, top, width,
                     height, seconds);
  return CreateJavaDiagnosticArray(env, svg_dom->diagnostics());
}

jint getStreamingSessionLayerCount(JNIEnv* env, jobject j_engine,
                                   jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  return svg_dom ? static_cast<jint>(svg_dom->LayerCount()) : 0;
}

jboolean isStreamingSessionLayerAnimated(JNIEnv* env, jobject j_engine,
                                         jlong handle, jint index) {
  (void)env;
  (void)j_engine;
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return JNI_FALSE;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom || index < 0) {
    return JNI_FALSE;
  }
  return svg_dom->LayerHasAnimations(static_cast<size_t>(index)) ? JNI_TRUE
                                                                 : JNI_FALSE;
}

void beginStreamingSessionFrame(JNIEnv* env, jobject j_engine, jlong handle,
                                jdouble seconds) {
  (void)env;
  (void)j_engine;
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom) {
    return;
  }
  svg_dom->BeginAnimationFrame(seconds);
}

void endStreamingSessionFrame(JNIEnv* env, jobject j_engine, jlong handle) {
  (void)env;
  (void)j_engine;
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return;
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom) {
    return;
  }
  svg_dom->EndAnimationFrame();
}

jobjectArray renderStreamingSessionLayerAtTimeWithDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jint index,
    jfloat left, jfloat top, jfloat width, jfloat height, jstring j_color,
    jdouble seconds) {
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return CreateJavaDiagnosticArray(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom || index < 0) {
    return CreateJavaDiagnosticArray(env, {});
  }
  ApplyDefaultColor(env, svg_dom, j_color);
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  svg_dom->RenderLayerAtTime(&sr_android_canvas, view_port,
                             static_cast<size_t>(index), seconds);
  return CreateJavaDiagnosticArray(env, svg_dom->diagnostics());
}

jobjectArray renderStreamingSessionLayerWithCurrentFrameDiagnostics(
    JNIEnv* env, jobject j_engine, jobject j_render, jlong handle, jint index,
    jfloat left, jfloat top, jfloat width, jfloat height, jstring j_color) {
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return CreateJavaDiagnosticArray(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom || index < 0) {
    return CreateJavaDiagnosticArray(env, {});
  }
  ApplyDefaultColor(env, svg_dom, j_color);
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  svg_dom->RenderLayer(&sr_android_canvas, view_port,
                       static_cast<size_t>(index));
  return CreateJavaDiagnosticArray(env, svg_dom->diagnostics());
}

jobject hitTestStreamingSession(JNIEnv* env, jobject j_engine, jobject j_render,
                                jlong handle, jfloat left, jfloat top,
                                jfloat width, jfloat height, jfloat x,
                                jfloat y) {
  auto* session = AsStreamingSession(handle);
  if (!session) {
    return CreateJavaHitTestResult(env, {});
  }
  std::lock_guard<std::mutex> lock(session->mutex);
  SrSVGDOM* svg_dom = EnsureStreamingPreviewDom(session);
  if (!svg_dom) {
    return CreateJavaHitTestResult(env, {});
  }
  SrAndroidCanvas sr_android_canvas(env, j_engine, j_render);
  SrSVGBox view_port{left, top, width, height};
  auto result =
      svg_dom->HitTest(sr_android_canvas.PathFactory(), view_port, x, y);
  return CreateJavaHitTestResult(env, result);
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

jobject CreateJavaHitTestResult(
    JNIEnv* env, const serval::svg::parser::SrSVGHitTestResult& hit_result) {
  jclass hit_result_class =
      env->FindClass("com/lynx/serval/svg/SVGRender$SVGHitTestResult");
  if (!hit_result_class) {
    return nullptr;
  }
  jmethodID constructor =
      env->GetMethodID(hit_result_class, "<init>",
                       "(ZLjava/lang/String;Ljava/lang/String;)V");
  if (!constructor) {
    return nullptr;
  }
  jstring id = env->NewStringUTF(hit_result.id.c_str());
  jstring action = env->NewStringUTF(hit_result.action.c_str());
  jobject result =
      env->NewObject(hit_result_class, constructor,
                     static_cast<jboolean>(hit_result.hit), id, action);
  env->DeleteLocalRef(id);
  env->DeleteLocalRef(action);
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
