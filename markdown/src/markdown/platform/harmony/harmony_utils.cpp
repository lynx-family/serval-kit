// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_utils.h"

#include <utility>

#include "base/include/fml/message_loop.h"

namespace serval::markdown {
namespace {
fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() {
  static fml::RefPtr<fml::TaskRunner> ui_task_runner;
  return ui_task_runner;
}

bool EnsureUITaskRunner() {
  auto& runner = GetUITaskRunner();
  if (runner != nullptr) {
    return true;
  }
  auto env = HarmonyEnv::GetEnv();
  if (env == nullptr) {
    return false;
  }
  uv_loop_s* loop = nullptr;
  if (napi_get_uv_event_loop(env, &loop) != napi_ok || loop == nullptr) {
    return false;
  }
  runner = lynx::fml::MessageLoop::EnsureInitializedForCurrentThread(loop)
               .GetTaskRunner();
  return runner != nullptr;
}
}  // namespace

thread_local napi_env ENV = nullptr;
napi_env HarmonyEnv::GetEnv() {
  return ENV;
}
void HarmonyEnv::SetEnv(napi_env env) {
  ENV = env;
}

void HarmonyUIThread::Init(napi_env env) {
  (void)env;
  RunOnUIThread([]() {});
}
void HarmonyUIThread::RunOnUIThread(std::function<void()> task,
                                    int64_t micro_seconds) {
  if (!task || !EnsureUITaskRunner()) {
    return;
  }
  auto& runner = GetUITaskRunner();
  if (micro_seconds <= 0) {
    fml::TaskRunner::RunNowOrPostTask(runner,
                                      lynx::base::closure(std::move(task)));
    return;
  }
  runner->PostDelayedTask(lynx::base::closure(std::move(task)),
                          fml::TimeDelta::FromMicroseconds(micro_seconds));
}
}  // namespace serval::markdown
