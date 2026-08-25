// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_PLATFORM_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_PLATFORM_IOS_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "markdown/utils/markdown_platform.h"

NS_ASSUME_NONNULL_BEGIN

namespace serval::markdown {

std::unique_ptr<MarkdownPlatform> CreateIOSMarkdownPlatform();

}  // namespace serval::markdown

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_PLATFORM_IOS_H_
