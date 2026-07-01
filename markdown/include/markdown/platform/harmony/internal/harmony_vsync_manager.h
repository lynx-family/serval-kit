// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VSYNC_MANAGER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VSYNC_MANAGER_H_
#include <cstdint>
namespace serval::markdown {
class HarmonyVSyncCallback {
 public:
  virtual ~HarmonyVSyncCallback() = default;
  virtual void OnVSync(int64_t time_stamp) = 0;
};
class HarmonyVSyncManager {
 public:
  static void AddVSyncCallback(HarmonyVSyncCallback* callback);
  static void RemoveVSyncCallback(HarmonyVSyncCallback* callback);

  // Requests a single VSync frame. In the future this can be used by animation
  // logic to pull the next frame only when it knows more frames are needed.
  static void RequestNextFrame();
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_VSYNC_MANAGER_H_
