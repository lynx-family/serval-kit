// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MARKDOWN_TESTS_PLATFORM_H_
#define MARKDOWN_TESTING_MARKDOWN_MARKDOWN_TESTS_PLATFORM_H_

#include <memory>

#include "markdown/element/markdown_context.h"

namespace serval::markdown::testing {

std::unique_ptr<MarkdownPlatform> CreateTestMarkdownPlatform();
std::shared_ptr<MarkdownContext> CreateTestMarkdownSharedContext();

}  // namespace serval::markdown::testing

#endif  // MARKDOWN_TESTING_MARKDOWN_MARKDOWN_TESTS_PLATFORM_H_
