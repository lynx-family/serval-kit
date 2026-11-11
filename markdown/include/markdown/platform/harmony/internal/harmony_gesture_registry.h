// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_GESTURE_REGISTRY_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_GESTURE_REGISTRY_H_

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

struct ArkUI_GestureRecognizer;

namespace serval::markdown {
class NativeServalMarkdownView;

class HarmonyGestureRegistry {
 public:
  using Recognizers = std::initializer_list<ArkUI_GestureRecognizer*>;

  // All recognizers on a view share one token. ArkUI may deliver an old token
  // after the view and recognizer addresses have both been reused.
  void* Register(std::weak_ptr<NativeServalMarkdownView> view,
                 Recognizers recognizers) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* token = reinterpret_cast<void*>(next_token_++);
    views_.emplace(token, std::move(view));
    for (auto* recognizer : recognizers) {
      tokens_.emplace(recognizer, token);
    }
    return token;
  }

  void Unregister(void* token, Recognizers recognizers) {
    std::lock_guard<std::mutex> lock(mutex_);
    views_.erase(token);
    for (auto* recognizer : recognizers) {
      tokens_.erase(recognizer);
    }
  }

  std::shared_ptr<NativeServalMarkdownView> GetView(void* token) {
    std::lock_guard<std::mutex> lock(mutex_);
    return FindView(token);
  }

  std::shared_ptr<NativeServalMarkdownView> GetView(
      ArkUI_GestureRecognizer* recognizer) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokens_.find(recognizer);
    return it != tokens_.end() ? FindView(it->second) : nullptr;
  }

 private:
  std::shared_ptr<NativeServalMarkdownView> FindView(void* token) const {
    auto it = views_.find(token);
    return it != views_.end() ? it->second.lock() : nullptr;
  }

  std::mutex mutex_;
  uintptr_t next_token_{1};
  std::unordered_map<ArkUI_GestureRecognizer*, void*> tokens_;
  std::unordered_map<void*, std::weak_ptr<NativeServalMarkdownView>> views_;
};
}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_GESTURE_REGISTRY_H_
