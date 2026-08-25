// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "markdown/parser/markdown_resource_loader.h"

#import "markdown/platform/ios/IMarkdownResourceDelegate.h"

NS_ASSUME_NONNULL_BEGIN

namespace serval::markdown {
class MarkdownResourceLoaderIOS final : public MarkdownResourceLoader {
 public:
  MarkdownResourceLoaderIOS() = default;
  ~MarkdownResourceLoaderIOS() override;

  void SetDelegate(id<IMarkdownResourceDelegate> _Nullable delegate) {
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

  void* _Nullable LoadFont(const char* family,
                           MarkdownFontWeight weight) override;

  MarkdownReplacementView LoadReplacementView(void* ud, int32_t id,
                                              float max_width,
                                              float max_height) override;

 private:
  __weak id<IMarkdownResourceDelegate> _Nullable delegate_{nil};
  // Keep returned UIFont handles alive while the loader is alive.
  NSMutableSet<UIFont*>* _Nullable font_cache_{nil};
};
}  // namespace serval::markdown

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_RESOURCE_LOADER_IOS_H_
