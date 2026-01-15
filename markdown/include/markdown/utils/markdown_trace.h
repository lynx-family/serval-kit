// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_TRACE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_TRACE_H_
namespace lynx::markdown {
void TraceEventBegin(const char* name);
void TraceEventEnd();
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_TRACE_H_
