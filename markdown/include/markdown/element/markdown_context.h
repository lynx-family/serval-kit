// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_

#include <memory>
#include <utility>

#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {

class MarkdownContext {
 public:
  explicit MarkdownContext(std::unique_ptr<MarkdownPlatform> platform)
      : platform_(std::move(platform)) {}

  tttext::TextLayout* GetTextLayout() const {
    return platform_ == nullptr ? nullptr : platform_->GetTextLayout();
  }

  MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas) const {
    return platform_ == nullptr ? nullptr
                                : platform_->GetMarkdownCanvasExtend(canvas);
  }

 private:
  std::unique_ptr<MarkdownPlatform> platform_;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_CONTEXT_H_
