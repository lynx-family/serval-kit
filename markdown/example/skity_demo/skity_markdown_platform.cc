// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "skity_demo/skity_markdown_platform.h"

#include <utility>

#include "skity_demo/skity_demo_font_manager.h"
#include "skity_demo/skity_markdown_canvas.h"

namespace serval::markdown::example {

SkityMarkdownPlatform::SkityMarkdownPlatform(std::filesystem::path font_root)
    : font_manager_(
          std::make_shared<SkityDemoFontManager>(std::move(font_root))),
      font_collection_(MakeFontCollection(font_manager_)),
      text_layout_(&font_collection_, tttext::kSelfRendering) {}

tttext::TextLayout* SkityMarkdownPlatform::GetTextLayout() {
  return &text_layout_;
}

MarkdownCanvasExtend* SkityMarkdownPlatform::GetMarkdownCanvasExtend(
    tttext::ICanvasHelper* canvas) {
  return static_cast<SkityMarkdownCanvas*>(canvas);
}

SkityDemoFontManager* SkityMarkdownPlatform::GetFontManager() const {
  return font_manager_.get();
}

tttext::FontmgrCollection SkityMarkdownPlatform::MakeFontCollection(
    const std::shared_ptr<SkityDemoFontManager>& font_manager) {
  tttext::FontmgrCollection font_collection(nullptr);
  font_collection.SetAssetFontManager(font_manager);
  return font_collection;
}

}  // namespace serval::markdown::example
