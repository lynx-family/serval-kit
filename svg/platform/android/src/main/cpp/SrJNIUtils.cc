// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrJNIUtils.h"

namespace serval {
namespace svg {
namespace android {

JavaVM* g_vm = nullptr;

void InitVM(JavaVM* vm) {
  g_vm = vm;
}

JNIEnv* GetEnvForCurrentThread() {
  JNIEnv* env = nullptr;
  if (g_vm) {
    jint ret = (const_cast<JavaVM*>(g_vm))
                   ->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_2);
    if (ret == JNI_EDETACHED || !env) {
      ret = (const_cast<JavaVM*>(g_vm))->AttachCurrentThread(&env, nullptr);
    }
  }
  return env;
}

JavaLocalRef<jclass> GetClass(JNIEnv* env, const char* class_name) {
  jclass clazz = env->FindClass(class_name);
  if (ClearException(env) || !clazz) {
    LOGF("Failed to find class = %s", class_name);
  } else {
    LOGD("Success to find class = %s", class_name);
  }
  return JavaLocalRef<jclass>(env, clazz);
}

JavaLocalRef<jclass> GetClass(JNIEnv* env, jobject j_object) {
  jclass clazz = env->GetObjectClass(j_object);
  if (ClearException(env) || !clazz) {
    LOGF("Failed to find class");
  } else {
    LOGD("Success to find class");
  }
  return JavaLocalRef<jclass>(env, clazz);
}

jmethodID GetMethod(JNIEnv* env, jclass clazz, MethodType type,
                    const char* method_name, const char* jni_signature) {
  jmethodID id = nullptr;
  if (clazz) {
    if (type == STATIC_METHOD) {
      id = env->GetStaticMethodID(clazz, method_name, jni_signature);
    } else if (type == INSTANCE_METHOD) {
      id = env->GetMethodID(clazz, method_name, jni_signature);
    }
    if (ClearException(env) || !id) {
      LOGF("Failed to find method = %s, signature = %s", method_name,
           jni_signature);
    } else {
      LOGD("Success to find method = %s, signature = %s", method_name,
           jni_signature);
    }
  }
  return id;
}

jmethodID GetMethod(JNIEnv* env, jclass clazz, MethodType type,
                    const char* method_name, const char* jni_signature,
                    intptr_t* method_id) {
  if (*method_id) {
    return reinterpret_cast<jmethodID>(*method_id);
  }
  *method_id = reinterpret_cast<intptr_t>(
      GetMethod(env, clazz, type, method_name, jni_signature));
  return reinterpret_cast<jmethodID>(*method_id);
}

bool HasException(JNIEnv* env) {
  return env->ExceptionCheck() != JNI_FALSE;
}

bool ClearException(JNIEnv* env) {
  if (HasException(env)) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
  }
  return false;
}

}  // namespace android
}  // namespace svg
}  // namespace serval

