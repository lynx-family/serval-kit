// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/harmony/internal/harmony_utils.h"

#include <memory>
#include <utility>

#include "markdown/platform/harmony/internal/harmony_task_runner.h"

namespace serval::markdown {
namespace {
std::shared_ptr<HarmonyTaskRunner> ui_task_runner;

void CloseUITaskRunner(napi_async_cleanup_hook_handle handle, void* data) {
  auto* runner = static_cast<std::shared_ptr<HarmonyTaskRunner>*>(data);
  std::atomic_store(&ui_task_runner, std::shared_ptr<HarmonyTaskRunner>{});
  (*runner)->Close([runner, handle]() {
    delete runner;
    napi_remove_async_cleanup_hook(handle);
  });
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
  if (std::atomic_load(&ui_task_runner)) {
    return;
  }
  uv_loop_t* loop = nullptr;
  if (napi_get_uv_event_loop(env, &loop) != napi_ok || loop == nullptr) {
    return;
  }
  auto runner = HarmonyTaskRunner::Create(loop);
  if (!runner) {
    return;
  }
  auto* cleanup = new std::shared_ptr<HarmonyTaskRunner>(runner);
  if (napi_add_async_cleanup_hook(env, CloseUITaskRunner, cleanup, nullptr) !=
      napi_ok) {
    delete cleanup;
    runner->Close();
    return;
  }
  std::atomic_store(&ui_task_runner, std::move(runner));
}
void HarmonyUIThread::RunOnUIThread(std::function<void()> task,
                                    int64_t micro_seconds) {
  auto runner = std::atomic_load(&ui_task_runner);
  if (!task || !runner) {
    return;
  }
  runner->PostTask(std::move(task), micro_seconds);
}
}  // namespace serval::markdown
