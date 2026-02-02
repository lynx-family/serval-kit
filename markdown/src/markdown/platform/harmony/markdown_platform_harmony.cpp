// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include "markdown/utils/markdown_platform.h"
#include "textra/fontmgr_collection.h"
#include "textra/platform_helper.h"
namespace lynx::markdown {
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

}  // namespace lynx::markdown
