// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_

#include <memory>

#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {
class MarkdownPlatformView;
class MarkdownResourceLoader {
 public:
  virtual ~MarkdownResourceLoader() = default;
  virtual std::unique_ptr<tttext::RunDelegate> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) = 0;
  virtual std::unique_ptr<tttext::RunDelegate> LoadInlineView(
      const char* id_selector, float max_width, float max_height) = 0;
  virtual void* LoadFont(const char* family, MarkdownFontWeight weight) = 0;
  virtual std::unique_ptr<tttext::RunDelegate> LoadGradient(
      const char* gradient, float font_size, float root_font_size) {
    return nullptr;
  }
  virtual std::unique_ptr<tttext::RunDelegate> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) = 0;
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
