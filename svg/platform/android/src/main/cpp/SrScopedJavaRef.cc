// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrScopedJavaRef.h"

#include "platform/android/SrJNIUtils.h"

namespace serval {
namespace svg {
namespace android {

void JavaRef::ReleaseLocalRef(JNIEnv* env) {
  if (!j_obj_) {
    return;
  }
  if (!env) {
    env = GetEnvForCurrentThread();
  }
  if (env) {
    env->DeleteLocalRef(j_obj_);
    j_obj_ = nullptr;
  }
}

void JavaRef::ReleaseGlobalRef(JNIEnv* env) {
  if (!j_obj_) {
    return;
  }
  if (!env) {
    env = GetEnvForCurrentThread();
  }
  if (env) {
    env->DeleteGlobalRef(j_obj_);
    j_obj_ = nullptr;
  }
}

void JavaRef::ResetNewLocalRef(JNIEnv* env, jobject j_obj) {
  if (!env) {
    env = GetEnvForCurrentThread();
  }
  if (!env) {
    return;
  }
  if (j_obj) {
    j_obj = env->NewLocalRef(j_obj);
  }
  if (j_obj_) {
    env->DeleteLocalRef(j_obj_);
  }
  j_obj_ = j_obj;
}

void JavaRef::ResetNewGlobalRef(JNIEnv* env, jobject j_obj) {
  if (!env) {
    env = GetEnvForCurrentThread();
  }
  if (!env) {
    return;
  }
  if (j_obj) {
    j_obj = env->NewGlobalRef(j_obj);
  }
  if (j_obj_) {
    env->DeleteGlobalRef(j_obj_);
  }
  j_obj_ = j_obj;
}

}  // namespace android
}  // namespace svg
}  // namespace serval

