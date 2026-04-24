// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_MARKDOWN_PLATFORM_HARMONY_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_MARKDOWN_PLATFORM_HARMONY_H_

#include <memory>

#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {

std::unique_ptr<MarkdownPlatform> CreateHarmonyMarkdownPlatform();

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_MARKDOWN_PLATFORM_HARMONY_H_
