// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidParagraphFactory.h"
#include <android/log.h>
#include "platform/android/SrAndroidCanvas.h"
#include "platform/android/SrAndroidParagraph.h"

namespace serval {
namespace svg {
namespace canvas {

SrAndroidParagraphFactory::SrAndroidParagraphFactory(const SrCanvas* srCanvas) {
  // create stringBuilder
  srCanvas_ = srCanvas;
  // TODO(@hujing.1)  maybe need remove the variable "j_paragraph_ref_"
  auto* android_canvas = static_cast<const android::SrAndroidCanvas*>(srCanvas);
  auto jni_env = android_canvas->GetJNIEnv();
  auto jni_engine = android_canvas->GetJEngine();

  android::JavaLocalRef<jclass> engine_clazz_ref =
      android::GetClass(jni_env, jni_engine);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_string_builder = GetMethod(
      jni_env, engine_clazz_ref.Get(), android::STATIC_METHOD,
      "makeStringBuilder",
      "()"
      "Landroid/text/SpannableStringBuilder;",
      &(android::SrAndroidCanvas::g_SVGRenderEngine_makeSpanStringBuilder_));

  if (!j_make_string_builder) {
    return;
  }
  j_paragraph_ref_ = android::JavaGlobalRef<jobject>(
      jni_env, jni_env->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                               j_make_string_builder));
}

std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas* srCanvas) {
  auto android_paragraph_factory = new SrAndroidParagraphFactory(srCanvas);
  return std::unique_ptr<ParagraphFactory>(android_paragraph_factory);
}

std::unique_ptr<Paragraph> SrAndroidParagraphFactory::CreateParagraph() {
  auto paragraph = new SrAndroidParagraph(j_paragraph_ref_.Get(),
                                          std::move(paragraph_style_));
  return std::unique_ptr<Paragraph>(paragraph);
}

void SrAndroidParagraphFactory::PushTextStyle(const SrTextStyle& style) {
  style_stack_.push_back(style);
}

void SrAndroidParagraphFactory::PopTextStyle() {
  style_stack_.pop_back();
}

void SrAndroidParagraphFactory::SetParagraphStyle(SrParagraphStyle&& style) {
  paragraph_style_ = std::move(style);
}

void SrAndroidParagraphFactory::AddText(const std::string& text) {
  const SrTextStyle& style = style_stack_.back();
  //build spanString，and append text
  if (srCanvas_) {
    auto* android_canvas =
        static_cast<const android::SrAndroidCanvas*>(srCanvas_);
    auto jni_env = android_canvas->GetJNIEnv();
    auto jni_engine = android_canvas->GetJEngine();
    android::JavaLocalRef<jclass> engine_clazz_ref =
        android::GetClass(jni_env, jni_engine);
    if (engine_clazz_ref.IsNull()) {
      return;
    }
    jmethodID append_span_method = GetMethod(
        jni_env, engine_clazz_ref.Get(), android::STATIC_METHOD, "appendSpan",
        "(Landroid/text/SpannableStringBuilder;Ljava/lang/String;IF)V",
        &(android::SrAndroidCanvas::g_SVGRenderEngine_appendSpan_));

    if (!append_span_method) {
      return;
    }
    android::JavaLocalRef<jstring> text_string_ref = {
        jni_env, jni_env->NewStringUTF(text.c_str())};
    jni_env->CallStaticVoidMethod(engine_clazz_ref.Get(), append_span_method,
                                  j_paragraph_ref_.Get(), text_string_ref.Get(),
                                  style.color, style.font_size);
  }
}

void SrAndroidParagraphFactory::Reset() {
  style_stack_.clear();
  style_stack_.emplace_back((SrTextStyle){NSVG_RGB(0, 0, 0), 14.f});
  //clear stingBuilder
}
}  // namespace canvas
}  // namespace svg
}  // namespace serval
