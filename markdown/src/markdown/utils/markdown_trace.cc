// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/utils/markdown_trace.h"

#include "base/trace/native/trace_event.h"

namespace lynx::markdown {
constexpr const char* MARKDOWN_CATEGORY = "markdown";
void TraceEventBegin(const char* name) {
  TRACE_EVENT_BEGIN(MARKDOWN_CATEGORY, name);
}
void TraceEventEnd() {
  TRACE_EVENT_END(MARKDOWN_CATEGORY);
}
}  // namespace lynx::markdown
