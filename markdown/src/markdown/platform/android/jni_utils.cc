// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/internal/jni_utils.h"

#include <sys/prctl.h>

#include <cassert>

namespace serval::markdown::android {
namespace {

JavaVM* java_vm = nullptr;

struct ThreadEnvironment {
  JNIEnv* env{nullptr};
  bool attached{false};

  ~ThreadEnvironment() {
    if (attached) {
      java_vm->DetachCurrentThread();
    }
  }
};

}  // namespace

void InitJavaVM(JavaVM* vm) {
  java_vm = vm;
}

JNIEnv* GetJNIEnv() {
  assert(java_vm != nullptr);
  thread_local ThreadEnvironment thread;
  if (thread.env != nullptr) {
    return thread.env;
  }
  jint status =
      java_vm->GetEnv(reinterpret_cast<void**>(&thread.env), JNI_VERSION_1_6);
  if (status == JNI_EDETACHED) {
    char name[16];
    JavaVMAttachArgs args{JNI_VERSION_1_6,
                          prctl(PR_GET_NAME, name) == 0 ? name : nullptr,
                          nullptr};
    status = java_vm->AttachCurrentThread(&thread.env, &args);
    thread.attached = status == JNI_OK;
  }
  return status == JNI_OK ? thread.env : nullptr;
}

}  // namespace serval::markdown::android
