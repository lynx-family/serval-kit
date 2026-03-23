// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_screen_metrics.h"

namespace serval::markdown {
MarkdownScreenMetrics& MarkdownScreenMetrics::Ins() {
  static MarkdownScreenMetrics ins;
  return ins;
}

}  // namespace serval::markdown
