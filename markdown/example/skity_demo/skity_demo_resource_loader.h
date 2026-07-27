// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_RESOURCE_LOADER_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_RESOURCE_LOADER_H_

#include <memory>

#include "markdown/parser/markdown_resource_loader.h"

namespace serval::markdown::example {

class SkityDemoFontManager;

class SkityDemoResourceLoader final : public MarkdownResourceLoader {
 public:
  explicit SkityDemoResourceLoader(SkityDemoFontManager* font_manager);
  ~SkityDemoResourceLoader() override = default;

  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) override;
  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override;
  void* LoadFont(const char* family, MarkdownFontWeight weight) override;
  MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                              float max_width,
                                              float max_height) override;

 private:
  SkityDemoFontManager* font_manager_{nullptr};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_RESOURCE_LOADER_H_
