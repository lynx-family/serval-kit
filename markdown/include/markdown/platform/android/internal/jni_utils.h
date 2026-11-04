// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_JNI_UTILS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_JNI_UTILS_H_

#include <jni.h>

namespace serval::markdown::android {

void InitJavaVM(JavaVM* vm);
JNIEnv* GetJNIEnv();

}  // namespace serval::markdown::android

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_JNI_UTILS_H_
