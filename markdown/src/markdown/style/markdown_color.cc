// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/style/markdown_color.h"

#include <cmath>
#include <string_view>

#include "markdown/utils/markdown_string_utils.h"

namespace serval::markdown {

namespace {

uint8_t ClampByte(float value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return static_cast<uint8_t>(std::round(value));
}

float ClampFloat01(float value) {
  if (value < 0) {
    return 0;
  }
  if (value > 1) {
    return 1;
  }
  return value;
}

bool ParseHexColor(std::string_view value, uint32_t* color) {
  if (value.empty()) {
    return false;
  }
  const bool has_hash = value[0] == '#';
  const auto hex = has_hash ? value.substr(1) : value;
  auto nibble = [](char c) -> int32_t {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    return -1;
  };
  auto byte_from_pair = [&](char high, char low) -> int32_t {
    const auto high_value = nibble(high);
    const auto low_value = nibble(low);
    if (high_value < 0 || low_value < 0) {
      return -1;
    }
    return (high_value << 4) | low_value;
  };
  if (!has_hash) {
    if (hex.empty() || hex.size() > 8) {
      return false;
    }
    uint32_t legacy_color = 0;
    for (const auto c : hex) {
      const auto value = nibble(c);
      if (value < 0) {
        return false;
      }
      legacy_color = (legacy_color << 4) | static_cast<uint32_t>(value);
    }
    if (hex.size() <= 6) {
      legacy_color |= 0xff000000;
    }
    *color = legacy_color;
    return true;
  }
  if (hex.size() == 3 || hex.size() == 4) {
    const auto r = nibble(hex[0]);
    const auto g = nibble(hex[1]);
    const auto b = nibble(hex[2]);
    const auto a = hex.size() == 4 ? nibble(hex[3]) : 0xF;
    if (r < 0 || g < 0 || b < 0 || a < 0) {
      return false;
    }
    *color = MarkdownColor::MakeArgb(
        static_cast<uint8_t>((r << 4) | r), static_cast<uint8_t>((g << 4) | g),
        static_cast<uint8_t>((b << 4) | b), static_cast<uint8_t>((a << 4) | a));
    return true;
  }
  if (hex.size() == 6 || hex.size() == 8) {
    const auto r = byte_from_pair(hex[0], hex[1]);
    const auto g = byte_from_pair(hex[2], hex[3]);
    const auto b = byte_from_pair(hex[4], hex[5]);
    const auto a = hex.size() == 8 ? byte_from_pair(hex[6], hex[7]) : 0xFF;
    if (r < 0 || g < 0 || b < 0 || a < 0) {
      return false;
    }
    *color = MarkdownColor::MakeArgb(
        static_cast<uint8_t>(r), static_cast<uint8_t>(g),
        static_cast<uint8_t>(b), static_cast<uint8_t>(a));
    return true;
  }
  return false;
}

bool ParseCssComponent(std::string_view value, float max_value, float* result) {
  auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  float number = 0;
  if (trimmed.back() == '%') {
    if (!StringToFloat(trimmed.substr(0, trimmed.size() - 1), number, true)) {
      return false;
    }
    *result = number * max_value / 100.f;
    return true;
  }
  if (!StringToFloat(trimmed, number, true)) {
    return false;
  }
  *result = number;
  return true;
}

float HueToRgb(float m1, float m2, float h) {
  if (h < 0) {
    h += 1;
  }
  if (h > 1) {
    h -= 1;
  }
  if (6 * h < 1) {
    return m1 + (m2 - m1) * h * 6;
  }
  if (2 * h < 1) {
    return m2;
  }
  if (3 * h < 2) {
    return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6;
  }
  return m1;
}

uint32_t HslaToArgb(float h, float s, float l, float a) {
  h = std::fmod(h, 360.f);
  if (h < 0) {
    h += 360.f;
  }
  if (s > 1) {
    s *= 0.01f;
  }
  if (l > 1) {
    l *= 0.01f;
  }
  s = ClampFloat01(s);
  l = ClampFloat01(l);
  const float hue = h / 360.f;
  const float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
  const float m1 = l * 2.0f - m2;
  return MarkdownColor::MakeArgb(
      ClampByte(HueToRgb(m1, m2, hue + 1.0f / 3.0f) * 255.f),
      ClampByte(HueToRgb(m1, m2, hue) * 255.f),
      ClampByte(HueToRgb(m1, m2, hue - 1.0f / 3.0f) * 255.f),
      ClampByte(ClampFloat01(a) * 255.f));
}

bool ParseFunctionalColor(std::string_view value, uint32_t* color) {
  const auto open = value.find('(');
  const auto close = open == std::string_view::npos
                         ? std::string_view::npos
                         : FindMatchingParenthesis(value, open);
  if (open == std::string_view::npos || close == std::string_view::npos ||
      close + 1 != value.size()) {
    return false;
  }
  const auto name = Trim(value.substr(0, open));
  const auto parts =
      SplitTopLevel(value.substr(open + 1, close - open - 1), ',');
  const bool is_rgb = name == "rgb";
  const bool is_rgba = name == "rgba";
  const bool is_hsl = name == "hsl";
  const bool is_hsla = name == "hsla";

  if (is_rgb || is_rgba) {
    if ((is_rgb && parts.size() != 3) || (is_rgba && parts.size() != 4)) {
      return false;
    }
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
    if (!ParseCssComponent(parts[0], 255.f, &r) ||
        !ParseCssComponent(parts[1], 255.f, &g) ||
        !ParseCssComponent(parts[2], 255.f, &b)) {
      return false;
    }
    if (parts.size() == 4 && !ParseCssComponent(parts[3], 1.f, &a)) {
      return false;
    }
    *color = MarkdownColor::MakeArgb(ClampByte(r), ClampByte(g), ClampByte(b),
                                     ClampByte(ClampFloat01(a) * 255.f));
    return true;
  }
  if (is_hsl || is_hsla) {
    if ((is_hsl && parts.size() != 3) || (is_hsla && parts.size() != 4)) {
      return false;
    }
    float h = 0;
    float s = 0;
    float l = 0;
    float a = 1;
    if (!StringToFloat(parts[0], h, true) ||
        !ParseCssComponent(parts[1], 1.f, &s) ||
        !ParseCssComponent(parts[2], 1.f, &l)) {
      return false;
    }
    if (parts.size() == 4 && !ParseCssComponent(parts[3], 1.f, &a)) {
      return false;
    }
    *color = HslaToArgb(h, s, l, a);
    return true;
  }
  return false;
}

bool ParseNamedColor(std::string_view value, uint32_t* color) {
  struct NamedColor {
    const char* name;
    uint32_t color;
  };
  static constexpr NamedColor kNamedColors[] = {
      {"transparent", 0x00000000}, {"black", 0xff000000},
      {"white", 0xffffffff},       {"red", 0xffff0000},
      {"green", 0xff008000},       {"blue", 0xff0000ff},
      {"yellow", 0xffffff00},      {"cyan", 0xff00ffff},
      {"magenta", 0xffff00ff},     {"gray", 0xff808080},
      {"grey", 0xff808080},        {"orange", 0xffffa500},
      {"purple", 0xff800080},
  };
  const auto trimmed = Trim(value);
  for (const auto& named_color : kNamedColors) {
    if (trimmed == named_color.name) {
      *color = named_color.color;
      return true;
    }
  }
  return false;
}

}  // namespace

uint32_t MarkdownColor::MakeArgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

uint32_t MarkdownColor::Interpolate(uint32_t start_color, uint32_t end_color,
                                    float start_pos, float end_pos,
                                    float current_pos) {
  if (end_pos == start_pos) {
    return end_color;
  }
  const float weight = (current_pos - start_pos) / (end_pos - start_pos);
  const auto lerp = [weight](int start, int end) -> uint8_t {
    return ClampByte(start + (end - start) * weight);
  };
  return MakeArgb(lerp((start_color >> 16) & 0xff, (end_color >> 16) & 0xff),
                  lerp((start_color >> 8) & 0xff, (end_color >> 8) & 0xff),
                  lerp(start_color & 0xff, end_color & 0xff),
                  lerp((start_color >> 24) & 0xff, (end_color >> 24) & 0xff));
}

bool MarkdownColor::Parse(std::string_view value, uint32_t* color) {
  if (color == nullptr) {
    return false;
  }
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  return ParseHexColor(trimmed, color) ||
         ParseFunctionalColor(trimmed, color) ||
         ParseNamedColor(trimmed, color);
}

}  // namespace serval::markdown
