// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_MARKDOWN_PLATFORM_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_MARKDOWN_PLATFORM_H_

#include <textra/fontmgr_collection.h>

#include <filesystem>
#include <memory>

#include "markdown/utils/markdown_platform.h"

namespace serval::markdown::example {

class SkityDemoFontManager;

class SkityMarkdownPlatform final : public MarkdownPlatform {
 public:
  explicit SkityMarkdownPlatform(std::filesystem::path font_root);
  ~SkityMarkdownPlatform() override = default;

  tttext::TextLayout* GetTextLayout() override;
  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) override;

  SkityDemoFontManager* GetFontManager() const;

 private:
  static tttext::FontmgrCollection MakeFontCollection(
      const std::shared_ptr<SkityDemoFontManager>& font_manager);

  std::shared_ptr<SkityDemoFontManager> font_manager_;
  tttext::FontmgrCollection font_collection_;
  tttext::TextLayout text_layout_;
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_MARKDOWN_PLATFORM_H_
