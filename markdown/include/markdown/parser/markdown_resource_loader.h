// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_

#include <memory>

#include "markdown/element/markdown_drawable.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace serval::markdown {
class MarkdownPlatformView;
class MarkdownResourceLoader {
 public:
  virtual ~MarkdownResourceLoader() = default;
  virtual std::shared_ptr<MarkdownDrawable> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) = 0;
  virtual std::shared_ptr<MarkdownDrawable> LoadInlineView(
      const char* id_selector, float max_width, float max_height) = 0;
  virtual void* LoadFont(const char* family, MarkdownFontWeight weight) = 0;
  virtual std::shared_ptr<MarkdownDrawable> LoadGradient(const char* gradient,
                                                         float font_size,
                                                         float root_font_size) {
    return nullptr;
  }
  virtual std::shared_ptr<MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) = 0;
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
