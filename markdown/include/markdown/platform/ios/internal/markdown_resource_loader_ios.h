// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_

#include <string>
#include <unordered_map>

#include "markdown/parser/markdown_resource_loader.h"

#import "markdown/platform/ios/IMarkdownResourceDelegate.h"

namespace serval::markdown {
class MarkdownResourceLoaderIOS final : public MarkdownResourceLoader {
 public:
  MarkdownResourceLoaderIOS() = default;
  ~MarkdownResourceLoaderIOS() override;

  void SetDelegate(id<IMarkdownResourceDelegate> delegate) {
    delegate_ = delegate;
  }

  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float border_radius) override;

  std::shared_ptr<MarkdownDrawable> LoadInlineView(const char* id_selector,
                                                   float max_width,
                                                   float max_height) override;

  void* LoadFont(const char* family, MarkdownFontWeight weight) override;

  std::shared_ptr<MarkdownDrawable> LoadGradient(const char* gradient,
                                                 float font_size,
                                                 float root_font_size) override;

  std::shared_ptr<MarkdownDrawable> LoadReplacementView(
      void* ud, int32_t id, float max_width, float max_height) override;

 private:
  __weak id<IMarkdownResourceDelegate> delegate_{nil};
  // Keep returned UIFont handles alive while the loader is alive.
  NSMutableSet<UIFont*>* font_cache_{nil};
};
}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_
