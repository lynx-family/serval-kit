// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include "markdown/platform/harmony/internal/harmony_utils.h"
#include "markdown/utils/markdown_platform.h"
#include "textra/fontmgr_collection.h"
#include "textra/platform_helper.h"
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

class TextLayoutManager {
 public:
  TextLayoutManager() {
    auto ft = tttext::PlatformHelper::CreateFontManager(
        tttext::PlatformType::kSystem);
    tttext::FontmgrCollection collection(ft);
    text_layout_ = std::make_unique<tttext::TextLayout>(
        &collection, tttext::ShaperType::kSystem);
  }
  ~TextLayoutManager() = default;

  tttext::TextLayout* GetTextLayout() { return text_layout_.get(); }
  std::unique_ptr<tttext::TextLayout> text_layout_;
};
tttext::TextLayout* MarkdownPlatform::GetTextLayout() {
  thread_local TextLayoutManager manager;
  return manager.GetTextLayout();
}
MarkdownCanvasExtend* MarkdownPlatform::GetMarkdownCanvasExtend(
    tttext::ICanvasHelper* canvas) {
  return static_cast<HarmonyMarkdownCanvas*>(canvas);
}

void MarkdownPlatform::RunOnUIThread(std::function<void()> task,
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
