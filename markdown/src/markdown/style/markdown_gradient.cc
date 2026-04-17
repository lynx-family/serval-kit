// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/style/markdown_gradient.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "markdown/utils/markdown_string_utils.h"

namespace serval::markdown {

namespace {

constexpr std::string_view kLinearPrefix = "linear-gradient(";
constexpr std::string_view kRadialPrefix = "radial-gradient(";
constexpr std::string_view kConicPrefix = "conic-gradient(";
constexpr size_t kInvalidIndex = static_cast<size_t>(-1);

struct ColorStop {
  uint32_t color{0};
  bool has_position{false};
  float position{0};
};

struct LengthData {
  StyleValuePattern raw_pattern{StyleValuePattern::kEmpty};
  double raw_value{0};
  double computed_value{0};
  MarkdownPlatformLengthUnit computed_unit{
      MarkdownPlatformLengthUnit::kNumber};
};

struct PositionToken {
  bool is_keyword{false};
  MarkdownBackgroundPositionType keyword{
      MarkdownBackgroundPositionType::kCenter};
  bool horizontal_only{false};
  bool vertical_only{false};
  StyleValuePattern raw_pattern{StyleValuePattern::kEmpty};
  double raw_value{0};
  double computed_value{0};
  MarkdownPlatformLengthUnit computed_unit{
      MarkdownPlatformLengthUnit::kNumber};
};

struct LegacyPosition {
  int32_t type{0};
  double value{0};
};

enum class ParseStatus {
  kNotFound,
  kSuccess,
  kInvalid,
};

std::string_view Trim(std::string_view value) {
  size_t begin = 0;
  while (begin < value.size() &&
         (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\n' ||
          value[begin] == '\r' || value[begin] == '\f')) {
    begin++;
  }
  size_t end = value.size();
  while (end > begin &&
         (value[end - 1] == ' ' || value[end - 1] == '\t' ||
          value[end - 1] == '\n' || value[end - 1] == '\r' ||
          value[end - 1] == '\f')) {
    end--;
  }
  return value.substr(begin, end - begin);
}

std::string ToLower(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(tolower(c)); });
  return result;
}

bool BeginsWithIgnoreCase(std::string_view value, std::string_view prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); i++) {
    const auto left =
        static_cast<char>(tolower(static_cast<unsigned char>(value[i])));
    const auto right =
        static_cast<char>(tolower(static_cast<unsigned char>(prefix[i])));
    if (left != right) {
      return false;
    }
  }
  return true;
}

std::vector<std::string_view> SplitTopLevel(std::string_view value, char split) {
  std::vector<std::string_view> result;
  size_t start = 0;
  int depth = 0;
  for (size_t i = 0; i < value.size(); i++) {
    switch (value[i]) {
      case '(':
        depth++;
        break;
      case ')':
        depth--;
        break;
      default:
        break;
    }
    if (value[i] == split && depth == 0) {
      const auto part = Trim(value.substr(start, i - start));
      if (part.empty()) {
        return {};
      }
      result.emplace_back(part);
      start = i + 1;
    }
  }
  const auto tail = Trim(value.substr(start));
  if (tail.empty()) {
    return {};
  }
  result.emplace_back(tail);
  return result;
}

std::vector<std::string_view> SplitTopLevelSpaces(std::string_view value) {
  std::vector<std::string_view> result;
  size_t start = kInvalidIndex;
  int depth = 0;
  for (size_t i = 0; i < value.size(); i++) {
    const char c = value[i];
    if (c == '(') {
      depth++;
    } else if (c == ')') {
      depth--;
    }
    if (depth == 0 &&
        (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')) {
      if (start != kInvalidIndex) {
        result.emplace_back(value.substr(start, i - start));
        start = kInvalidIndex;
      }
      continue;
    }
    if (start == kInvalidIndex) {
      start = i;
    }
  }
  if (start != kInvalidIndex) {
    result.emplace_back(value.substr(start));
  }
  return result;
}

size_t FindMatchingParenthesis(std::string_view value, size_t open_index) {
  int depth = 0;
  for (size_t i = open_index; i < value.size(); i++) {
    if (value[i] == '(') {
      depth++;
    } else if (value[i] == ')') {
      depth--;
      if (depth == 0) {
        return i;
      }
    }
  }
  return std::string_view::npos;
}

std::unique_ptr<Value> MakeNumericValue(double value) {
  if (std::floor(value) == value &&
      value >= static_cast<double>(std::numeric_limits<int32_t>::min()) &&
      value <= static_cast<double>(std::numeric_limits<int32_t>::max())) {
    return Value::MakeInt(static_cast<int32_t>(value));
  }
  return Value::MakeDouble(value);
}

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

uint32_t MakeArgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return (static_cast<uint32_t>(a) << 24) |
         (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

bool ParseHexColor(std::string_view value, uint32_t* color) {
  if (value.empty() || value[0] != '#') {
    return false;
  }
  const auto hex = value.substr(1);
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
  if (hex.size() == 3 || hex.size() == 4) {
    const auto r = nibble(hex[0]);
    const auto g = nibble(hex[1]);
    const auto b = nibble(hex[2]);
    const auto a = hex.size() == 4 ? nibble(hex[3]) : 0xF;
    if (r < 0 || g < 0 || b < 0 || a < 0) {
      return false;
    }
    *color = MakeArgb(static_cast<uint8_t>((r << 4) | r),
                      static_cast<uint8_t>((g << 4) | g),
                      static_cast<uint8_t>((b << 4) | b),
                      static_cast<uint8_t>((a << 4) | a));
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
    *color = MakeArgb(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                      static_cast<uint8_t>(b), static_cast<uint8_t>(a));
    return true;
  }
  return false;
}

bool ParseCssNumber(std::string_view value, float* result) {
  return StringToFloat(std::string(value), *result, true);
}

bool ParseCssComponent(std::string_view value, float max_value,
                       float* result) {
  auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  float number = 0;
  if (trimmed.back() == '%') {
    if (!ParseCssNumber(trimmed.substr(0, trimmed.size() - 1), &number)) {
      return false;
    }
    *result = number * max_value / 100.f;
    return true;
  }
  if (!ParseCssNumber(trimmed, &number)) {
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
  return MakeArgb(ClampByte(HueToRgb(m1, m2, hue + 1.0f / 3.0f) * 255.f),
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
  const auto name = ToLower(Trim(value.substr(0, open)));
  const auto parts = SplitTopLevel(value.substr(open + 1, close - open - 1),
                                   ',');
  if (name == "rgb" || name == "rgba") {
    if ((name == "rgb" && parts.size() != 3) ||
        (name == "rgba" && parts.size() != 4)) {
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
    *color = MakeArgb(ClampByte(r), ClampByte(g), ClampByte(b),
                      ClampByte(ClampFloat01(a) * 255.f));
    return true;
  }
  if (name == "hsl" || name == "hsla") {
    if ((name == "hsl" && parts.size() != 3) ||
        (name == "hsla" && parts.size() != 4)) {
      return false;
    }
    float h = 0;
    float s = 0;
    float l = 0;
    float a = 1;
    if (!ParseCssNumber(parts[0], &h) || !ParseCssComponent(parts[1], 1.f, &s) ||
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
  static const std::unordered_map<std::string, uint32_t> kNamedColors = {
      {"transparent", 0x00000000},
      {"black", 0xff000000},
      {"white", 0xffffffff},
      {"red", 0xffff0000},
      {"green", 0xff008000},
      {"blue", 0xff0000ff},
      {"yellow", 0xffffff00},
      {"cyan", 0xff00ffff},
      {"magenta", 0xffff00ff},
      {"gray", 0xff808080},
      {"grey", 0xff808080},
      {"orange", 0xffffa500},
      {"purple", 0xff800080},
  };
  const auto lower = ToLower(Trim(value));
  const auto it = kNamedColors.find(lower);
  if (it == kNamedColors.end()) {
    return false;
  }
  *color = it->second;
  return true;
}

bool ParseColor(std::string_view value, uint32_t* color) {
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  return ParseHexColor(trimmed, color) || ParseFunctionalColor(trimmed, color) ||
         ParseNamedColor(trimmed, color);
}

bool ExtractColorStop(std::string_view value, ColorStop* stop) {
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  std::string_view color_part;
  std::string_view remainder;
  if (trimmed[0] == '#') {
    size_t end = 0;
    while (end < trimmed.size() && trimmed[end] != ' ' && trimmed[end] != '\t' &&
           trimmed[end] != '\n' && trimmed[end] != '\r' &&
           trimmed[end] != '\f') {
      end++;
    }
    color_part = trimmed.substr(0, end);
    remainder = trimmed.substr(end);
  } else {
    const auto open = trimmed.find('(');
    const auto first_space = trimmed.find_first_of(" \t\n\r\f");
    if (open != std::string_view::npos &&
        (first_space == std::string_view::npos || open < first_space)) {
      const auto close = FindMatchingParenthesis(trimmed, open);
      if (close == std::string_view::npos) {
        return false;
      }
      color_part = trimmed.substr(0, close + 1);
      remainder = trimmed.substr(close + 1);
    } else {
      const auto end =
          first_space == std::string_view::npos ? trimmed.size() : first_space;
      color_part = trimmed.substr(0, end);
      remainder = trimmed.substr(end);
    }
  }

  if (!ParseColor(color_part, &stop->color)) {
    return false;
  }

  remainder = Trim(remainder);
  if (remainder.empty()) {
    stop->has_position = false;
    return true;
  }

  float position = 0;
  auto lower = ToLower(remainder);
  if (!lower.empty() && lower.back() == '%') {
    if (!ParseCssNumber(lower.substr(0, lower.size() - 1), &position)) {
      return false;
    }
  } else {
    if (!ParseCssNumber(lower, &position)) {
      return false;
    }
    position *= 100.f;
  }
  stop->has_position = true;
  stop->position = position;
  return true;
}

uint32_t InterpolateColor(uint32_t start_color, uint32_t end_color,
                          float start_pos, float end_pos, float current_pos) {
  if (end_pos == start_pos) {
    return end_color;
  }
  const float weight = (current_pos - start_pos) / (end_pos - start_pos);
  const auto lerp = [weight](int start, int end) -> uint8_t {
    return ClampByte(start + (end - start) * weight);
  };
  return MakeArgb(
      lerp((start_color >> 16) & 0xff, (end_color >> 16) & 0xff),
      lerp((start_color >> 8) & 0xff, (end_color >> 8) & 0xff),
      lerp(start_color & 0xff, end_color & 0xff),
      lerp((start_color >> 24) & 0xff, (end_color >> 24) & 0xff));
}

void ClampColorStopsAtFront(std::vector<uint32_t>* colors,
                            std::vector<float>* stops,
                            uint32_t first_positive_index) {
  const float prev_stop = (*stops)[first_positive_index - 1];
  const uint32_t prev_color = (*colors)[first_positive_index - 1];
  const float current_stop = (*stops)[first_positive_index];
  const uint32_t current_color = (*colors)[first_positive_index];
  (*colors)[first_positive_index - 1] = InterpolateColor(
      prev_color, current_color, prev_stop, current_stop, 0.f);
  (*stops)[first_positive_index - 1] = 0.f;
  if (first_positive_index > 1) {
    colors->erase(colors->begin(), colors->begin() + first_positive_index - 1);
    stops->erase(stops->begin(), stops->begin() + first_positive_index - 1);
  }
}

void ClampColorStopsAtBack(std::vector<uint32_t>* colors,
                           std::vector<float>* stops,
                           uint32_t tail_index) {
  const float prev_stop = (*stops)[tail_index - 1];
  const uint32_t prev_color = (*colors)[tail_index - 1];
  const float current_stop = (*stops)[tail_index];
  const uint32_t current_color = (*colors)[tail_index];
  (*colors)[tail_index] = InterpolateColor(prev_color, current_color, prev_stop,
                                           current_stop, 100.f);
  (*stops)[tail_index] = 100.f;
  if (tail_index + 1 < stops->size()) {
    colors->erase(colors->begin() + tail_index + 1, colors->end());
    stops->erase(stops->begin() + tail_index + 1, stops->end());
  }
}

void ClampColorStops(std::vector<uint32_t>* colors, std::vector<float>* stops) {
  if (stops->size() < 2) {
    return;
  }
  if (stops->front() < 0.f) {
    const auto it = std::find_if(stops->begin(), stops->end(),
                                 [](float value) { return value >= 0.f; });
    if (it != stops->begin() && it != stops->end()) {
      ClampColorStopsAtFront(colors, stops,
                             static_cast<uint32_t>(it - stops->begin()));
    }
  }
  if (!stops->empty() && stops->back() > 100.f) {
    const auto it = std::find_if(stops->begin(), stops->end(),
                                 [](float value) { return value >= 100.f; });
    if (it != stops->begin() && it != stops->end()) {
      ClampColorStopsAtBack(colors, stops,
                            static_cast<uint32_t>(it - stops->begin()));
    }
  }
}

bool ParseColorStopList(const std::vector<std::string_view>& args,
                        size_t start_index, std::vector<uint32_t>* colors,
                        std::vector<float>* stops) {
  size_t position_begin_index = kInvalidIndex;
  float position_begin_value = 0;
  for (size_t index = start_index; index < args.size(); index++) {
    ColorStop stop;
    if (!ExtractColorStop(args[index], &stop)) {
      return false;
    }
    colors->emplace_back(stop.color);
    if (!stop.has_position) {
      if (position_begin_index == kInvalidIndex) {
        position_begin_index = colors->size() - 1;
      }
      continue;
    }
    if (position_begin_index != kInvalidIndex) {
      const auto current_index = colors->size() - 1;
      if (position_begin_index > 0) {
        position_begin_value = (*stops)[position_begin_index - 1];
      } else {
        position_begin_index++;
        stops->emplace_back(0.f);
      }
      const float step =
          (stop.position - position_begin_value) /
          static_cast<float>(current_index - position_begin_index + 1);
      for (size_t fill_index = position_begin_index; fill_index < current_index;
           fill_index++) {
        stops->emplace_back(position_begin_value +
                            static_cast<float>(fill_index - position_begin_index + 1) *
                                step);
      }
    }
    stops->emplace_back(stop.position);
    position_begin_index = kInvalidIndex;
  }

  const int32_t fill_step =
      static_cast<int32_t>(colors->size() - stops->size());
  if (!stops->empty() && fill_step > 0) {
    const float begin_value = stops->back();
    const float step_value = (100.f - begin_value) / fill_step;
    for (int32_t i = 1; i < fill_step; i++) {
      stops->emplace_back(begin_value + step_value * i);
    }
    stops->emplace_back(100.f);
  }

  ClampColorStops(colors, stops);
  return colors->size() >= 2 &&
         (stops->empty() || stops->size() == colors->size());
}

double GetRawNumber(const MarkdownStyleValue* value, double fallback) {
  if (const auto* length = dynamic_cast<const MarkdownLengthValue*>(value)) {
    return length->GetValue();
  }
  if (const auto* number = dynamic_cast<const MarkdownNumberValue*>(value)) {
    return number->value_;
  }
  return fallback;
}

bool ParseLengthValue(std::string_view token,
                      const MarkdownLengthContext& context,
                      LengthData* result) {
  const auto lower = ToLower(Trim(token));
  auto value = MarkdownStyleValue::ParseValue(lower);
  if (value == nullptr || !value->IsValid()) {
    return false;
  }
  result->raw_pattern = value->GetType();
  result->computed_value = value->CalculateLengthValue(context);
  result->raw_value = GetRawNumber(value.get(), result->computed_value);
  result->computed_unit = value->GetType() == StyleValuePattern::kPercent
                              ? MarkdownPlatformLengthUnit::kPercentage
                              : MarkdownPlatformLengthUnit::kNumber;
  return true;
}

bool ParsePositionToken(std::string_view token,
                        const MarkdownLengthContext& context,
                        PositionToken* result) {
  const auto lower = ToLower(Trim(token));
  if (lower == "left") {
    result->is_keyword = true;
    result->keyword = MarkdownBackgroundPositionType::kLeft;
    result->horizontal_only = true;
    return true;
  }
  if (lower == "right") {
    result->is_keyword = true;
    result->keyword = MarkdownBackgroundPositionType::kRight;
    result->horizontal_only = true;
    return true;
  }
  if (lower == "top") {
    result->is_keyword = true;
    result->keyword = MarkdownBackgroundPositionType::kTop;
    result->vertical_only = true;
    return true;
  }
  if (lower == "bottom") {
    result->is_keyword = true;
    result->keyword = MarkdownBackgroundPositionType::kBottom;
    result->vertical_only = true;
    return true;
  }
  if (lower == "center") {
    result->is_keyword = true;
    result->keyword = MarkdownBackgroundPositionType::kCenter;
    return true;
  }

  LengthData length;
  if (!ParseLengthValue(lower, context, &length)) {
    return false;
  }
  result->raw_pattern = length.raw_pattern;
  result->raw_value = length.raw_value;
  result->computed_value = length.computed_value;
  result->computed_unit = length.computed_unit;
  return true;
}

void PositionFromOneValue(const PositionToken& value, PositionToken* x,
                          PositionToken* y) {
  *x = value;
  y->is_keyword = true;
  y->keyword = MarkdownBackgroundPositionType::kCenter;
  if (value.vertical_only) {
    std::swap(*x, *y);
  }
}

void PositionFromTwoValues(const PositionToken& first, const PositionToken& second,
                           PositionToken* x, PositionToken* y) {
  *x = first;
  *y = second;
  if (first.vertical_only || second.horizontal_only) {
    std::swap(*x, *y);
  }
}

bool ParseBackgroundPosition(const std::vector<std::string_view>& tokens,
                             const MarkdownLengthContext& context,
                             PositionToken* x, PositionToken* y) {
  if (tokens.empty() || tokens.size() > 2) {
    return false;
  }
  PositionToken value1;
  if (!ParsePositionToken(tokens[0], context, &value1)) {
    return false;
  }
  if (tokens.size() == 1) {
    PositionFromOneValue(value1, x, y);
    return true;
  }
  PositionToken value2;
  if (!ParsePositionToken(tokens[1], context, &value2)) {
    return false;
  }
  if (value1.vertical_only && !value2.is_keyword) {
    return false;
  }
  PositionFromTwoValues(value1, value2, x, y);
  return true;
}

LegacyPosition ToLegacyPosition(const PositionToken& token) {
  if (token.is_keyword) {
    const auto keyword = static_cast<int32_t>(token.keyword);
    return {.type = -keyword, .value = static_cast<double>(keyword)};
  }
  if (token.computed_unit == MarkdownPlatformLengthUnit::kPercentage) {
    return {.type = static_cast<int32_t>(StyleValuePattern::kPercent),
            .value = token.raw_value};
  }
  return {.type = static_cast<int32_t>(StyleValuePattern::kPx),
          .value = token.computed_value};
}

void ToConicPosition(const PositionToken& token, double* value, int32_t* unit) {
  if (token.is_keyword) {
    switch (token.keyword) {
      case MarkdownBackgroundPositionType::kLeft:
      case MarkdownBackgroundPositionType::kTop:
        *value = 0;
        *unit = static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage);
        return;
      case MarkdownBackgroundPositionType::kRight:
      case MarkdownBackgroundPositionType::kBottom:
        *value = 100;
        *unit = static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage);
        return;
      case MarkdownBackgroundPositionType::kCenter:
        *value = 50;
        *unit = static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage);
        return;
    }
  }
  if (token.computed_unit == MarkdownPlatformLengthUnit::kPercentage) {
    *value = token.raw_value;
    *unit = static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage);
    return;
  }
  *value = token.computed_value;
  *unit = static_cast<int32_t>(MarkdownPlatformLengthUnit::kNumber);
}

bool ParseAngle(std::string_view value, float* angle) {
  const auto lower = ToLower(Trim(value));
  if (lower.empty()) {
    return false;
  }
  auto parse_angle_with_unit = [&](std::string_view suffix,
                                   float factor) -> bool {
    if (!BeginsWithIgnoreCase(lower.substr(lower.size() - suffix.size()),
                              suffix)) {
      return false;
    }
    float number = 0;
    if (!ParseCssNumber(lower.substr(0, lower.size() - suffix.size()),
                        &number)) {
      return false;
    }
    *angle = number * factor;
    return true;
  };
  if (lower.size() >= 3 && lower.substr(lower.size() - 3) == "deg") {
    return parse_angle_with_unit("deg", 1.f);
  }
  if (lower.size() >= 4 && lower.substr(lower.size() - 4) == "grad") {
    return parse_angle_with_unit("grad", 360.f / 400.f);
  }
  if (lower.size() >= 3 && lower.substr(lower.size() - 3) == "rad") {
    return parse_angle_with_unit("rad", 180.f / static_cast<float>(M_PI));
  }
  if (lower.size() >= 4 && lower.substr(lower.size() - 4) == "turn") {
    return parse_angle_with_unit("turn", 360.f);
  }
  if (lower == "0") {
    *angle = 0;
    return true;
  }
  return false;
}

ParseStatus ParseLinearPrelude(std::string_view value, float* angle,
                               MarkdownLinearGradientDirection* direction) {
  const auto lower = ToLower(Trim(value));
  if (lower.empty()) {
    return ParseStatus::kNotFound;
  }
  if (ParseAngle(lower, angle)) {
    *direction = MarkdownLinearGradientDirection::kAngle;
    return ParseStatus::kSuccess;
  }
  std::string key;
  if (BeginsWithIgnoreCase(lower, "to")) {
    key = lower.substr(2);
  } else if (BeginsWithIgnoreCase(lower, "toleft") ||
             BeginsWithIgnoreCase(lower, "toright") ||
             BeginsWithIgnoreCase(lower, "totop") ||
             BeginsWithIgnoreCase(lower, "tobottom")) {
    key = lower.substr(2);
  } else {
    return ParseStatus::kNotFound;
  }
  key.erase(std::remove_if(key.begin(), key.end(),
                           [](char c) { return c == ' ' || c == '\t'; }),
            key.end());
  static const std::unordered_map<std::string,
                                  std::pair<float, MarkdownLinearGradientDirection>>
      kDirections = {
          {"top", {0.f, MarkdownLinearGradientDirection::kToTop}},
          {"bottom", {180.f, MarkdownLinearGradientDirection::kToBottom}},
          {"left", {270.f, MarkdownLinearGradientDirection::kToLeft}},
          {"right", {90.f, MarkdownLinearGradientDirection::kToRight}},
          {"topleft", {315.f, MarkdownLinearGradientDirection::kToTopLeft}},
          {"lefttop", {315.f, MarkdownLinearGradientDirection::kToTopLeft}},
          {"topright", {45.f, MarkdownLinearGradientDirection::kToTopRight}},
          {"righttop", {45.f, MarkdownLinearGradientDirection::kToTopRight}},
          {"bottomleft", {225.f, MarkdownLinearGradientDirection::kToBottomLeft}},
          {"leftbottom", {225.f, MarkdownLinearGradientDirection::kToBottomLeft}},
          {"bottomright",
           {135.f, MarkdownLinearGradientDirection::kToBottomRight}},
          {"rightbottom",
           {135.f, MarkdownLinearGradientDirection::kToBottomRight}},
      };
  const auto it = kDirections.find(key);
  if (it == kDirections.end()) {
    return ParseStatus::kInvalid;
  }
  *angle = it->second.first;
  *direction = it->second.second;
  return ParseStatus::kSuccess;
}

bool ParseLinearGradient(std::string_view args, ValueArray* output) {
  const auto parts = SplitTopLevel(args, ',');
  if (parts.size() < 2) {
    return false;
  }
  float angle = 180.f;
  auto direction = MarkdownLinearGradientDirection::kToBottom;
  size_t color_start_index = 0;
  const auto prelude = ParseLinearPrelude(parts[0], &angle, &direction);
  if (prelude == ParseStatus::kInvalid) {
    return false;
  }
  if (prelude == ParseStatus::kSuccess) {
    color_start_index = 1;
  }

  std::vector<uint32_t> colors;
  std::vector<float> stops;
  if (!ParseColorStopList(parts, color_start_index, &colors, &stops)) {
    return false;
  }

  output->emplace_back(Value::MakeDouble(angle));
  ValueArray color_array;
  color_array.reserve(colors.size());
  for (const auto color : colors) {
    color_array.emplace_back(Value::MakeLong(color));
  }
  output->emplace_back(Value::MakeArray(std::move(color_array)));

  ValueArray stop_array;
  stop_array.reserve(stops.size());
  for (const auto stop : stops) {
    stop_array.emplace_back(MakeNumericValue(stop));
  }
  output->emplace_back(Value::MakeArray(std::move(stop_array)));
  output->emplace_back(
      Value::MakeInt(static_cast<int32_t>(direction)));
  return true;
}

bool ParseRadialPrelude(std::string_view value,
                        const MarkdownLengthContext& context,
                        ValueArray* shape_array) {
  auto tokens = SplitTopLevelSpaces(value);
  if (tokens.empty()) {
    return false;
  }

  auto shape = MarkdownRadialGradientShapeType::kEllipse;
  auto size_type = MarkdownRadialGradientSizeType::kFarthestCorner;
  bool has_shape = false;
  bool has_size_keyword = false;
  bool has_anything = false;
  LengthData size_x;
  LengthData size_y;
  bool has_size_x = false;
  bool has_size_y = false;
  PositionToken pos_x;
  PositionToken pos_y;
  pos_x.is_keyword = true;
  pos_x.keyword = MarkdownBackgroundPositionType::kCenter;
  pos_y.is_keyword = true;
  pos_y.keyword = MarkdownBackgroundPositionType::kCenter;

  size_t index = 0;
  const auto match_keyword = [&](std::string_view token,
                                 std::string_view keyword) -> bool {
    return ToLower(token) == keyword;
  };
  if (index < tokens.size()) {
    if (match_keyword(tokens[index], "ellipse")) {
      has_shape = true;
      has_anything = true;
      index++;
    } else if (match_keyword(tokens[index], "circle")) {
      has_shape = true;
      has_anything = true;
      shape = MarkdownRadialGradientShapeType::kCircle;
      index++;
    }
  }

  if (index < tokens.size()) {
    const auto lower = ToLower(tokens[index]);
    if (lower == "farthest-corner") {
      has_size_keyword = true;
      has_anything = true;
      size_type = MarkdownRadialGradientSizeType::kFarthestCorner;
      index++;
    } else if (lower == "farthest-side") {
      has_size_keyword = true;
      has_anything = true;
      size_type = MarkdownRadialGradientSizeType::kFarthestSide;
      index++;
    } else if (lower == "closest-corner") {
      has_size_keyword = true;
      has_anything = true;
      size_type = MarkdownRadialGradientSizeType::kClosestCorner;
      index++;
    } else if (lower == "closest-side") {
      has_size_keyword = true;
      has_anything = true;
      size_type = MarkdownRadialGradientSizeType::kClosestSide;
      index++;
    }
  }

  if (index < tokens.size() && ToLower(tokens[index]) != "at") {
    if (!ParseLengthValue(tokens[index], context, &size_x)) {
      return false;
    }
    has_anything = true;
    has_size_x = true;
    index++;
    if (index < tokens.size() && ToLower(tokens[index]) != "at") {
      if (!ParseLengthValue(tokens[index], context, &size_y)) {
        return false;
      }
      has_size_y = true;
      index++;
    }
  }

  if (has_size_keyword && has_size_x) {
    return false;
  }
  if (has_size_x) {
    size_type = MarkdownRadialGradientSizeType::kLength;
  }
  if (has_shape && shape == MarkdownRadialGradientShapeType::kCircle &&
      has_size_y) {
    return false;
  }
  if (has_shape && shape == MarkdownRadialGradientShapeType::kEllipse &&
      has_size_x && !has_size_y) {
    return false;
  }
  if (has_size_x && !has_size_y) {
    shape = MarkdownRadialGradientShapeType::kCircle;
    size_y = size_x;
    has_size_y = true;
  }

  if (index < tokens.size()) {
    if (ToLower(tokens[index]) != "at") {
      return false;
    }
    index++;
    if (!ParseBackgroundPosition(
            std::vector<std::string_view>(tokens.begin() + index, tokens.end()),
            context, &pos_x, &pos_y)) {
      return false;
    }
    has_anything = true;
    index = tokens.size();
  }
  if (!has_anything || index != tokens.size()) {
    return false;
  }

  const auto legacy_x = ToLegacyPosition(pos_x);
  const auto legacy_y = ToLegacyPosition(pos_y);
  shape_array->emplace_back(
      Value::MakeInt(static_cast<int32_t>(shape)));
  shape_array->emplace_back(
      Value::MakeInt(static_cast<int32_t>(size_type)));
  shape_array->emplace_back(Value::MakeInt(legacy_x.type));
  shape_array->emplace_back(MakeNumericValue(legacy_x.value));
  shape_array->emplace_back(Value::MakeInt(legacy_y.type));
  shape_array->emplace_back(MakeNumericValue(legacy_y.value));

  if (size_type == MarkdownRadialGradientSizeType::kLength) {
    auto push_length = [shape_array](const LengthData& length) {
      shape_array->emplace_back(
          Value::MakeInt(static_cast<int32_t>(length.raw_pattern)));
      shape_array->emplace_back(MakeNumericValue(length.raw_value));
    };
    auto push_computed = [shape_array](const LengthData& length) {
      shape_array->emplace_back(MakeNumericValue(length.computed_value));
      shape_array->emplace_back(Value::MakeInt(
          static_cast<int32_t>(length.computed_unit)));
    };
    push_length(size_x);
    push_length(size_y);
    push_computed(size_x);
    push_computed(size_y);
  }
  return true;
}

bool ParseRadialGradient(std::string_view args,
                         const MarkdownLengthContext& context,
                         ValueArray* output) {
  const auto parts = SplitTopLevel(args, ',');
  if (parts.size() < 2) {
    return false;
  }

  size_t color_start_index = 0;
  ColorStop first_color;
  ValueArray shape_array;
  if (!ExtractColorStop(parts[0], &first_color)) {
    if (!ParseRadialPrelude(parts[0], context, &shape_array)) {
      return false;
    }
    color_start_index = 1;
  } else {
    shape_array.emplace_back(Value::MakeInt(
        static_cast<int32_t>(MarkdownRadialGradientShapeType::kEllipse)));
    shape_array.emplace_back(Value::MakeInt(static_cast<int32_t>(
        MarkdownRadialGradientSizeType::kFarthestCorner)));
    const auto center_type =
        -static_cast<int32_t>(MarkdownBackgroundPositionType::kCenter);
    const auto center_value =
        static_cast<int32_t>(MarkdownBackgroundPositionType::kCenter);
    shape_array.emplace_back(Value::MakeInt(center_type));
    shape_array.emplace_back(Value::MakeInt(center_value));
    shape_array.emplace_back(Value::MakeInt(center_type));
    shape_array.emplace_back(Value::MakeInt(center_value));
  }

  std::vector<uint32_t> colors;
  std::vector<float> stops;
  if (!ParseColorStopList(parts, color_start_index, &colors, &stops)) {
    return false;
  }

  output->emplace_back(Value::MakeArray(std::move(shape_array)));
  ValueArray color_array;
  color_array.reserve(colors.size());
  for (const auto color : colors) {
    color_array.emplace_back(Value::MakeLong(color));
  }
  output->emplace_back(Value::MakeArray(std::move(color_array)));

  ValueArray stop_array;
  stop_array.reserve(stops.size());
  for (const auto stop : stops) {
    stop_array.emplace_back(MakeNumericValue(stop));
  }
  output->emplace_back(Value::MakeArray(std::move(stop_array)));
  return true;
}

bool ParseConicPrelude(std::string_view value,
                       const MarkdownLengthContext& context, float* angle,
                       ValueArray* center_array) {
  const auto tokens = SplitTopLevelSpaces(value);
  if (tokens.empty()) {
    return false;
  }
  PositionToken pos_x;
  PositionToken pos_y;
  pos_x.is_keyword = true;
  pos_x.keyword = MarkdownBackgroundPositionType::kCenter;
  pos_y.is_keyword = true;
  pos_y.keyword = MarkdownBackgroundPositionType::kCenter;

  bool consumed_anything = false;
  size_t index = 0;
  if (index < tokens.size() && ToLower(tokens[index]) == "from") {
    if (index + 1 >= tokens.size() || !ParseAngle(tokens[index + 1], angle)) {
      return false;
    }
    consumed_anything = true;
    index += 2;
  }
  if (index < tokens.size() && ToLower(tokens[index]) == "at") {
    if (!ParseBackgroundPosition(
            std::vector<std::string_view>(tokens.begin() + index + 1,
                                          tokens.end()),
            context, &pos_x, &pos_y)) {
      return false;
    }
    consumed_anything = true;
    index = tokens.size();
  }
  if (!consumed_anything || index != tokens.size()) {
    return false;
  }

  double x_value = 0;
  double y_value = 0;
  int32_t x_unit = 0;
  int32_t y_unit = 0;
  ToConicPosition(pos_x, &x_value, &x_unit);
  ToConicPosition(pos_y, &y_value, &y_unit);
  center_array->emplace_back(MakeNumericValue(x_value));
  center_array->emplace_back(Value::MakeInt(x_unit));
  center_array->emplace_back(MakeNumericValue(y_value));
  center_array->emplace_back(Value::MakeInt(y_unit));
  return true;
}

bool ParseConicGradient(std::string_view args,
                        const MarkdownLengthContext& context,
                        ValueArray* output) {
  const auto parts = SplitTopLevel(args, ',');
  if (parts.size() < 2) {
    return false;
  }

  float angle = 0;
  size_t color_start_index = 0;
  ValueArray center_array;
  ColorStop first_color;
  if (!ExtractColorStop(parts[0], &first_color)) {
    if (!ParseConicPrelude(parts[0], context, &angle, &center_array)) {
      return false;
    }
    color_start_index = 1;
  } else {
    center_array.emplace_back(Value::MakeInt(50));
    center_array.emplace_back(Value::MakeInt(
        static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage)));
    center_array.emplace_back(Value::MakeInt(50));
    center_array.emplace_back(Value::MakeInt(
        static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage)));
  }

  std::vector<uint32_t> colors;
  std::vector<float> stops;
  if (!ParseColorStopList(parts, color_start_index, &colors, &stops)) {
    return false;
  }

  output->emplace_back(Value::MakeDouble(angle));
  output->emplace_back(Value::MakeArray(std::move(center_array)));

  ValueArray color_array;
  color_array.reserve(colors.size());
  for (const auto color : colors) {
    color_array.emplace_back(Value::MakeLong(color));
  }
  output->emplace_back(Value::MakeArray(std::move(color_array)));

  ValueArray stop_array;
  stop_array.reserve(stops.size());
  for (const auto stop : stops) {
    stop_array.emplace_back(MakeNumericValue(stop));
  }
  output->emplace_back(Value::MakeArray(std::move(stop_array)));
  return true;
}

}  // namespace

bool IsGradientValue(std::string_view value) {
  const auto trimmed = Trim(value);
  return BeginsWithIgnoreCase(trimmed, kLinearPrefix) ||
         BeginsWithIgnoreCase(trimmed, kRadialPrefix) ||
         BeginsWithIgnoreCase(trimmed, kConicPrefix);
}

std::unique_ptr<Value> ParseGradientValue(
    std::string_view value, const MarkdownLengthContext& context) {
  const auto trimmed = Trim(value);
  if (trimmed.empty() || trimmed.back() != ')') {
    return nullptr;
  }

  MarkdownGradientType type;
  std::string_view args;
  if (BeginsWithIgnoreCase(trimmed, kLinearPrefix)) {
    type = MarkdownGradientType::kLinear;
    args = trimmed.substr(kLinearPrefix.size(),
                          trimmed.size() - kLinearPrefix.size() - 1);
  } else if (BeginsWithIgnoreCase(trimmed, kRadialPrefix)) {
    type = MarkdownGradientType::kRadial;
    args = trimmed.substr(kRadialPrefix.size(),
                          trimmed.size() - kRadialPrefix.size() - 1);
  } else if (BeginsWithIgnoreCase(trimmed, kConicPrefix)) {
    type = MarkdownGradientType::kConic;
    args = trimmed.substr(kConicPrefix.size(),
                          trimmed.size() - kConicPrefix.size() - 1);
  } else {
    return nullptr;
  }

  ValueArray gradient_args;
  bool success = false;
  switch (type) {
    case MarkdownGradientType::kLinear:
      success = ParseLinearGradient(args, &gradient_args);
      break;
    case MarkdownGradientType::kRadial:
      success = ParseRadialGradient(args, context, &gradient_args);
      break;
    case MarkdownGradientType::kConic:
      success = ParseConicGradient(args, context, &gradient_args);
      break;
  }
  if (!success) {
    return nullptr;
  }

  ValueArray result;
  result.emplace_back(Value::MakeInt(static_cast<int32_t>(type)));
  result.emplace_back(Value::MakeArray(std::move(gradient_args)));
  return Value::MakeArray(std::move(result));
}

}  // namespace serval::markdown
