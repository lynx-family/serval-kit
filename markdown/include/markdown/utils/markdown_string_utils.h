// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_STRING_UTILS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_STRING_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace serval::markdown {

inline bool BeginsWith(std::string_view s, std::string_view begin) {
  return s.length() >= begin.length() &&
         s.compare(0, begin.length(), begin) == 0;
}

inline size_t UTF8SequenceLengthNonASCII(char b0) {
  if ((b0 & 0xC0) != 0xC0) {
    return 0;
  }
  if ((b0 & 0xE0) == 0xC0) {
    return 2;
  }
  if ((b0 & 0xF0) == 0xE0) {
    return 3;
  }
  if ((b0 & 0xF8) == 0xF0) {
    return 4;
  }
  return 0;
}

inline size_t UTF8SequenceLength(char b0) {
  return (b0 & ~0x7F) == 0 ? 1 : UTF8SequenceLengthNonASCII(b0);
}

inline size_t UTF8IndexToCIndex(const char* utf8, size_t c_length,
                                size_t utf8_index) {
  size_t current_utf8_index = 0;
  size_t current_c_index = 0;
  while (current_utf8_index != utf8_index && current_c_index < c_length) {
    current_c_index += UTF8SequenceLength(utf8[current_c_index]);
    current_utf8_index++;
  }
  return current_c_index;
}

inline size_t CIndexToUTF8Index(const char* utf8, size_t c_length,
                                size_t c_index) {
  size_t current_c_index = 0;
  size_t current_utf8_index = 0;
  while (current_c_index < c_index && current_c_index < c_length) {
    current_c_index += UTF8SequenceLength(utf8[current_c_index]);
    current_utf8_index++;
  }
  return current_utf8_index;
}

bool StringToInt(const std::string& input, int64_t& output, uint8_t base = 10);
bool StringToInt(const std::string& input, int* output, uint8_t base = 10);
bool StringToFloat(const std::string& input, float& output,
                   bool error_on_nan_or_inf = false);

std::u16string U8StringToU16(std::string_view u8_string);
std::string U32StringToU8(std::u32string_view u32_string);

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_STRING_UTILS_H_
