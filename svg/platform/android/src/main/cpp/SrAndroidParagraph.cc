// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidParagraph.h"
#include "platform/android/SrAndroidCanvas.h"

namespace serval {
namespace svg {
namespace canvas {

SrAndroidParagraph::SrAndroidParagraph(jobject jParagraphRef,
                                       SrParagraphStyle paragraph_style)
    : j_paragraph_(jParagraphRef), paragraph_style_(paragraph_style) {}

void SrAndroidParagraph::Layout(float max_width) {}

void SrAndroidParagraph::Draw(SrCanvas* canvas, float x, float y) {
  auto* android_canvas = static_cast<const android::SrAndroidCanvas*>(canvas);
  auto jni_env = android_canvas->GetJNIEnv();
  auto jni_render = android_canvas->GetJRender();

  android::JavaLocalRef<jclass> j_render_clazz =
      android::GetClass(jni_env, jni_render);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_make_draw_text =
      GetMethod(jni_env, j_render_clazz.Get(), android::INSTANCE_METHOD,
                "drawText", "(Landroid/text/SpannableStringBuilder;IFF)V",
                &(android::SrAndroidCanvas::g_SVGRender_drawText_));

  if (!j_make_draw_text) {
    return;
  }
  jni_env->CallVoidMethod(jni_render, j_make_draw_text, j_paragraph_,
                          paragraph_style_.text_anchor, x, y);
}

}  // namespace canvas
}  // namespace svg
}  // namespace serval
