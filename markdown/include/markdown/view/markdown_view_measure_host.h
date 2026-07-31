// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURE_HOST_H_
#define MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURE_HOST_H_

namespace serval::markdown {

class MarkdownViewMeasureHost {
 public:
  virtual ~MarkdownViewMeasureHost() = default;
  virtual void RequestMeasure() = 0;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_VIEW_MARKDOWN_VIEW_MEASURE_HOST_H_
