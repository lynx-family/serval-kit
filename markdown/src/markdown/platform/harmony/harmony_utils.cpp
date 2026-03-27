// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_utils.h"

#include <utility>
#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {
thread_local napi_env ENV = nullptr;

napi_env HarmonyEnv::GetEnv() {
  return ENV;
}

void HarmonyEnv::SetEnv(napi_env env) {
  ENV = env;
}

void HarmonyUIThread::Init(napi_env env) {
  (void)env;
  MarkdownPlatform::RunOnUIThread([]() {});
}

void HarmonyUIThread::PostTask(std::function<void()> task) {
  MarkdownPlatform::RunOnUIThread(std::move(task));
}

void HarmonyUIThread::PostDelayedTask(std::function<void()> task,
                                      int64_t micro_seconds) {
  MarkdownPlatform::RunOnUIThread(std::move(task), micro_seconds);
}
}  // namespace serval::markdown
