// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_CONTEXT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_CONTEXT_H_

#include "markdown/utils/markdown_screen_metrics.h"

namespace serval::markdown {

class MarkdownContext {
 public:
  MarkdownContext() = default;
  ~MarkdownContext() = default;

  MarkdownScreenMetrics& GetScreenMetrics() { return screen_metrics_; }
  const MarkdownScreenMetrics& GetScreenMetrics() const {
    return screen_metrics_;
  }

 protected:
  MarkdownScreenMetrics screen_metrics_{};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_CONTEXT_H_
