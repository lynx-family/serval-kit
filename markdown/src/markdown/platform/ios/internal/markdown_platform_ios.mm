// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include "markdown/platform/ios/internal/markdown_canvas_ios.h"
#include "markdown/utils/markdown_platform.h"
#import "textra/fontmgr_collection.h"
#import "textra/platform/ios/ios_font_manager.h"
#import "textra/text_layout.h"

namespace lynx {
namespace markdown {
class TextLayoutManager {
 public:
  TextLayoutManager() {
    FontManagerRef ft = std::make_shared<tttext::FontManagerCoreText>();
    tttext::FontmgrCollection font_collection(ft);
    textlayout_ = std::make_unique<tttext::TextLayout>(
        &font_collection, tttext::ShaperType::kSystem);
  }
  ~TextLayoutManager() = default;
  tttext::TextLayout* GetLayout() { return textlayout_.get(); }

 private:
  std::unique_ptr<tttext::TextLayout> textlayout_;
};
tttext::TextLayout* MarkdownPlatform::GetTextLayout() {
  thread_local TextLayoutManager text_layout_mgr;
  return text_layout_mgr.GetLayout();
}
MarkdownCanvasExtend* MarkdownPlatform::GetMarkdownCanvasExtend(
    tttext::ICanvasHelper* canvas) {
  return static_cast<MarkdownCanvasIOS*>(canvas);
}
}  // namespace markdown
}  // namespace lynx
