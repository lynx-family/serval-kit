// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
#include <memory>
#include <string>
#include <vector>

#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_marco.h"
#include "markdown/utils/markdown_value.h"
namespace serval::markdown {
class MarkdownResourceLoader;
class MarkdownTextAttachment;
class MarkdownDocument;
class MarkdownContext;
class L_EXPORT MarkdownStyleReader {
 public:
  static MarkdownStyle ReadStyle(const ValueMap& map,
                                 MarkdownResourceLoader* loader,
                                 MarkdownContext* context = nullptr);
  static std::vector<std::unique_ptr<MarkdownTextAttachment>>
  ReadTextAttachments(Value* array, MarkdownDocument* document);
  static MarkdownBaseStylePart ReadBaseStyle(
      const ValueMap& map, MarkdownResourceLoader* loader,
      MarkdownContext* context = nullptr);
  static uint32_t ReadColor(const std::string& color);
};
}  // namespace serval::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_READER_H_
