// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_vsync_manager.h"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "native_vsync/native_vsync.h"
namespace serval::markdown {
class HarmonyVSyncManagerImpl {
 public:
  static HarmonyVSyncManagerImpl& Instance() {
    static HarmonyVSyncManagerImpl impl;
    return impl;
  }
  HarmonyVSyncManagerImpl() {
    const char name[] = "serval_markdown_vsync";
    vsync_ = OH_NativeVSync_Create(name, sizeof(name));
  }
  ~HarmonyVSyncManagerImpl() {
    if (vsync_ != nullptr) {
      OH_NativeVSync_Destroy(vsync_);
    }
  }

  void AddVSyncCallback(HarmonyVSyncCallback* callback) {
    HarmonyUIThread::RunOnUIThread([this, callback]() {
      std::lock_guard<std::mutex> guard(callbacks_mutex_);
      callbacks_.emplace_back(callback);
      if (callbacks_.size() == 1) {
        RequestNextFrame();
      }
    });
  }
  void RemoveVSyncCallback(HarmonyVSyncCallback* callback) {
    std::lock_guard<std::mutex> guard(callbacks_mutex_);
    callbacks_.erase(
        std::remove(callbacks_.begin(), callbacks_.end(), callback));
  }

  void RequestNextFrame() const {
    OH_NativeVSync_RequestFrame(vsync_, HarmonyVSyncManagerImpl::OnVSyncStatic,
                                nullptr);
  }

  static void OnVSyncStatic(long long time_stamp, void* ud) {
    Instance().OnVSync(time_stamp);
  }
  void OnVSync(int64_t time_stamp) {
    HarmonyUIThread::RunOnUIThread([this, time_stamp]() {
      std::lock_guard<std::mutex> guard(callbacks_mutex_);
      for (auto* callback : callbacks_) {
        callback->OnVSync(time_stamp);
      }

      // Keep requesting frames while there are attached callbacks. Exposure and
      // renderer on-screen checks run inside OnVSync and are throttled inside
      // MarkdownView so they don't become a CPU burden. The long-term design can
      // move to a pull model where animation logic explicitly calls
      // HarmonyVSyncManager::RequestNextFrame() only when it needs another frame.
      if (!callbacks_.empty()) {
        RequestNextFrame();
      }
    });
  }

  OH_NativeVSync* vsync_{nullptr};
  std::vector<HarmonyVSyncCallback*> callbacks_;
  std::mutex callbacks_mutex_;
};
void HarmonyVSyncManager::AddVSyncCallback(HarmonyVSyncCallback* callback) {
  HarmonyVSyncManagerImpl::Instance().AddVSyncCallback(callback);
}
void HarmonyVSyncManager::RemoveVSyncCallback(HarmonyVSyncCallback* callback) {
  HarmonyVSyncManagerImpl::Instance().RemoveVSyncCallback(callback);
}
void HarmonyVSyncManager::RequestNextFrame() {
  HarmonyVSyncManagerImpl::Instance().RequestNextFrame();
}
}  // namespace serval::markdown
