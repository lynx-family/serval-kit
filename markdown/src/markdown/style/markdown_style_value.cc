// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_screen_metrics.h"
namespace lynx::markdown {
float MarkdownStyleLengthValue::GetPx() const {
  if (unit_ == MarkdownStyleLengthUnit::kDp) {
    return MarkdownScreenMetrics::DPToPx(value_);
  }
  return value_;
}

}  // namespace lynx::markdown
