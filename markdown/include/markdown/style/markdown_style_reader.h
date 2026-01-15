// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_value.h"
namespace lynx::markdown {
class MarkdownResourceLoader;
class MarkdownStyleReader {
 public:
  static MarkdownStyle ReadStyle(const ValueMap& map,
                                 MarkdownResourceLoader* loader);
  static MarkdownBaseStylePart ReadBaseStyle(const ValueMap& map,
                                             MarkdownResourceLoader* loader);
  static uint32_t ReadColor(std::string_view color);
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
