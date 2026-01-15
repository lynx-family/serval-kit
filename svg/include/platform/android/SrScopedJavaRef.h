// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRSCOPEDJAVAREF_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRSCOPEDJAVAREF_H_

#include <jni.h>
#include "utils/SrSVGLog.h"

namespace serval {
namespace svg {
namespace android {

class JavaRef {
 public:
  JavaRef() = default;
  JavaRef(jobject j_obj) : j_obj_(j_obj) {}

  void ReleaseLocalRef(JNIEnv* env);
  void ReleaseGlobalRef(JNIEnv* env);
  void ResetNewLocalRef(JNIEnv* env, jobject j_obj);
  void ResetNewGlobalRef(JNIEnv* env, jobject j_obj);

  virtual bool IsLocal() const { return false; }
  virtual bool IsGlobal() const { return false; }
  bool IsNull() const { return j_obj_ == nullptr; }
  jobject Get() const { return j_obj_; }

 protected:
  jobject j_obj_{nullptr};
};

template <typename T>
class JavaLocalRef final : public JavaRef {
 public:
  JavaLocalRef(JNIEnv* jni_env, T obj) : JavaRef(obj), jni_env_(jni_env) {}

  ~JavaLocalRef() { ReleaseLocalRef(jni_env_); }

  JavaLocalRef(const JavaLocalRef<T>& other) : jni_env_(other.jni_env_) {
    Reset(jni_env_, other.Get());
  }

  JavaLocalRef(JavaLocalRef<T>&& other) {
    j_obj_ = other.j_obj_;
    other.j_obj_ = nullptr;
  }

  void operator=(const JavaLocalRef<T>& other) {
    jni_env_ = other.jni_env_;
    Reset(jni_env_, other.Get());
  }

  void Reset() { ReleaseLocalRef(jni_env_); }

  void Reset(JNIEnv* jni_env, jobject j_obj) {
    ResetNewLocalRef(jni_env, j_obj);
  }

  T Get() const { return (T)j_obj_; }

  bool IsLocal() const override { return true; }

 private:
  JNIEnv* jni_env_{nullptr};
};

template <typename T>
class JavaGlobalRef final : public JavaRef {
 public:
  JavaGlobalRef() {}
  JavaGlobalRef(JNIEnv* env, T obj) { Reset(env, obj); }

  ~JavaGlobalRef() { ReleaseGlobalRef(nullptr); }

  JavaGlobalRef(const JavaGlobalRef<T>& other) { Reset(nullptr, other.Get()); }

  JavaGlobalRef(const JavaLocalRef<T>& other) { Reset(nullptr, other.Get()); }

  JavaGlobalRef(JavaGlobalRef<T>&& other) {
    j_obj_ = other.j_obj_;
    other.j_obj_ = nullptr;
  }

  void operator=(const JavaGlobalRef<T>& other) { Reset(nullptr, other.Get()); }

  void Reset(JNIEnv* env, const JavaLocalRef<T>& other) {
    ResetNewGlobalRef(env, other.Get());
  }

  void Reset(JNIEnv* env, jobject obj) { ResetNewGlobalRef(env, obj); }

  T Get() const { return (T)j_obj_; }

  bool IsGlobal() const override { return true; }
};

}  // namespace android
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRSCOPEDJAVAREF_H_
