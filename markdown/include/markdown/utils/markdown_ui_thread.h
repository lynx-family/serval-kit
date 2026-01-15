// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_UI_THREAD_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_UI_THREAD_H_
#include "base/include/fml/task_runner.h"
namespace lynx::markdown {
class MarkdownUIThread {
 public:
  static fml::RefPtr<fml::TaskRunner>& GetUITaskRunner();
};
}  // namespace lynx::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_UI_THREAD_H_
