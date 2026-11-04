// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_task_runner.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace serval::markdown {

std::shared_ptr<HarmonyTaskRunner> HarmonyTaskRunner::Create(uv_loop_t* loop) {
  auto runner = std::shared_ptr<HarmonyTaskRunner>(new HarmonyTaskRunner());
  if (uv_async_init(loop, &runner->async_, OnAsync) != 0) {
    return nullptr;
  }
  runner->async_.data = runner.get();
  runner->open_handles_ = 1;
  runner->keep_alive_ = runner;
  if (uv_timer_init(loop, &runner->timer_) != 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(&runner->async_), OnClosed);
    return nullptr;
  }
  runner->timer_.data = runner.get();
  runner->open_handles_ = 2;
  return runner;
}

void HarmonyTaskRunner::PostTask(std::function<void()> task,
                                 int64_t delay_microseconds) {
  const bool run_now =
      delay_microseconds <= 0 && std::this_thread::get_id() == thread_id_;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
      return;
    }
    if (!run_now) {
      tasks_.emplace(Clock::now() + std::chrono::microseconds(std::max<int64_t>(
                                        delay_microseconds, 0)),
                     std::move(task));
      // Sending under the lock prevents racing uv_close during env cleanup.
      [[maybe_unused]] int status = uv_async_send(&async_);
      assert(status == 0);
    }
  }
  if (run_now) {
    task();
  }
}

void HarmonyTaskRunner::RunPendingTasks() {
  const auto now = Clock::now();
  for (;;) {
    std::function<void()> task;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_) {
        return;
      }
      if (tasks_.empty()) {
        uv_timer_stop(&timer_);
        return;
      }
      auto next = tasks_.begin();
      if (next->first > now) {
        const auto delay =
            std::max(next->first - Clock::now(), Clock::duration::zero());
        const auto milliseconds =
            std::chrono::ceil<std::chrono::milliseconds>(delay).count();
        uv_update_time(async_.loop);
        [[maybe_unused]] int status = uv_timer_start(
            &timer_, OnTimer, static_cast<uint64_t>(milliseconds), 0);
        assert(status == 0);
        return;
      }
      task = std::move(next->second);
      tasks_.erase(next);
    }
    task();
  }
}

void HarmonyTaskRunner::Close(std::function<void()> on_closed) {
  assert(std::this_thread::get_id() == thread_id_);
  on_closed_ = std::move(on_closed);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    tasks_.clear();
  }
  uv_timer_stop(&timer_);
  uv_close(reinterpret_cast<uv_handle_t*>(&async_), OnClosed);
  uv_close(reinterpret_cast<uv_handle_t*>(&timer_), OnClosed);
}

void HarmonyTaskRunner::OnAsync(uv_async_t* handle) {
  static_cast<HarmonyTaskRunner*>(handle->data)->RunPendingTasks();
}

void HarmonyTaskRunner::OnTimer(uv_timer_t* handle) {
  static_cast<HarmonyTaskRunner*>(handle->data)->RunPendingTasks();
}

void HarmonyTaskRunner::OnClosed(uv_handle_t* handle) {
  auto* runner = static_cast<HarmonyTaskRunner*>(handle->data);
  if (--runner->open_handles_ == 0) {
    auto on_closed = std::move(runner->on_closed_);
    runner->keep_alive_.reset();
    if (on_closed) {
      on_closed();
    }
  }
}

}  // namespace serval::markdown
