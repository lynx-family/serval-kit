// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/utils/markdown_ui_thread.h"

namespace lynx::markdown {
fml::RefPtr<fml::TaskRunner>& MarkdownUIThread::GetUITaskRunner() {
  static fml::RefPtr<fml::TaskRunner> ui_task_runner;
  return ui_task_runner;
}
}  // namespace lynx::markdown
