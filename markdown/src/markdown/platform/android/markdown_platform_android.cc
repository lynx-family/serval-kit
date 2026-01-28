// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <textra/fontmgr_collection.h>
#include <textra/platform/java/tttext_jni_proxy.h>

#include "markdown/platform/android/markdown_java_canvas_helper.h"
#include "markdown/utils/markdown_platform.h"
#include "markdown/view/markdown_selection_view.h"
namespace lynx {
namespace markdown {
class TextLayoutManager {
 public:
  TextLayoutManager() {
    tttext::FontmgrCollection font_collection(
        tttext::TTTextJNIProxy::GetInstance().GetDefaultFontManager());
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
MarkdownPlatformView* MarkdownSelectionHandle::CreateView(
    lynx::markdown::MarkdownViewContainerHandle* handle,
    lynx::markdown::SelectionHandleType type, float size, float margin,
    uint32_t color) {
  return nullptr;
}
MarkdownPlatformView* MarkdownSelectionHighlight::CreateView(
    lynx::markdown::MarkdownViewContainerHandle* handle, uint32_t color) {
  return nullptr;
}
}  // namespace markdown
}  // namespace lynx
