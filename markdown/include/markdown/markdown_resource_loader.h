// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_RESOURCE_LOADER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_RESOURCE_LOADER_H_

#include <memory>

#include "markdown/element/markdown_drawable.h"
#include "markdown/style/markdown_style.h"
namespace lynx {
namespace markdown {
class MarkdownDrawable;
class MarkdownPlatformView;
class MarkdownResourceLoader {
 public:
  virtual ~MarkdownResourceLoader() = default;
  virtual MarkdownPlatformView* LoadImageView(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) = 0;
  virtual MarkdownPlatformView* LoadInlineView(const char* id_selector,
                                               float max_width,
                                               float max_height) = 0;
  virtual void* LoadFont(const char* family) = 0;
  virtual std::shared_ptr<MarkdownDrawable> LoadBackgroundDrawable(
      MarkdownBackgroundStylePart* background_style, float border_radius,
      float font_size, float root_font_size) {
    return nullptr;
  }
  virtual MarkdownPlatformView* LoadReplacementView(void* ud, float max_width,
                                                    float max_height) = 0;
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_RESOURCE_LOADER_H_
