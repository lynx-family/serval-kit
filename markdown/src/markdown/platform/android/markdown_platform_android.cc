// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/markdown_platform_android.h"

#include <textra/fontmgr_collection.h>
#include <textra/platform/java/tttext_jni_proxy.h>

#include "markdown/platform/android/markdown_class_cache.h"
#include "markdown/platform/android/markdown_java_canvas_helper.h"
namespace serval::markdown {
namespace {
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

class AndroidMarkdownPlatform final : public MarkdownPlatform {
 public:
  tttext::TextLayout* GetTextLayout() override {
    thread_local TextLayoutManager text_layout_mgr;
    return text_layout_mgr.GetLayout();
  }

  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) override {
    return static_cast<MarkdownJavaCanvasHelper*>(canvas);
  }
};
}  // namespace

std::unique_ptr<MarkdownPlatform> CreateAndroidMarkdownPlatform() {
  return std::make_unique<AndroidMarkdownPlatform>();
}
}  // namespace serval::markdown
