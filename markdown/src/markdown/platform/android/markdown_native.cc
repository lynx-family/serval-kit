// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/include/platform/android/jni_convert_helper.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/platform/android/android_serval_markdown_view.h"
#include "markdown/platform/android/markdown_buffer_reader.h"
#include "markdown/platform/android/markdown_java_canvas_helper.h"
#include "markdown/utils/markdown_screen_metrics.h"
#include "markdown/view/markdown_gesture.h"
using serval::markdown::MarkdownDrawable;
using serval::markdown::MeasureSpec;
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

jobjectArray CreateJavaStringArray(JNIEnv* env,
                                   const std::vector<std::string>& values) {
  auto* string_class = MarkdownClassCache::GetInstance().GetStringClass();
  bool need_release_local_ref = false;
  if (string_class == nullptr) {
    string_class = env->FindClass("java/lang/String");
    need_release_local_ref = true;
  }
  if (string_class == nullptr) {
    return nullptr;
  }
  auto result = env->NewObjectArray(static_cast<jsize>(values.size()),
                                    string_class, nullptr);
  for (size_t i = 0; i < values.size(); i++) {
    auto* j_value = env->NewStringUTF(values[i].c_str());
    env->SetObjectArrayElement(result, static_cast<jsize>(i), j_value);
    env->DeleteLocalRef(j_value);
  }
  if (need_release_local_ref) {
    env->DeleteLocalRef(string_class);
  }
  return result;
}

enum class MarkdownIndexType : jint {
  kChar = 0,
  kSource = 1,
};

enum class MarkdownCharRangeType : jint {
  kChar = 0,
  kWord = 1,
  kSentence = 2,
  kParagraph = 3,
};

bool IsSourceIndexType(jint index_type) {
  return index_type == static_cast<jint>(MarkdownIndexType::kSource);
}

serval::markdown::MarkdownSelection::CharRangeType ConvertCharRangeType(
    jint range_type) {
  switch (static_cast<MarkdownCharRangeType>(range_type)) {
    case MarkdownCharRangeType::kWord:
      return serval::markdown::MarkdownSelection::CharRangeType::kWord;
    case MarkdownCharRangeType::kSentence:
      return serval::markdown::MarkdownSelection::CharRangeType::kSentence;
    case MarkdownCharRangeType::kParagraph:
      return serval::markdown::MarkdownSelection::CharRangeType::kParagraph;
    case MarkdownCharRangeType::kChar:
    default:
      return serval::markdown::MarkdownSelection::CharRangeType::kChar;
  }
}

serval::markdown::Range ConvertToCharRange(
    serval::markdown::MarkdownView* markdown_view, jint start, jint end,
    jint index_type) {
  int32_t char_start =
      std::min(static_cast<int32_t>(start), static_cast<int32_t>(end));
  int32_t char_end =
      std::max(static_cast<int32_t>(start), static_cast<int32_t>(end));
  char_start = std::max(0, char_start);
  char_end = std::max(0, char_end);
  if (IsSourceIndexType(index_type)) {
    char_start = markdown_view->SourceOffsetToCharOffset(char_start);
    char_end = markdown_view->SourceOffsetToCharOffset(char_end);
  }
  return {char_start, char_end};
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeCreateInstance(JNIEnv* env,
                                                               jobject thiz) {
  auto view_instance = new AndroidServalMarkdownView(env, thiz);
  return reinterpret_cast<jlong>(view_instance);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeDestroyInstance(
    JNIEnv* env, jobject thiz, jlong instance) {
  auto* view = ConvertView(instance);
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
extern "C" JNIEXPORT jstring JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetDocumentContent(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return env->NewStringUTF("");
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewStringUTF("");
  }
  auto content = markdown_view->GetContent();
  return env->NewStringUTF(content.c_str());
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetContentID(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong instance) {
  if (instance == 0) {
    return env->NewStringUTF("");
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewStringUTF("");
  }
  auto content_id = markdown_view->GetContentID();
  return env->NewStringUTF(content_id.c_str());
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetContent(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong instance,
                                                           jint start, jint end,
                                                           jint index_type) {
  if (instance == 0) {
    return env->NewStringUTF("");
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewStringUTF("");
  }
  auto char_range = ConvertToCharRange(markdown_view, start, end, index_type);
  auto content = markdown_view->GetParsedContent(char_range);
  return env->NewStringUTF(content.c_str());
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSelectedText(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return env->NewStringUTF("");
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewStringUTF("");
  }
  auto content = markdown_view->GetSelectedText();
  return env->NewStringUTF(content.c_str());
}
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetAllImageUrl(JNIEnv* env,
                                                               jobject thiz,
                                                               jlong instance) {
  auto result = CreateJavaStringArray(env, {});
  if (instance == 0) {
    return result;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return result;
  }
  return CreateJavaStringArray(env, markdown_view->GetAllImageUrl());
}
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetLinkUrl(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong instance) {
  auto result = CreateJavaStringArray(env, {});
  if (instance == 0) {
    return result;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return result;
  }
  return CreateJavaStringArray(env, markdown_view->GetLinkUrl());
}
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetLinkContent(JNIEnv* env,
                                                               jobject thiz,
                                                               jlong instance) {
  auto result = CreateJavaStringArray(env, {});
  if (instance == 0) {
    return result;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return result;
  }
  return CreateJavaStringArray(env, markdown_view->GetLinkContent());
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetLinkBoundingRect(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return env->NewFloatArray(0);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewFloatArray(0);
  }
  auto rects = markdown_view->GetLinkBoundingRect();
  const auto float_count = static_cast<jsize>(rects.size() * 4);
  auto result = env->NewFloatArray(float_count);
  if (float_count == 0) {
    return result;
  }
  std::vector<jfloat> flatten_rects;
  flatten_rects.reserve(float_count);
  for (const auto& rect : rects) {
    flatten_rects.emplace_back(rect.GetLeft());
    flatten_rects.emplace_back(rect.GetTop());
    flatten_rects.emplace_back(rect.GetRight());
    flatten_rects.emplace_back(rect.GetBottom());
  }
  env->SetFloatArrayRegion(result, 0, float_count, flatten_rects.data());
  return result;
}
extern "C" JNIEXPORT jlongArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSyntaxSourceRanges(
    JNIEnv* env, jobject thiz, jlong instance, jstring tag) {
  auto result = env->NewLongArray(0);
  if (instance == 0 || tag == nullptr) {
    return result;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return result;
  }
  const auto length = env->GetStringUTFLength(tag);
  const auto* chars = env->GetStringUTFChars(tag, nullptr);
  std::string tag_value(chars, static_cast<size_t>(length));
  env->ReleaseStringUTFChars(tag, chars);

  auto ranges = markdown_view->GetSyntaxSourceRanges(tag_value);
  result = env->NewLongArray(static_cast<jsize>(ranges.size()));
  if (ranges.empty()) {
    return result;
  }
  std::vector<jlong> packed_ranges;
  packed_ranges.reserve(ranges.size());
  for (const auto& range : ranges) {
    packed_ranges.emplace_back(
        MarkdownJNIUtils::PackIntPair(range.start_, range.end_));
  }
  env->SetLongArrayRegion(result, 0, static_cast<jsize>(packed_ranges.size()),
                          packed_ranges.data());
  return result;
}
extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSelectedRange(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  auto range = markdown_view->GetSelectedRange();
  return MarkdownJNIUtils::PackIntPair(range.start_, range.end_);
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSelectedLineBoundingRect(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return env->NewFloatArray(0);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewFloatArray(0);
  }
  const auto& rects = markdown_view->GetSelectedLineBoundingRect();
  const auto float_count = static_cast<jsize>(rects.size() * 4);
  auto result = env->NewFloatArray(float_count);
  if (float_count == 0) {
    return result;
  }
  std::vector<jfloat> flatten_rects;
  flatten_rects.reserve(float_count);
  for (const auto& rect : rects) {
    flatten_rects.emplace_back(rect.GetLeft());
    flatten_rects.emplace_back(rect.GetTop());
    flatten_rects.emplace_back(rect.GetRight());
    flatten_rects.emplace_back(rect.GetBottom());
  }
  env->SetFloatArrayRegion(result, 0, float_count, flatten_rects.data());
  return result;
}
extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSelectionHandlePosition(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  const auto position = markdown_view->GetSelectionHandlePosition();
  return MarkdownJNIUtils::PackIntPair(static_cast<int32_t>(position.x_),
                                       static_cast<int32_t>(position.y_));
}
extern "C" JNIEXPORT jfloat JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetSelectionHandleRadius(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return 0;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return 0;
  }
  return markdown_view->GetSelectionHandleRadius();
}
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetTextBoundingRect(
    JNIEnv* env, jobject thiz, jlong instance, jint start, jint end,
    jint index_type) {
  if (instance == 0) {
    return env->NewFloatArray(0);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return env->NewFloatArray(0);
  }
  auto char_range = ConvertToCharRange(markdown_view, start, end, index_type);
  auto rects = markdown_view->GetTextLineBoundingRect(char_range);
  const auto float_count = static_cast<jsize>(rects.size() * 4);
  auto result = env->NewFloatArray(float_count);
  if (float_count == 0) {
    return result;
  }
  std::vector<jfloat> flatten_rects;
  flatten_rects.reserve(float_count);
  for (const auto& rect : rects) {
    flatten_rects.emplace_back(rect.GetLeft());
    flatten_rects.emplace_back(rect.GetTop());
    flatten_rects.emplace_back(rect.GetRight());
    flatten_rects.emplace_back(rect.GetBottom());
  }
  env->SetFloatArrayRegion(result, 0, float_count, flatten_rects.data());
  return result;
}
extern "C" JNIEXPORT jint JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetCharIndexByPoint(
    JNIEnv* env, jobject thiz, jlong instance, jfloat x, jfloat y,
    jint index_type) {
  if (instance == 0) {
    return -1;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return -1;
  }
  int32_t char_index = markdown_view->GetCharIndexByPosition({x, y});
  if (IsSourceIndexType(index_type) && char_index >= 0) {
    return markdown_view->CharOffsetToSourceOffset(char_index);
  }
  return char_index;
}
extern "C" JNIEXPORT jlong JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetCharRangeByPoint(
    JNIEnv* env, jobject thiz, jlong instance, jfloat x, jfloat y,
    jint index_type, jint range_type) {
  if (instance == 0) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return MarkdownJNIUtils::PackIntPair(-1, -1);
  }
  auto char_range_type = ConvertCharRangeType(range_type);
  auto char_range =
      markdown_view->GetCharRangeByPosition({x, y}, char_range_type);
  if (IsSourceIndexType(index_type)) {
    if (char_range.start_ >= 0) {
      char_range.start_ = markdown_view->CharOffsetToSourceOffset(
          static_cast<int32_t>(char_range.start_));
    }
    if (char_range.end_ >= 0) {
      char_range.end_ = markdown_view->CharOffsetToSourceOffset(
          static_cast<int32_t>(char_range.end_));
    }
  }
  return MarkdownJNIUtils::PackIntPair(char_range.start_, char_range.end_);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetTextSelection(
    JNIEnv* env, jobject thiz, jlong instance, jint start, jint end) {
  if (instance == 0) {
    return;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return;
  }
  markdown_view->SetTextSelection({start, end});
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_Markdown_initialClassCache(JNIEnv* env, jclass clazz) {
  MarkdownClassCache::GetInstance().Initial(env);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetDensity(JNIEnv* env,
                                                           jobject thiz,
                                                           jfloat density) {
  serval::markdown::MarkdownScreenMetrics::SetDensity(density);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetStyle(JNIEnv* env,
                                                         jobject thiz,
                                                         jlong instance,
                                                         jbyteArray buffer) {
  if (buffer == nullptr || instance == 0)
    return;
  auto view = ConvertView(instance);
  auto length = env->GetArrayLength(buffer);
  auto array = env->GetByteArrayElements(buffer, nullptr);
  BufferInputStream stream(reinterpret_cast<uint8_t*>(array), length);
  MarkdownBufferReader reader(stream);
  auto result = reader.ReadValue();
  env->ReleaseByteArrayElements(buffer, array, 0);
  if (result->GetType() != serval::markdown::ValueType::kMap)
    return;
  view->GetMarkdownView()->SetStyle(result->AsMap());
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeOnVSync(JNIEnv* env,
                                                        jobject thiz,
                                                        jlong instance,
                                                        jlong time) {
  if (instance == 0)
    return;
  auto view = ConvertView(instance);
  view->OnVSync(time);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeGetAnimationStep(
    JNIEnv* env, jobject thiz, jlong instance) {
  if (instance == 0) {
    return 0;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  if (markdown_view == nullptr) {
    return 0;
  }
  return markdown_view->GetAnimationStep();
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetAnimationStep(
    JNIEnv* env, jobject thiz, jlong instance, jint animation_step) {
  if (instance == 0) {
    return;
  }
  auto* view = ConvertView(instance);
  view->GetMarkdownView()->SetAnimationStep(animation_step);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetExposureListenerEnabled(
    JNIEnv* env, jobject thiz, jlong instance, jboolean enabled) {
  if (instance == 0) {
    return;
  }
  auto* view = ConvertView(instance);
  view->SetExposureListenerEnabled(enabled == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetNumberProp(
    JNIEnv* env, jobject thiz, jlong instance, jint key, jdouble value) {
  if (instance == 0)
    return;
  auto* view = ConvertView(instance);
  view->GetMarkdownView()->SetNumberProp(
      static_cast<serval::markdown::MarkdownProps>(key), value);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetStringProp(
    JNIEnv* env, jobject thiz, jlong instance, jint key, jstring value) {
  if (instance == 0) {
    return;
  }
  auto* view = ConvertView(instance);
  if (value == nullptr) {
    view->GetMarkdownView()->SetStringProp(
        static_cast<serval::markdown::MarkdownProps>(key), "");
    return;
  }
  const auto length = env->GetStringUTFLength(value);
  const auto* chars = env->GetStringUTFChars(value, nullptr);
  view->GetMarkdownView()->SetStringProp(
      static_cast<serval::markdown::MarkdownProps>(key),
      {chars, static_cast<size_t>(length)});
  env->ReleaseStringUTFChars(value, chars);
}
extern "C" JNIEXPORT void JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeSetValueProp(
    JNIEnv* env, jobject thiz, jlong instance, jint key, jbyteArray config) {
  if (instance == 0 || config == nullptr) {
    return;
  }
  auto* view = ConvertView(instance);
  const auto length = env->GetArrayLength(config);
  if (length <= 0) {
    return;
  }
  auto* array = env->GetByteArrayElements(config, nullptr);
  BufferInputStream stream(reinterpret_cast<uint8_t*>(array), length);
  MarkdownBufferReader reader(stream);
  auto result = reader.ReadValue();
  env->ReleaseByteArrayElements(config, array, JNI_ABORT);
  if (result == nullptr) {
    return;
  }
  const auto prop = static_cast<serval::markdown::MarkdownProps>(key);
  if (result->GetType() == serval::markdown::ValueType::kArray) {
    view->GetMarkdownView()->SetArrayProp(prop, result->AsArray());
  } else if (result->GetType() == serval::markdown::ValueType::kMap) {
    view->GetMarkdownView()->SetMapProp(prop, result->AsMap());
  }
}

serval::markdown::GestureEventType ConvertGestureEventType(jint type) {
  switch (type) {
    case 1:
      return serval::markdown::GestureEventType::kDown;
    case 2:
      return serval::markdown::GestureEventType::kMove;
    case 3:
      return serval::markdown::GestureEventType::kUp;
    case 4:
      return serval::markdown::GestureEventType::kCancel;
    default:
      return serval::markdown::GestureEventType::kUnknown;
  }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeDispatchTap(
    JNIEnv* env, jobject thiz, jlong instance, jfloat x, jfloat y) {
  if (instance == 0) {
    return JNI_FALSE;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  return markdown_view->OnTap({x, y}, serval::markdown::GestureEventType::kDown)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeDispatchLongPress(
    JNIEnv* env, jobject thiz, jlong instance, jfloat x, jfloat y) {
  if (instance == 0) {
    return JNI_FALSE;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  return markdown_view->OnLongPress({x, y},
                                    serval::markdown::GestureEventType::kDown)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_markdown_ServalMarkdownView_nativeDispatchPan(
    JNIEnv* env, jobject thiz, jlong instance, jfloat x, jfloat y,
    jfloat motion_x, jfloat motion_y, jint type) {
  if (instance == 0) {
    return JNI_FALSE;
  }
  auto* view = ConvertView(instance);
  auto* markdown_view = view->GetMarkdownView();
  const auto event = ConvertGestureEventType(type);
  return markdown_view->OnPan({x, y}, {motion_x, motion_y}, event) ? JNI_TRUE
                                                                   : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_markdown_SelectionHandleView_nativeDispatchPanByPlatformView(
    JNIEnv* env, jobject thiz, jlong platform_view, jfloat x, jfloat y,
    jfloat motion_x, jfloat motion_y, jint type) {
  auto* view = reinterpret_cast<AndroidMarkdownView*>(platform_view);
  if (view == nullptr) {
    return JNI_FALSE;
  }
  const auto event = ConvertGestureEventType(type);
  return view->DispatchPan({x, y}, {motion_x, motion_y}, event) ? JNI_TRUE
                                                                : JNI_FALSE;
}
