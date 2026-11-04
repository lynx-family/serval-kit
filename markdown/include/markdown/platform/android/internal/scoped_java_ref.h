// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SCOPED_JAVA_REF_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SCOPED_JAVA_REF_H_

#include <jni.h>

#include <string>
#include <utility>

#include "markdown/platform/android/internal/jni_utils.h"

namespace serval::markdown::android {

enum class JavaRefType { kLocal, kGlobal, kWeakGlobal };

template <typename T, JavaRefType Type>
class ScopedJavaRef {
 public:
  ScopedJavaRef() = default;
  ScopedJavaRef(JNIEnv* env, T obj) {
    if constexpr (Type == JavaRefType::kLocal) {
      // Local references returned by JNI are adopted, not duplicated.
      env_ = env;
      obj_ = obj;
    } else {
      Reset(env, obj);
    }
  }
  ScopedJavaRef(const ScopedJavaRef&) = delete;
  ScopedJavaRef& operator=(const ScopedJavaRef&) = delete;
  ScopedJavaRef(ScopedJavaRef&& other) noexcept
      : env_(other.env_), obj_(std::exchange(other.obj_, nullptr)) {}
  ScopedJavaRef& operator=(ScopedJavaRef&& other) noexcept {
    Reset();
    env_ = other.env_;
    obj_ = std::exchange(other.obj_, nullptr);
    return *this;
  }
  ~ScopedJavaRef() { Reset(); }

  T Get() const { return obj_; }

  void Reset(JNIEnv* env, T obj) {
    T ref = nullptr;
    if (obj != nullptr) {
      if constexpr (Type == JavaRefType::kLocal) {
        ref = static_cast<T>(env->NewLocalRef(obj));
      } else if constexpr (Type == JavaRefType::kGlobal) {
        ref = static_cast<T>(env->NewGlobalRef(obj));
      } else {
        ref = static_cast<T>(env->NewWeakGlobalRef(obj));
      }
    }
    DeleteRef(env);
    env_ = env;
    obj_ = ref;
  }

  void Reset() {
    if (obj_ == nullptr) {
      return;
    }
    auto* env = Type == JavaRefType::kLocal ? env_ : GetJNIEnv();
    // Attaching a native thread can fail while the VM is shutting down.
    if (env != nullptr) {
      DeleteRef(env);
    }
  }

 private:
  void DeleteRef(JNIEnv* env) {
    if (obj_ == nullptr) {
      return;
    }
    if constexpr (Type == JavaRefType::kLocal) {
      env->DeleteLocalRef(obj_);
    } else if constexpr (Type == JavaRefType::kGlobal) {
      env->DeleteGlobalRef(obj_);
    } else {
      env->DeleteWeakGlobalRef(obj_);
    }
    obj_ = nullptr;
  }

  JNIEnv* env_{nullptr};
  T obj_{nullptr};
};

template <typename T>
using ScopedLocalJavaRef = ScopedJavaRef<T, JavaRefType::kLocal>;
template <typename T>
using ScopedGlobalJavaRef = ScopedJavaRef<T, JavaRefType::kGlobal>;
template <typename T>
using ScopedWeakGlobalJavaRef = ScopedJavaRef<T, JavaRefType::kWeakGlobal>;

inline ScopedLocalJavaRef<jstring> NewJavaString(JNIEnv* env,
                                                 const char* value) {
  return {env, env->NewStringUTF(value)};
}

inline ScopedLocalJavaRef<jstring> NewJavaString(JNIEnv* env,
                                                 const std::string& value) {
  return NewJavaString(env, value.c_str());
}

}  // namespace serval::markdown::android

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SCOPED_JAVA_REF_H_
