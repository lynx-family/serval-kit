// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
#include <cstdint>
namespace lynx::markdown {
enum class MarkdownStyleLengthUnit {
  kPx,
  kDp,
};
class MarkdownStyleLengthValue {
 public:
  constexpr MarkdownStyleLengthValue(float value, MarkdownStyleLengthUnit unit)
      : value_(value), unit_(unit) {}
  float GetPx() const;
  constexpr static MarkdownStyleLengthValue FromDp(float value) {
    return {value, MarkdownStyleLengthUnit::kDp};
  }

 private:
  float value_;
  MarkdownStyleLengthUnit unit_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
