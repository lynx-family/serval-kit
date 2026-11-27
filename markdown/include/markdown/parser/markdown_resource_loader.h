// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_

#include <memory>
#include <string>
#include <string_view>

#include "markdown/element/markdown_drawable.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace serval::markdown {
class MarkdownPlatformView;
struct MarkdownReplacementView {
  std::shared_ptr<MarkdownDrawable> view_;
  // Text emitted when this replacement view is selected or copied.
  std::string alt_text_;
};
class MarkdownResourceLoader {
 public:
  virtual ~MarkdownResourceLoader() = default;
  virtual std::shared_ptr<MarkdownDrawable> LoadImage(
      const char* src, float desire_width, float desire_height, float max_width,
      float max_height, float border_radius) = 0;
  virtual std::shared_ptr<MarkdownDrawable> LoadInlineView(
      const char* id_selector, float max_width, float max_height) = 0;
  virtual void* LoadFont(const char* family, MarkdownFontWeight weight) = 0;
  virtual MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                                      float max_width,
                                                      float max_height) = 0;
  // Formula excludes delimiters. Return null to display the original source.
  // When an asynchronous result is ready, request a measure on the layout thread.
  virtual std::shared_ptr<MarkdownDrawable> LoadFormula(
      std::string_view formula, float font_size, uint32_t color,
      float max_width, bool display_mode) {
    return nullptr;
  }
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PARSER_MARKDOWN_RESOURCE_LOADER_H_
