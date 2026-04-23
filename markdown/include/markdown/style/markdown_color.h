// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_COLOR_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_COLOR_H_

#include <cstdint>
#include <string_view>

namespace serval::markdown {

class MarkdownColor {
 public:
  static bool Parse(std::string_view value, uint32_t* color);
  static uint32_t MakeArgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  static uint32_t Interpolate(uint32_t start_color, uint32_t end_color,
                              float start_pos, float end_pos,
                              float current_pos);
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_COLOR_H_
