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

namespace {

void AppendModifiedUtf8Surrogate(std::string& text, uint32_t surrogate) {
  text.push_back(static_cast<char>(0xE0 | (surrogate >> 12)));
  text.push_back(static_cast<char>(0x80 | ((surrogate >> 6) & 0x3F)));
  text.push_back(static_cast<char>(0x80 | (surrogate & 0x3F)));
}

jstring NewParagraphString(JNIEnv* env, const std::string& text) {
  // SVG text can mix Modified UTF-8 from GetStringUTFChars with standard
  // UTF-8 from numeric references. Dalvik requires supplementary characters
  // to be encoded as a UTF-16 surrogate pair in Modified UTF-8.
  std::string modified;
  size_t copied = 0;
  for (size_t i = 0; i + 3 < text.size(); ++i) {
    const auto first = static_cast<unsigned char>(text[i]);
    if (first < 0xF0 || first > 0xF4) {
      continue;
    }
    const auto second = static_cast<unsigned char>(text[i + 1]);
    const auto third = static_cast<unsigned char>(text[i + 2]);
    const auto fourth = static_cast<unsigned char>(text[i + 3]);
    if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80 ||
        (fourth & 0xC0) != 0x80) {
      continue;
    }
    const uint32_t code_point = ((first & 7) << 18) | ((second & 0x3F) << 12) |
                                ((third & 0x3F) << 6) | (fourth & 0x3F);
    if (code_point < 0x10000 || code_point > 0x10FFFF) {
      continue;
    }
    modified.append(text, copied, i - copied);
    const uint32_t supplementary = code_point - 0x10000;
    AppendModifiedUtf8Surrogate(modified, 0xD800 | (supplementary >> 10));
    AppendModifiedUtf8Surrogate(modified, 0xDC00 | (supplementary & 0x3FF));
    i += 3;
    copied = i + 1;
  }
  if (copied == 0) {
    return env->NewStringUTF(text.c_str());
  }
  modified.append(text, copied, std::string::npos);
  return env->NewStringUTF(modified.c_str());
}

}  // namespace

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
        jni_env, NewParagraphString(jni_env, text)};
    if (text_string_ref.IsNull()) {
      return;
    }
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
