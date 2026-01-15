// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include <utility>
#include "base/include/fml/message_loop.h"
#include "markdown/utils/markdown_ui_thread.h"
namespace lynx::markdown {
thread_local napi_env ENV = nullptr;
napi_env HarmonyEnv::GetEnv() {
  return ENV;
}
void HarmonyEnv::SetEnv(napi_env env) {
  ENV = env;
}

void HarmonyUIThread::Init(napi_env env) {
  static std::once_flag instance_once_flag;
  std::call_once(instance_once_flag, [&env]() {
    uv_loop_s* loop;
    napi_get_uv_event_loop(env, &loop);
    MarkdownUIThread::GetUITaskRunner() =
        lynx::fml::MessageLoop::EnsureInitializedForCurrentThread(loop)
            .GetTaskRunner();
  });
}
void HarmonyUIThread::PostTask(std::function<void()> task) {
  fml::TaskRunner::RunNowOrPostTask(MarkdownUIThread::GetUITaskRunner(),
                                    base::closure(std::move(task)));
}
void HarmonyUIThread::PostDelayedTask(std::function<void()> task,
                                      int64_t micro_seconds) {
  MarkdownUIThread::GetUITaskRunner()->PostDelayedTask(
      base::closure(std::move(task)),
      fml::TimeDelta::FromMicroseconds(micro_seconds));
}
}  // namespace lynx::markdown
