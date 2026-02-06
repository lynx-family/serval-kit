// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_PLATFORM_RESOURCE_LOADER_
#define MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_PLATFORM_RESOURCE_LOADER_
#include <memory>
#include "markdown/style/markdown_style.h"
namespace lynx {
namespace markdown {
class MarkdownDrawable;
class MarkdownPlatformView;
class MarkdownPlatformLoader {
 public:
  virtual ~MarkdownPlatformLoader() = default;
  virtual MarkdownPlatformView* LoadImageView(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) = 0;
  virtual MarkdownPlatformView* LoadInlineView(const char* id_selector,
                                               float max_width,
                                               float max_height) = 0;
  virtual void* LoadFont(const char* family, MarkdownFontWeight weight) = 0;
  virtual MarkdownPlatformView* LoadReplacementView(void* ud, int32_t id,
                                                    float max_width,
                                                    float max_height) = 0;
};
}  // namespace markdown
}  // namespace lynx
#endif  //MARKDOWN_INCLUDE_MARKDOWN_MARKDOWN_PLATFORM_RESOURCE_LOADER_
