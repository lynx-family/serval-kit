// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_BUFFER_READER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_BUFFER_READER_H_
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "markdown/platform/android/buffer_input_stream.h"
#include "markdown/utils/markdown_value.h"
class MarkdownBufferReader {
 public:
  explicit MarkdownBufferReader(BufferInputStream& stream) : stream_(stream) {}
  std::unique_ptr<lynx::markdown::Value> ReadValue();

 protected:
  lynx::markdown::ValueMap ReadMap();
  lynx::markdown::ValueArray ReadArray();

 private:
  BufferInputStream& stream_;
};

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_BUFFER_READER_H_
