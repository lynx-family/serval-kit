// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_string_utils.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace serval::markdown {

namespace {

std::u32string U8StringToU32(std::string_view u8_string) {
  const size_t length = u8_string.length();
  std::u32string u32;
  uint32_t i = 0;
  while (i < length) {
    uint32_t utf32 = static_cast<unsigned char>(u8_string[i]);
    int additional_bytes = 0;
    if ((utf32 & 0x80u) == 0) {
      additional_bytes = 0;
    } else if ((utf32 & 0xE0u) == 0xC0) {
      utf32 &= 0x1Fu;
      additional_bytes = 1;
    } else if ((utf32 & 0xF0u) == 0xE0) {
      utf32 &= 0x0Fu;
      additional_bytes = 2;
    } else if ((utf32 & 0xF8u) == 0xF0) {
      utf32 &= 0x07u;
      additional_bytes = 3;
    } else {
      return U"";
    }

    for (int j = 0; j < additional_bytes; ++j) {
      if (++i >= length) {
        return U"";
      }
      const auto byte = static_cast<unsigned char>(u8_string[i]);
      if ((byte & 0xC0u) != 0x80) {
        return U"";
      }
      utf32 = (utf32 << 6u) | (byte & 0x3Fu);
    }

    u32.push_back(utf32);
    ++i;
  }
  return u32;
}

std::u16string U32StringToU16(std::u32string_view u32_string) {
  std::u16string u16;
  u16.reserve(u32_string.size());
  for (uint32_t utf32 : u32_string) {
    if (utf32 <= 0xFFFF) {
      u16.push_back(static_cast<char16_t>(utf32));
    } else if (utf32 >= 0x10000 && utf32 <= 0x10FFFF) {
      utf32 -= 0x10000;
      u16.push_back(static_cast<char16_t>(0xD800 | ((utf32 >> 10) & 0x3FF)));
      u16.push_back(static_cast<char16_t>(0xDC00 | (utf32 & 0x3FF)));
    } else {
      return u"";
    }
  }
  return u16;
}

}  // namespace

bool StringToFloat(const std::string& input, float& output,
                   bool error_on_nan_or_inf) {
  const int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  const float parsed = strtof(input.c_str(), &endptr);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr &&
                !std::isspace(static_cast<unsigned char>(input[0])));
  if (errno == 0) {
    errno = old_error;
  }
  if (valid) {
    output = parsed;
  }
  if (error_on_nan_or_inf && (std::isnan(parsed) || std::isinf(parsed))) {
    valid = false;
  }
  return valid;
}

bool StringToInt(const std::string& input, int64_t& output, uint8_t base) {
  const int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  const int64_t parsed = strtoll(input.c_str(), &endptr, base);
  const bool valid = (errno == 0 && !input.empty() &&
                      input.c_str() + input.length() == endptr &&
                      !std::isspace(static_cast<unsigned char>(input[0])));
  if (errno == 0) {
    errno = old_error;
  }
  if (valid) {
    output = parsed;
  }
  return valid;
}

bool StringToInt(const std::string& input, int* output, uint8_t base) {
  int64_t parsed = 0;
  if (!StringToInt(input, parsed, base)) {
    return false;
  }
  *output = static_cast<int>(parsed);
  return true;
}

std::u16string U8StringToU16(std::string_view u8_string) {
  return U32StringToU16(U8StringToU32(u8_string));
}

std::string U32StringToU8(std::u32string_view u32_string) {
  std::string utf8;
  utf8.reserve(u32_string.size());
  for (uint32_t utf32 : u32_string) {
    if (utf32 <= 0x7F) {
      utf8.push_back(static_cast<char>(utf32));
    } else if (utf32 <= 0x7FF) {
      utf8.push_back(static_cast<char>(0xC0 | ((utf32 >> 6u) & 0x1Fu)));
      utf8.push_back(static_cast<char>(0x80 | (utf32 & 0x3Fu)));
    } else if (utf32 <= 0xFFFF) {
      utf8.push_back(static_cast<char>(0xE0 | ((utf32 >> 12u) & 0x0Fu)));
      utf8.push_back(static_cast<char>(0x80 | ((utf32 >> 6u) & 0x3Fu)));
      utf8.push_back(static_cast<char>(0x80 | (utf32 & 0x3Fu)));
    } else if (utf32 <= 0x10FFFF) {
      utf8.push_back(static_cast<char>(0xF0 | ((utf32 >> 18u) & 0x07u)));
      utf8.push_back(static_cast<char>(0x80 | ((utf32 >> 12u) & 0x3Fu)));
      utf8.push_back(static_cast<char>(0x80 | ((utf32 >> 6u) & 0x3Fu)));
      utf8.push_back(static_cast<char>(0x80 | (utf32 & 0x3Fu)));
    } else {
      return "";
    }
  }
  return utf8;
}

}  // namespace serval::markdown
