// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_TASK_RUNNER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_TASK_RUNNER_H_

#include <uv.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace serval::markdown {

// Created and closed on the UI loop; tasks may be posted from any thread.
class HarmonyTaskRunner {
 public:
  static std::shared_ptr<HarmonyTaskRunner> Create(uv_loop_t* loop);
  void PostTask(std::function<void()> task, int64_t delay_microseconds = 0);
  void Close(std::function<void()> on_closed = {});

 private:
  using Clock = std::chrono::steady_clock;
  HarmonyTaskRunner() = default;
  static void OnAsync(uv_async_t* handle);
  static void OnTimer(uv_timer_t* handle);
  static void OnClosed(uv_handle_t* handle);
  void RunPendingTasks();

  uv_async_t async_{};
  uv_timer_t timer_{};
  const std::thread::id thread_id_{std::this_thread::get_id()};
  std::mutex mutex_;
  std::multimap<Clock::time_point, std::function<void()>> tasks_;
  bool closed_{false};
  int open_handles_{0};
  std::function<void()> on_closed_;
  // libuv retains the handles until both asynchronous close callbacks finish.
  std::shared_ptr<HarmonyTaskRunner> keep_alive_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_TASK_RUNNER_H_
