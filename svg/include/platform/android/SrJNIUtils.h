// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRJNIUTILS_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRJNIUTILS_H_

#include <jni.h>
#include <string>

#include "platform/android/SrScopedJavaRef.h"

namespace serval {
namespace svg {
namespace android {

enum MethodType {
  STATIC_METHOD,
  INSTANCE_METHOD,
};

void InitVM(JavaVM* vm);

JNIEnv* GetEnvForCurrentThread();

JavaLocalRef<jclass> GetClass(JNIEnv* env, const char* class_name);

JavaLocalRef<jclass> GetClass(JNIEnv* env, jobject j_object);

jmethodID GetMethod(JNIEnv* env, jclass clazz, MethodType type,
                    const char* method_name, const char* jni_signature);

jmethodID GetMethod(JNIEnv* env, jclass clazz, MethodType type,
                    const char* method_name, const char* jni_signature,
                    intptr_t* method_id);

bool HasException(JNIEnv* env);

bool ClearException(JNIEnv* env);

}  // namespace android
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRJNIUTILS_H_
