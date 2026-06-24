// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGAnimation.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

#include "element/SrSVGPath.h"

namespace serval {
namespace svg {
namespace element {

namespace {

constexpr double kPi = 3.14159265358979323846;

struct MotionPoint {
  double x{0.0};
  double y{0.0};
};

struct MotionSample {
  MotionPoint point;
  double angle{0.0};
};

std::string Trim(const std::string& value) {
  size_t begin = 0;
  while (begin < value.size() &&
         std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  size_t end = value.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(begin, end - begin);
}

std::string RemoveSpaces(const std::string& value) {
  std::string result;
  result.reserve(value.size());
  for (char c : value) {
    if (!std::isspace(static_cast<unsigned char>(c))) {
      result.push_back(c);
    }
  }
  return result;
}

std::vector<std::string> Split(const std::string& input, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  std::stringstream stream(input);
  while (std::getline(stream, current, delimiter)) {
    current = Trim(current);
    if (!current.empty()) {
      parts.push_back(std::move(current));
    }
  }
  return parts;
}

std::vector<double> ParseSemicolonNumberList(const char* value) {
  std::vector<double> numbers;
  if (!value) {
    return numbers;
  }
  for (const auto& part : Split(value, ';')) {
    char* end = nullptr;
    const double parsed = std::strtod(part.c_str(), &end);
    if (end != part.c_str()) {
      numbers.push_back(parsed);
    }
  }
  return numbers;
}

double ParsePlainClockValue(const std::string& value) {
  const std::string trimmed = Trim(value);
  if (trimmed.empty() || trimmed == "indefinite") {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const auto clock_parts = Split(trimmed, ':');
  if (clock_parts.size() > 1) {
    double total = 0.0;
    double scale = 1.0;
    for (auto it = clock_parts.rbegin(); it != clock_parts.rend(); ++it) {
      char* end = nullptr;
      const double component = std::strtod(it->c_str(), &end);
      if (end == it->c_str() || (end && *end != '\0')) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      total += component * scale;
      scale *= 60.0;
    }
    return total;
  }

  char* end = nullptr;
  const double number = std::strtod(trimmed.c_str(), &end);
  if (end == trimmed.c_str()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const std::string suffix = Trim(end ? end : "");
  if (suffix.empty() || suffix == "s") {
    return number;
  }
  if (suffix == "ms") {
    return number / 1000.0;
  }
  if (suffix == "min") {
    return number * 60.0;
  }
  if (suffix == "h") {
    return number * 3600.0;
  }
  return std::numeric_limits<double>::quiet_NaN();
}

double ParseClockValue(const char* value) {
  if (!value) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  for (const auto& part : Split(value, ';')) {
    const double parsed = ParsePlainClockValue(part);
    if (!std::isnan(parsed)) {
      return parsed;
    }
  }
  return std::numeric_limits<double>::quiet_NaN();
}

bool ParseNumberWithSuffix(const std::string& value, double* number,
                           std::string* suffix) {
  const char* str = value.c_str();
  char* end = nullptr;
  const double parsed = std::strtod(str, &end);
  if (end == str) {
    return false;
  }
  *number = parsed;
  *suffix = end ? end : "";
  return true;
}

std::string FormatNumber(double value) {
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

double Clamp01(double value) {
  return std::max(0.0, std::min(1.0, value));
}

int ClampByte(double value) {
  return static_cast<int>(std::max(0.0, std::min(255.0, std::round(value))));
}

struct ColorValue {
  double r{0.0};
  double g{0.0};
  double b{0.0};
  double a{255.0};
};

std::vector<double> ParseNumberList(const std::string& value);

bool ParseColorValue(const std::string& value, ColorValue* color) {
  const std::string trimmed = Trim(value);
  if (!color || trimmed.empty()) {
    return false;
  }
  std::string lowercase = trimmed;
  std::transform(
      lowercase.begin(), lowercase.end(), lowercase.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  // Keep animated color syntax aligned with normal SVG paint parsing.
  auto parse = [](const std::string& color_text, ColorValue* out) -> bool {
    uint32_t rgba = 0;
    if (!parse_svg_color(color_text.c_str(), &rgba)) {
      return false;
    }
    out->r = static_cast<double>((rgba >> 16) & 0xff);
    out->g = static_cast<double>((rgba >> 8) & 0xff);
    out->b = static_cast<double>(rgba & 0xff);
    out->a = static_cast<double>((rgba >> 24) & 0xff);
    return true;
  };
  if (parse(trimmed, color)) {
    return true;
  }
  return lowercase != trimmed && lowercase != "currentcolor" &&
         parse(lowercase, color);
}

std::string FormatColorValue(const ColorValue& color) {
  std::ostringstream stream;
  stream << "rgba(" << ClampByte(color.r) << "," << ClampByte(color.g) << ","
         << ClampByte(color.b) << "," << (ClampByte(color.a) / 255.0) << ")";
  return stream.str();
}

std::string InterpolateColor(const std::string& from, const std::string& to,
                             double progress) {
  ColorValue from_color;
  ColorValue to_color;
  if (!ParseColorValue(from, &from_color) || !ParseColorValue(to, &to_color)) {
    return "";
  }
  return FormatColorValue(
      ColorValue{from_color.r + (to_color.r - from_color.r) * progress,
                 from_color.g + (to_color.g - from_color.g) * progress,
                 from_color.b + (to_color.b - from_color.b) * progress,
                 from_color.a + (to_color.a - from_color.a) * progress});
}

std::string InterpolateScalar(const std::string& from, const std::string& to,
                              double progress) {
  const std::string color = InterpolateColor(from, to, progress);
  if (!color.empty()) {
    return color;
  }
  double from_number = 0.0;
  double to_number = 0.0;
  std::string from_suffix;
  std::string to_suffix;
  if (!ParseNumberWithSuffix(from, &from_number, &from_suffix) ||
      !ParseNumberWithSuffix(to, &to_number, &to_suffix) ||
      from_suffix != to_suffix) {
    return progress < 1.0 ? from : to;
  }
  return FormatNumber(from_number + (to_number - from_number) * progress) +
         from_suffix;
}

std::vector<double> ParseNumberList(const std::string& value) {
  std::vector<double> numbers;
  std::string normalized = value;
  std::replace(normalized.begin(), normalized.end(), ',', ' ');
  std::replace(normalized.begin(), normalized.end(), ';', ' ');
  std::stringstream stream(normalized);
  std::string token;
  while (stream >> token) {
    char* end = nullptr;
    const double parsed = std::strtod(token.c_str(), &end);
    if (end != token.c_str()) {
      numbers.push_back(parsed);
    }
  }
  return numbers;
}

std::string FormatNumberList(const std::vector<double>& numbers) {
  std::ostringstream stream;
  for (size_t i = 0; i < numbers.size(); ++i) {
    if (i != 0) {
      stream << " ";
    }
    stream << numbers[i];
  }
  return stream.str();
}

bool SameSizeNumberLists(const std::string& from, const std::string& to,
                         std::vector<double>* from_numbers,
                         std::vector<double>* to_numbers) {
  if (!from_numbers || !to_numbers) {
    return false;
  }
  *from_numbers = ParseNumberList(from);
  *to_numbers = ParseNumberList(to);
  return !from_numbers->empty() && from_numbers->size() == to_numbers->size();
}

std::string InterpolateNumberList(const std::string& from,
                                  const std::string& to, double progress) {
  std::vector<double> from_numbers;
  std::vector<double> to_numbers;
  if (!SameSizeNumberLists(from, to, &from_numbers, &to_numbers)) {
    return "";
  }
  std::vector<double> result = from_numbers;
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] = from_numbers[i] + (to_numbers[i] - from_numbers[i]) * progress;
  }
  return FormatNumberList(result);
}

uint32_t PathOpArgCount(uint8_t op) {
  switch (op) {
    case SPO_MOVE_TO:
    case SPO_LINE_TO:
      return 2;
    case SPO_CUBIC_BEZ:
      return 6;
    case SPO_QUAD_ARC:
      return 4;
    case SPO_ELLIPTICAL_ARC:
      return 9;
    case SPO_CLOSE:
    default:
      return 0;
  }
}

bool CompatiblePathData(const SrPathData* from, const SrPathData* to) {
  if (!from || !to || from->n_ops != to->n_ops || from->n_args != to->n_args) {
    return false;
  }
  uint32_t arg_index = 0;
  for (uint32_t i = 0; i < from->n_ops; ++i) {
    if (from->ops[i] != to->ops[i]) {
      return false;
    }
    const uint32_t count = PathOpArgCount(from->ops[i]);
    if (arg_index + count > from->n_args || arg_index + count > to->n_args) {
      return false;
    }
    if (from->ops[i] == SPO_ELLIPTICAL_ARC &&
        (std::round(from->args[arg_index + 5]) !=
             std::round(to->args[arg_index + 5]) ||
         std::round(from->args[arg_index + 6]) !=
             std::round(to->args[arg_index + 6]))) {
      return false;
    }
    arg_index += count;
  }
  return arg_index == from->n_args && arg_index == to->n_args;
}

std::string SerializeInterpolatedPath(const SrPathData* from,
                                      const SrPathData* to, double progress) {
  if (!CompatiblePathData(from, to)) {
    return "";
  }
  std::ostringstream stream;
  uint32_t arg_index = 0;
  auto value_at = [&](uint32_t index) {
    return from->args[index] + (to->args[index] - from->args[index]) * progress;
  };
  for (uint32_t i = 0; i < from->n_ops; ++i) {
    if (i != 0) {
      stream << " ";
    }
    switch (from->ops[i]) {
      case SPO_MOVE_TO:
        stream << "M " << value_at(arg_index) << " " << value_at(arg_index + 1);
        break;
      case SPO_LINE_TO:
        stream << "L " << value_at(arg_index) << " " << value_at(arg_index + 1);
        break;
      case SPO_CUBIC_BEZ:
        stream << "C " << value_at(arg_index) << " " << value_at(arg_index + 1)
               << " " << value_at(arg_index + 2) << " "
               << value_at(arg_index + 3) << " " << value_at(arg_index + 4)
               << " " << value_at(arg_index + 5);
        break;
      case SPO_QUAD_ARC:
        stream << "Q " << value_at(arg_index) << " " << value_at(arg_index + 1)
               << " " << value_at(arg_index + 2) << " "
               << value_at(arg_index + 3);
        break;
      case SPO_ELLIPTICAL_ARC:
        stream << "A " << value_at(arg_index + 2) << " "
               << value_at(arg_index + 3) << " " << value_at(arg_index + 4)
               << " " << static_cast<int>(std::round(from->args[arg_index + 5]))
               << " " << static_cast<int>(std::round(from->args[arg_index + 6]))
               << " " << value_at(arg_index + 7) << " "
               << value_at(arg_index + 8);
        break;
      case SPO_CLOSE:
        stream << "Z";
        break;
      default:
        return "";
    }
    arg_index += PathOpArgCount(from->ops[i]);
  }
  return stream.str();
}

bool EnsurePathDataStorage(SrPathData* path, uint32_t n_ops, uint32_t n_args) {
  if (!path) {
    return false;
  }
  if (path->c_ops < n_ops) {
    auto* ops =
        static_cast<uint8_t*>(std::realloc(path->ops, n_ops * sizeof(uint8_t)));
    if (!ops && n_ops > 0) {
      return false;
    }
    path->ops = ops;
    path->c_ops = n_ops;
  }
  if (path->c_args < n_args) {
    auto* args =
        static_cast<float*>(std::realloc(path->args, n_args * sizeof(float)));
    if (!args && n_args > 0) {
      return false;
    }
    path->args = args;
    path->c_args = n_args;
  }
  return true;
}

bool WriteInterpolatedPathData(const SrPathData* from, const SrPathData* to,
                               double progress, SrPathData* out) {
  if (!CompatiblePathData(from, to) ||
      !EnsurePathDataStorage(out, from->n_ops, from->n_args)) {
    return false;
  }
  out->n_ops = from->n_ops;
  out->n_args = from->n_args;
  if (from->n_ops > 0) {
    std::memcpy(out->ops, from->ops, from->n_ops * sizeof(uint8_t));
  }
  uint32_t arg_index = 0;
  for (uint32_t i = 0; i < from->n_ops; ++i) {
    const uint32_t count = PathOpArgCount(from->ops[i]);
    for (uint32_t j = 0; j < count; ++j) {
      const uint32_t index = arg_index + j;
      const bool arc_flag =
          from->ops[i] == SPO_ELLIPTICAL_ARC && (j == 5 || j == 6);
      out->args[index] =
          arc_flag ? from->args[index]
                   : static_cast<float>(from->args[index] +
                                        (to->args[index] - from->args[index]) *
                                            progress);
    }
    arg_index += count;
  }
  return true;
}

std::string InterpolatePathData(const std::string& from, const std::string& to,
                                double progress,
                                const SrSVGDiagnosticSink* diagnostic_sink) {
  SrPathData* from_path = make_serval_path(from.c_str(), diagnostic_sink);
  SrPathData* to_path = make_serval_path(to.c_str(), diagnostic_sink);
  std::string value = SerializeInterpolatedPath(from_path, to_path, progress);
  release_serval_path(from_path);
  release_serval_path(to_path);
  return value;
}

bool IsNumberListAttribute(const std::string& attribute) {
  return attribute == "points" || attribute == "stroke-dasharray" ||
         attribute == "viewBox";
}

double NumericDistance(const std::string& from, const std::string& to) {
  std::vector<double> from_numbers;
  std::vector<double> to_numbers;
  if (SameSizeNumberLists(from, to, &from_numbers, &to_numbers)) {
    double total = 0.0;
    for (size_t i = 0; i < from_numbers.size(); ++i) {
      const double delta = to_numbers[i] - from_numbers[i];
      total += delta * delta;
    }
    return std::sqrt(total);
  }
  double from_number = 0.0;
  double to_number = 0.0;
  std::string from_suffix;
  std::string to_suffix;
  if (ParseNumberWithSuffix(from, &from_number, &from_suffix) &&
      ParseNumberWithSuffix(to, &to_number, &to_suffix) &&
      from_suffix == to_suffix) {
    return std::abs(to_number - from_number);
  }
  return 1.0;
}

std::vector<double> BuildEvenKeyTimes(size_t value_count, bool discrete_mode) {
  std::vector<double> times;
  if (value_count == 0) {
    return times;
  }
  times.resize(value_count);
  const double divisor =
      static_cast<double>(discrete_mode ? value_count : value_count - 1);
  if (divisor <= 0.0) {
    times.front() = 0.0;
    return times;
  }
  for (size_t i = 0; i < value_count; ++i) {
    times[i] = discrete_mode ? static_cast<double>(i) / divisor
                             : static_cast<double>(i) /
                                   static_cast<double>(value_count - 1);
  }
  if (!discrete_mode) {
    times.back() = 1.0;
  }
  return times;
}

std::vector<double> BuildPacedKeyTimes(const std::vector<std::string>& values) {
  std::vector<double> times(values.size(), 0.0);
  if (values.size() < 2) {
    return times;
  }
  double total = 0.0;
  for (size_t i = 1; i < values.size(); ++i) {
    total += NumericDistance(values[i - 1], values[i]);
    times[i] = total;
  }
  if (total <= 0.0) {
    return BuildEvenKeyTimes(values.size(), false);
  }
  for (double& time : times) {
    time /= total;
  }
  times.back() = 1.0;
  return times;
}

double CubicBezierCoordinate(double p1, double p2, double t) {
  const double inv = 1.0 - t;
  return 3.0 * inv * inv * t * p1 + 3.0 * inv * t * t * p2 + t * t * t;
}

double CubicBezierDerivative(double p1, double p2, double t) {
  const double inv = 1.0 - t;
  return 3.0 * inv * inv * p1 + 6.0 * inv * t * (p2 - p1) +
         3.0 * t * t * (1.0 - p2);
}

double ApplyKeySpline(double local_progress,
                      const std::vector<double>& key_splines,
                      size_t segment_index) {
  const size_t offset = segment_index * 4;
  if (offset + 3 >= key_splines.size()) {
    return local_progress;
  }
  const double x1 = key_splines[offset];
  const double y1 = key_splines[offset + 1];
  const double x2 = key_splines[offset + 2];
  const double y2 = key_splines[offset + 3];
  double t = Clamp01(local_progress);
  for (int i = 0; i < 8; ++i) {
    const double x = CubicBezierCoordinate(x1, x2, t) - local_progress;
    const double dx = CubicBezierDerivative(x1, x2, t);
    if (std::abs(x) < 1e-6 || std::abs(dx) < 1e-6) {
      break;
    }
    t = Clamp01(t - x / dx);
  }
  return Clamp01(CubicBezierCoordinate(y1, y2, t));
}

struct KeyedSegment {
  size_t from_index{0};
  size_t to_index{0};
  double local_progress{0.0};
};

KeyedSegment ResolveKeyedSegment(double progress, size_t value_count,
                                 std::vector<double> key_times,
                                 const std::vector<double>& key_splines,
                                 const std::string& calc_mode) {
  KeyedSegment segment;
  if (value_count == 0) {
    return segment;
  }
  const bool discrete = calc_mode == "discrete";
  if (key_times.size() != value_count) {
    key_times = BuildEvenKeyTimes(value_count, discrete);
  }
  if (discrete) {
    if (progress >= 1.0) {
      segment.from_index = value_count - 1;
      segment.to_index = value_count - 1;
      return segment;
    }
    for (size_t i = 0; i < value_count; ++i) {
      const double start = i < key_times.size() ? key_times[i] : 0.0;
      const double end = i + 1 < key_times.size() ? key_times[i + 1] : 1.0;
      if (progress >= start && progress < end) {
        segment.from_index = i;
        segment.to_index = i;
        return segment;
      }
    }
    segment.from_index = value_count - 1;
    segment.to_index = value_count - 1;
    return segment;
  }

  if (value_count == 1 || progress >= 1.0) {
    segment.from_index = value_count - 1;
    segment.to_index = value_count - 1;
    segment.local_progress = 1.0;
    return segment;
  }
  for (size_t i = 0; i + 1 < value_count; ++i) {
    const double start = key_times[i];
    const double end = key_times[i + 1];
    if (progress >= start && progress <= end) {
      segment.from_index = i;
      segment.to_index = i + 1;
      segment.local_progress =
          end > start ? Clamp01((progress - start) / (end - start)) : 0.0;
      if (calc_mode == "spline") {
        segment.local_progress =
            ApplyKeySpline(segment.local_progress, key_splines, i);
      }
      return segment;
    }
  }
  return segment;
}

std::string AddNumericValue(const std::string& base,
                            const std::string& addition, double scale) {
  std::vector<double> base_numbers;
  std::vector<double> addition_numbers;
  if (SameSizeNumberLists(base, addition, &base_numbers, &addition_numbers)) {
    std::vector<double> result = base_numbers;
    for (size_t i = 0; i < result.size(); ++i) {
      result[i] += addition_numbers[i] * scale;
    }
    return FormatNumberList(result);
  }
  double base_number = 0.0;
  double addition_number = 0.0;
  std::string base_suffix;
  std::string addition_suffix;
  if (ParseNumberWithSuffix(base, &base_number, &base_suffix) &&
      ParseNumberWithSuffix(addition, &addition_number, &addition_suffix) &&
      base_suffix == addition_suffix) {
    return FormatNumber(base_number + addition_number * scale) + base_suffix;
  }
  return "";
}

std::string DeltaNumericValue(const std::string& from, const std::string& to) {
  std::vector<double> from_numbers;
  std::vector<double> to_numbers;
  if (SameSizeNumberLists(from, to, &from_numbers, &to_numbers)) {
    std::vector<double> result = to_numbers;
    for (size_t i = 0; i < result.size(); ++i) {
      result[i] -= from_numbers[i];
    }
    return FormatNumberList(result);
  }
  double from_number = 0.0;
  double to_number = 0.0;
  std::string from_suffix;
  std::string to_suffix;
  if (ParseNumberWithSuffix(from, &from_number, &from_suffix) &&
      ParseNumberWithSuffix(to, &to_number, &to_suffix) &&
      from_suffix == to_suffix) {
    return FormatNumber(to_number - from_number) + from_suffix;
  }
  return "";
}

std::string ZeroValueFor(const std::string& reference) {
  ColorValue color;
  if (ParseColorValue(reference, &color)) {
    return "#000000";
  }
  const auto numbers = ParseNumberList(reference);
  if (numbers.size() > 1) {
    return FormatNumberList(std::vector<double>(numbers.size(), 0.0));
  }
  double number = 0.0;
  std::string suffix;
  if (ParseNumberWithSuffix(reference, &number, &suffix)) {
    return "0" + suffix;
  }
  return "";
}

std::string DefaultTransformFromValue(const std::string& type,
                                      const std::vector<double>& reference) {
  if (reference.empty()) {
    return "";
  }
  std::vector<double> numbers(reference.size(), 0.0);
  if (type == "rotate" && reference.size() >= 3) {
    numbers[1] = reference[1];
    numbers[2] = reference[2];
  }
  return FormatNumberList(numbers);
}

std::string AddTransformValue(const std::string& base,
                              const std::string& delta) {
  const auto base_numbers = ParseNumberList(base);
  const auto delta_numbers = ParseNumberList(delta);
  if (base_numbers.empty() || delta_numbers.empty()) {
    return "";
  }
  std::vector<double> result = base_numbers;
  for (size_t i = 0; i < result.size(); ++i) {
    if (i < delta_numbers.size()) {
      result[i] += delta_numbers[i];
    }
  }
  return FormatNumberList(result);
}

std::string ResolveUnderlyingTransformValue(const std::string& type,
                                            const std::string& underlying) {
  const std::string prefix = type + "(";
  const size_t start = underlying.find(prefix);
  if (start == std::string::npos) {
    return "";
  }
  const size_t args_start = start + prefix.size();
  const size_t end = underlying.find(')', args_start);
  if (end == std::string::npos) {
    return "";
  }
  return underlying.substr(args_start, end - args_start);
}

std::string WrapTransform(const std::string& type, const std::string& value) {
  return value.empty() ? "" : type + "(" + value + ")";
}

std::string InterpolateTransformRaw(const std::string& from,
                                    const std::string& to, double progress) {
  const auto from_numbers = ParseNumberList(from);
  const auto to_numbers = ParseNumberList(to);
  if (from_numbers.empty() || from_numbers.size() != to_numbers.size()) {
    return progress < 1.0 ? from : to;
  }
  return InterpolateNumberList(from, to, progress);
}

std::string TransformDelta(const std::string& from, const std::string& to) {
  const auto from_numbers = ParseNumberList(from);
  const auto to_numbers = ParseNumberList(to);
  if (from_numbers.empty() || from_numbers.size() != to_numbers.size()) {
    return "";
  }
  std::vector<double> delta = to_numbers;
  for (size_t i = 0; i < delta.size(); ++i) {
    delta[i] -= from_numbers[i];
  }
  return FormatNumberList(delta);
}

std::string AddTransformRaw(const std::string& base, const std::string& delta,
                            double scale) {
  const auto base_numbers = ParseNumberList(base);
  const auto delta_numbers = ParseNumberList(delta);
  if (base_numbers.empty() || base_numbers.size() != delta_numbers.size()) {
    return "";
  }
  std::vector<double> result = base_numbers;
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] += delta_numbers[i] * scale;
  }
  return FormatNumberList(result);
}

double Distance(const MotionPoint& a, const MotionPoint& b) {
  const double dx = b.x - a.x;
  const double dy = b.y - a.y;
  return std::sqrt(dx * dx + dy * dy);
}

MotionPoint Lerp(const MotionPoint& a, const MotionPoint& b, double t) {
  return MotionPoint{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

MotionPoint QuadPoint(const MotionPoint& p0, const MotionPoint& p1,
                      const MotionPoint& p2, double t) {
  const double mt = 1.0 - t;
  return MotionPoint{mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x,
                     mt * mt * p0.y + 2.0 * mt * t * p1.y + t * t * p2.y};
}

MotionPoint CubicPoint(const MotionPoint& p0, const MotionPoint& p1,
                       const MotionPoint& p2, const MotionPoint& p3, double t) {
  const double mt = 1.0 - t;
  const double mt2 = mt * mt;
  const double t2 = t * t;
  return MotionPoint{mt2 * mt * p0.x + 3.0 * mt2 * t * p1.x +
                         3.0 * mt * t2 * p2.x + t2 * t * p3.x,
                     mt2 * mt * p0.y + 3.0 * mt2 * t * p1.y +
                         3.0 * mt * t2 * p2.y + t2 * t * p3.y};
}

double VectorAngle(double ux, double uy, double vx, double vy) {
  const double dot = ux * vx + uy * vy;
  const double len = std::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
  if (len <= 0.0) {
    return 0.0;
  }
  const double clamped = std::max(-1.0, std::min(1.0, dot / len));
  const double sign = (ux * vy - uy * vx) < 0.0 ? -1.0 : 1.0;
  return sign * std::acos(clamped);
}

void AddArcPolyline(std::vector<MotionPoint>* points, MotionPoint p0, double rx,
                    double ry, double x_axis_rotation, bool large_arc,
                    bool sweep, MotionPoint p1) {
  if (!points) {
    return;
  }
  if (rx == 0.0 || ry == 0.0 || Distance(p0, p1) <= 0.0) {
    points->push_back(p1);
    return;
  }

  rx = std::abs(rx);
  ry = std::abs(ry);
  const double phi = x_axis_rotation * kPi / 180.0;
  const double cos_phi = std::cos(phi);
  const double sin_phi = std::sin(phi);
  const double dx = (p0.x - p1.x) / 2.0;
  const double dy = (p0.y - p1.y) / 2.0;
  const double x1p = cos_phi * dx + sin_phi * dy;
  const double y1p = -sin_phi * dx + cos_phi * dy;

  const double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
  if (lambda > 1.0) {
    const double scale = std::sqrt(lambda);
    rx *= scale;
    ry *= scale;
  }

  const double rx2 = rx * rx;
  const double ry2 = ry * ry;
  const double x1p2 = x1p * x1p;
  const double y1p2 = y1p * y1p;
  const double denom = rx2 * y1p2 + ry2 * x1p2;
  if (denom <= 0.0) {
    points->push_back(p1);
    return;
  }
  double coef =
      std::sqrt(std::max(0.0, (rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2) / denom));
  if (large_arc == sweep) {
    coef = -coef;
  }
  const double cxp = coef * (rx * y1p / ry);
  const double cyp = coef * (-ry * x1p / rx);
  const double cx = cos_phi * cxp - sin_phi * cyp + (p0.x + p1.x) / 2.0;
  const double cy = sin_phi * cxp + cos_phi * cyp + (p0.y + p1.y) / 2.0;

  const double theta1 =
      VectorAngle(1.0, 0.0, (x1p - cxp) / rx, (y1p - cyp) / ry);
  double delta = VectorAngle((x1p - cxp) / rx, (y1p - cyp) / ry,
                             (-x1p - cxp) / rx, (-y1p - cyp) / ry);
  if (!sweep && delta > 0.0) {
    delta -= 2.0 * kPi;
  } else if (sweep && delta < 0.0) {
    delta += 2.0 * kPi;
  }

  const int segments =
      std::max(12, static_cast<int>(std::ceil(std::abs(delta) / (kPi / 24.0))));
  for (int i = 1; i <= segments; ++i) {
    const double t =
        theta1 + delta * static_cast<double>(i) / static_cast<double>(segments);
    const double x =
        cx + rx * std::cos(t) * cos_phi - ry * std::sin(t) * sin_phi;
    const double y =
        cy + rx * std::cos(t) * sin_phi + ry * std::sin(t) * cos_phi;
    points->push_back(MotionPoint{x, y});
  }
}

std::vector<MotionPoint> BuildPolyline(const SrPathData* path) {
  std::vector<MotionPoint> points;
  if (!path || !path->ops || !path->args) {
    return points;
  }
  uint32_t arg_index = 0;
  MotionPoint current;
  MotionPoint sub_path_start;
  bool has_current = false;
  for (uint32_t i = 0; i < path->n_ops; ++i) {
    switch (path->ops[i]) {
      case SPO_MOVE_TO:
        if (arg_index + 1 >= path->n_args) {
          return points;
        }
        current = MotionPoint{path->args[arg_index], path->args[arg_index + 1]};
        arg_index += 2;
        sub_path_start = current;
        points.push_back(current);
        has_current = true;
        break;
      case SPO_LINE_TO: {
        if (arg_index + 1 >= path->n_args) {
          return points;
        }
        current = MotionPoint{path->args[arg_index], path->args[arg_index + 1]};
        arg_index += 2;
        points.push_back(current);
        has_current = true;
        break;
      }
      case SPO_CUBIC_BEZ: {
        if (!has_current || arg_index + 5 >= path->n_args) {
          return points;
        }
        MotionPoint p0 = current;
        MotionPoint p1{path->args[arg_index], path->args[arg_index + 1]};
        MotionPoint p2{path->args[arg_index + 2], path->args[arg_index + 3]};
        MotionPoint p3{path->args[arg_index + 4], path->args[arg_index + 5]};
        arg_index += 6;
        for (int step = 1; step <= 24; ++step) {
          points.push_back(
              CubicPoint(p0, p1, p2, p3, static_cast<double>(step) / 24.0));
        }
        current = p3;
        break;
      }
      case SPO_QUAD_ARC: {
        if (!has_current || arg_index + 3 >= path->n_args) {
          return points;
        }
        MotionPoint p0 = current;
        MotionPoint p1{path->args[arg_index], path->args[arg_index + 1]};
        MotionPoint p2{path->args[arg_index + 2], path->args[arg_index + 3]};
        arg_index += 4;
        for (int step = 1; step <= 20; ++step) {
          points.push_back(
              QuadPoint(p0, p1, p2, static_cast<double>(step) / 20.0));
        }
        current = p2;
        break;
      }
      case SPO_ELLIPTICAL_ARC: {
        if (arg_index + 8 >= path->n_args) {
          return points;
        }
        MotionPoint p0{path->args[arg_index], path->args[arg_index + 1]};
        const double rx = path->args[arg_index + 2];
        const double ry = path->args[arg_index + 3];
        const double x_axis_rotation = path->args[arg_index + 4];
        const bool large_arc = path->args[arg_index + 5] != 0.0f;
        const bool sweep = path->args[arg_index + 6] != 0.0f;
        MotionPoint p1{path->args[arg_index + 7], path->args[arg_index + 8]};
        arg_index += 9;
        AddArcPolyline(&points, p0, rx, ry, x_axis_rotation, large_arc, sweep,
                       p1);
        current = p1;
        has_current = true;
        break;
      }
      case SPO_CLOSE:
        if (has_current) {
          current = sub_path_start;
          points.push_back(current);
        }
        break;
      default:
        break;
    }
  }
  return points;
}

bool SamplePolyline(const std::vector<MotionPoint>& points, double progress,
                    MotionSample* sample) {
  if (!sample || points.empty()) {
    return false;
  }
  if (points.size() == 1) {
    sample->point = points.front();
    sample->angle = 0.0;
    return true;
  }
  double total = 0.0;
  for (size_t i = 1; i < points.size(); ++i) {
    total += Distance(points[i - 1], points[i]);
  }
  if (total <= 0.0) {
    sample->point = points.back();
    sample->angle = 0.0;
    return true;
  }

  const double target = std::max(0.0, std::min(1.0, progress)) * total;
  double walked = 0.0;
  for (size_t i = 1; i < points.size(); ++i) {
    const double segment = Distance(points[i - 1], points[i]);
    if (segment <= 0.0) {
      continue;
    }
    if (walked + segment >= target || i + 1 == points.size()) {
      const double local = (target - walked) / segment;
      sample->point = Lerp(points[i - 1], points[i], local);
      sample->angle = std::atan2(points[i].y - points[i - 1].y,
                                 points[i].x - points[i - 1].x) *
                      180.0 / kPi;
      return true;
    }
    walked += segment;
  }
  sample->point = points.back();
  sample->angle = 0.0;
  return true;
}

uint64_t HashBytes(uint64_t seed, const void* data, size_t size) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; ++i) {
    seed ^= static_cast<uint64_t>(bytes[i]);
    seed *= 1099511628211ull;
  }
  return seed;
}

uint64_t HashPathData(const SrPathData* path) {
  uint64_t hash = 1469598103934665603ull;
  if (!path) {
    return hash;
  }
  hash = HashBytes(hash, &path->n_ops, sizeof(path->n_ops));
  hash = HashBytes(hash, &path->n_args, sizeof(path->n_args));
  if (path->ops && path->n_ops > 0) {
    hash = HashBytes(hash, path->ops, path->n_ops * sizeof(uint8_t));
  }
  if (path->args && path->n_args > 0) {
    hash = HashBytes(hash, path->args, path->n_args * sizeof(float));
  }
  return hash;
}

std::vector<double> FlattenPolylineCoordinates(
    const std::vector<MotionPoint>& points) {
  std::vector<double> coordinates;
  coordinates.reserve(points.size() * 2);
  for (const auto& point : points) {
    coordinates.push_back(point.x);
    coordinates.push_back(point.y);
  }
  return coordinates;
}

void RebuildMotionPathLengths(SrSVGMotionPathCache* cache) {
  if (!cache) {
    return;
  }
  cache->cumulative_lengths.clear();
  cache->total_length = 0.0;
  const size_t point_count = cache->coordinates.size() / 2;
  cache->cumulative_lengths.resize(point_count, 0.0);
  for (size_t i = 1; i < point_count; ++i) {
    const MotionPoint from{cache->coordinates[(i - 1) * 2],
                           cache->coordinates[(i - 1) * 2 + 1]};
    const MotionPoint to{cache->coordinates[i * 2],
                         cache->coordinates[i * 2 + 1]};
    cache->total_length += Distance(from, to);
    cache->cumulative_lengths[i] = cache->total_length;
  }
}

bool SampleMotionPathCache(const SrSVGMotionPathCache& cache, double progress,
                           MotionSample* sample) {
  if (!sample || cache.coordinates.size() < 2) {
    return false;
  }
  const size_t point_count = cache.coordinates.size() / 2;
  if (point_count == 1) {
    sample->point = MotionPoint{cache.coordinates[0], cache.coordinates[1]};
    sample->angle = 0.0;
    return true;
  }
  if (cache.total_length <= 0.0 ||
      cache.cumulative_lengths.size() != point_count) {
    sample->point = MotionPoint{cache.coordinates[(point_count - 1) * 2],
                                cache.coordinates[(point_count - 1) * 2 + 1]};
    sample->angle = 0.0;
    return true;
  }

  const double target = Clamp01(progress) * cache.total_length;
  auto it = std::lower_bound(cache.cumulative_lengths.begin() + 1,
                             cache.cumulative_lengths.end(), target);
  size_t index =
      static_cast<size_t>(std::distance(cache.cumulative_lengths.begin(), it));
  if (index == 0) {
    index = 1;
  } else if (index >= point_count) {
    index = point_count - 1;
  }
  const double previous_length = cache.cumulative_lengths[index - 1];
  const double segment_length =
      cache.cumulative_lengths[index] - previous_length;
  const double local =
      segment_length > 0.0 ? (target - previous_length) / segment_length : 0.0;
  const MotionPoint from{cache.coordinates[(index - 1) * 2],
                         cache.coordinates[(index - 1) * 2 + 1]};
  const MotionPoint to{cache.coordinates[index * 2],
                       cache.coordinates[index * 2 + 1]};
  sample->point = Lerp(from, to, local);
  sample->angle = std::atan2(to.y - from.y, to.x - from.x) * 180.0 / kPi;
  return true;
}

std::string NormalizeHref(const std::string& href) {
  if (href.empty()) {
    return href;
  }
  if (href[0] == '#') {
    return href.substr(1);
  }
  return href;
}

void SortAndUniqueTimes(std::vector<double>* times) {
  if (!times) {
    return;
  }
  times->erase(std::remove_if(times->begin(), times->end(),
                              [](double value) { return std::isnan(value); }),
               times->end());
  std::sort(times->begin(), times->end());
  constexpr double kEpsilon = 1e-9;
  times->erase(std::unique(times->begin(), times->end(),
                           [](double a, double b) {
                             return std::abs(a - b) < kEpsilon ||
                                    (std::isinf(a) && std::isinf(b));
                           }),
               times->end());
}

std::string MotionTransform(const MotionSample& sample,
                            const std::string& rotate) {
  std::ostringstream stream;
  stream << "translate(" << sample.point.x << " " << sample.point.y << ")";
  if (rotate == "auto" || rotate == "auto-reverse") {
    stream << " rotate("
           << sample.angle + (rotate == "auto-reverse" ? 180.0 : 0.0) << ")";
  } else if (!rotate.empty()) {
    char* end = nullptr;
    const double degrees = std::strtod(rotate.c_str(), &end);
    if (end != rotate.c_str()) {
      stream << " rotate(" << degrees << ")";
    }
  }
  return stream.str();
}

std::string EffectiveCalcMode(SrSVGTag tag, const std::string& calc_mode) {
  if (!calc_mode.empty()) {
    return calc_mode;
  }
  return tag == SrSVGTag::kAnimateMotion ? "paced" : "linear";
}

bool IsAnimationTag(SrSVGTag tag) {
  return tag == SrSVGTag::kAnimate || tag == SrSVGTag::kAnimateColor ||
         tag == SrSVGTag::kAnimateMotion ||
         tag == SrSVGTag::kAnimateTransform || tag == SrSVGTag::kSet;
}

std::vector<std::string> BuildMotionValueCandidates(
    const std::vector<std::string>& values, const std::string& from,
    const std::string& to, const std::string& by) {
  if (!values.empty()) {
    return values;
  }
  if (!from.empty() && !to.empty()) {
    return {from, to};
  }
  if (!from.empty() && !by.empty()) {
    const std::string resolved_to = AddNumericValue(from, by, 1.0);
    return resolved_to.empty() ? std::vector<std::string>{}
                               : std::vector<std::string>{from, resolved_to};
  }
  if (!to.empty()) {
    return {"0 0", to};
  }
  if (!by.empty()) {
    return {"0 0", by};
  }
  return {};
}

bool SampleMotionValues(const std::vector<std::string>& values,
                        const std::vector<double>& key_times,
                        const std::vector<double>& key_splines,
                        const std::string& calc_mode, double progress,
                        MotionSample* sample) {
  if (!sample || values.empty()) {
    return false;
  }
  if (values.size() == 1) {
    const auto point = ParseNumberList(values.front());
    if (point.size() < 2) {
      return false;
    }
    sample->point = MotionPoint{point[0], point[1]};
    sample->angle = 0.0;
    return true;
  }
  std::vector<double> effective_key_times = key_times;
  if (calc_mode == "paced") {
    effective_key_times = BuildPacedKeyTimes(values);
  }
  const KeyedSegment segment = ResolveKeyedSegment(
      progress, values.size(), effective_key_times, key_splines, calc_mode);
  const auto from = ParseNumberList(values[segment.from_index]);
  const auto to = ParseNumberList(values[segment.to_index]);
  if (from.size() < 2 || to.size() < 2) {
    return false;
  }
  sample->point =
      MotionPoint{from[0] + (to[0] - from[0]) * segment.local_progress,
                  from[1] + (to[1] - from[1]) * segment.local_progress};
  sample->angle = std::atan2(to[1] - from[1], to[0] - from[0]) * 180.0 / kPi;
  return true;
}

}  // namespace

SrSVGAnimation::~SrSVGAnimation() {
  ResetCaches();
}

void SrSVGAnimation::ResetMotionPathCache() const {
  motion_path_cache_ = SrSVGMotionPathCache{};
}

void SrSVGAnimation::ResetPathDataCaches() const {
  for (auto& cache : path_pair_caches_) {
    release_serval_path(cache.from_path);
    release_serval_path(cache.to_path);
    cache.from_path = nullptr;
    cache.to_path = nullptr;
  }
  path_pair_caches_.clear();
  release_serval_path(interpolated_path_cache_);
  interpolated_path_cache_ = nullptr;
}

void SrSVGAnimation::ResetCaches() const {
  ResetMotionPathCache();
  ResetPathDataCaches();
}

bool SrSVGAnimation::EnsureMotionPathCacheForPathString(
    const std::string& path) const {
  if (path.empty()) {
    return false;
  }
  if (motion_path_cache_.source_path == nullptr &&
      motion_path_cache_.source_key == path) {
    return !motion_path_cache_.coordinates.empty();
  }

  ResetMotionPathCache();
  motion_path_cache_.source_key = path;
  SrPathData* path_data = make_serval_path(path.c_str(), diagnostic_sink_);
  if (!path_data) {
    return false;
  }
  motion_path_cache_.coordinates =
      FlattenPolylineCoordinates(BuildPolyline(path_data));
  release_serval_path(path_data);
  RebuildMotionPathLengths(&motion_path_cache_);
  return !motion_path_cache_.coordinates.empty();
}

bool SrSVGAnimation::EnsureMotionPathCacheForPathData(
    const std::string& source_key, const SrPathData* path_data) const {
  if (!path_data) {
    return false;
  }
  const uint64_t hash = HashPathData(path_data);
  if (motion_path_cache_.source_key == source_key &&
      motion_path_cache_.source_path == path_data &&
      motion_path_cache_.n_ops == path_data->n_ops &&
      motion_path_cache_.n_args == path_data->n_args &&
      motion_path_cache_.hash == hash) {
    return !motion_path_cache_.coordinates.empty();
  }

  ResetMotionPathCache();
  motion_path_cache_.source_key = source_key;
  motion_path_cache_.source_path = path_data;
  motion_path_cache_.n_ops = path_data->n_ops;
  motion_path_cache_.n_args = path_data->n_args;
  motion_path_cache_.hash = hash;
  motion_path_cache_.coordinates =
      FlattenPolylineCoordinates(BuildPolyline(path_data));
  RebuildMotionPathLengths(&motion_path_cache_);
  return !motion_path_cache_.coordinates.empty();
}

const SrSVGPathPairCache* SrSVGAnimation::EnsurePathPairCache(
    const std::string& from, const std::string& to) const {
  for (const auto& cache : path_pair_caches_) {
    if (cache.from == from && cache.to == to) {
      return &cache;
    }
  }

  SrSVGPathPairCache cache;
  cache.from = from;
  cache.to = to;
  cache.from_path = make_serval_path(from.c_str(), diagnostic_sink_);
  cache.to_path = make_serval_path(to.c_str(), diagnostic_sink_);
  cache.compatible = CompatiblePathData(cache.from_path, cache.to_path);
  path_pair_caches_.push_back(std::move(cache));
  return &path_pair_caches_.back();
}

const SrPathData* SrSVGAnimation::InterpolateCachedPathData(
    const std::string& from, const std::string& to, double progress) const {
  const auto* cache = EnsurePathPairCache(from, to);
  if (!cache || !cache->compatible) {
    return nullptr;
  }
  if (!interpolated_path_cache_) {
    interpolated_path_cache_ =
        static_cast<SrPathData*>(std::calloc(1, sizeof(SrPathData)));
    if (!interpolated_path_cache_) {
      return nullptr;
    }
  }
  if (!WriteInterpolatedPathData(cache->from_path, cache->to_path, progress,
                                 interpolated_path_cache_)) {
    return nullptr;
  }
  return interpolated_path_cache_;
}

const SrPathData* SrSVGAnimation::MakePathDataValue(
    const ActiveState& state, const std::string& underlying,
    std::string* fallback_value) const {
  if (fallback_value) {
    fallback_value->clear();
  }

  std::vector<std::string> candidates = values_;
  if (candidates.empty()) {
    if (!from_.empty() && !to_.empty()) {
      candidates = {from_, to_};
    } else if (!to_.empty()) {
      candidates = {underlying.empty() ? to_ : underlying, to_};
    } else if (!from_.empty()) {
      if (fallback_value) {
        *fallback_value = from_;
      }
      return InterpolateCachedPathData(from_, from_, 0.0);
    }
  }
  if (candidates.empty()) {
    return nullptr;
  }

  std::vector<double> key_times = key_times_;
  const std::string calc_mode = EffectiveCalcMode(Tag(), calc_mode_);
  if (calc_mode == "paced") {
    key_times = BuildPacedKeyTimes(candidates);
  }
  const KeyedSegment segment = ResolveKeyedSegment(
      state.progress, candidates.size(), key_times, key_splines_, calc_mode);
  const std::string& from = candidates[segment.from_index];
  const std::string& to = candidates[segment.to_index];
  const SrPathData* path_data =
      InterpolateCachedPathData(from, to, segment.local_progress);
  if (!path_data && fallback_value) {
    *fallback_value = segment.local_progress < 1.0
                          ? candidates[segment.from_index]
                          : candidates[segment.to_index];
  }
  return path_data;
}

bool SrSVGAnimation::ParseAndSetAttribute(const char* name, const char* value) {
  ResetCaches();
  if (std::strcmp(name, "attributeName") == 0) {
    attribute_name_ = value;
  } else if (std::strcmp(name, "href") == 0 ||
             std::strcmp(name, "xlink:href") == 0) {
    if (Tag() == SrSVGTag::kMPath) {
      path_href_ = value;
    } else {
      target_href_ = value;
    }
  } else if (std::strcmp(name, "from") == 0) {
    from_ = value;
  } else if (std::strcmp(name, "to") == 0) {
    to_ = value;
  } else if (std::strcmp(name, "by") == 0) {
    by_ = value;
  } else if (std::strcmp(name, "values") == 0) {
    values_ = Split(value, ';');
  } else if (std::strcmp(name, "keyTimes") == 0) {
    key_times_ = ParseSemicolonNumberList(value);
  } else if (std::strcmp(name, "keySplines") == 0) {
    key_splines_ = ParseNumberList(value);
  } else if (std::strcmp(name, "keyPoints") == 0) {
    key_points_ = ParseSemicolonNumberList(value);
  } else if (std::strcmp(name, "calcMode") == 0) {
    calc_mode_ = value;
  } else if (std::strcmp(name, "type") == 0) {
    transform_type_ = value;
  } else if (std::strcmp(name, "rotate") == 0) {
    rotate_ = value;
  } else if (std::strcmp(name, "path") == 0) {
    path_ = value;
  } else if (std::strcmp(name, "begin") == 0) {
    ParseBegin(value);
  } else if (std::strcmp(name, "dur") == 0) {
    dur_indefinite_ = std::strcmp(value, "indefinite") == 0;
    const double parsed = ParseClockValue(value);
    dur_ = std::isnan(parsed) ? 0.0 : parsed;
  } else if (std::strcmp(name, "end") == 0) {
    ParseEnd(value);
  } else if (std::strcmp(name, "repeatDur") == 0) {
    repeat_dur_indefinite_ = std::strcmp(value, "indefinite") == 0;
    const double parsed = ParseClockValue(value);
    if (!std::isnan(parsed)) {
      repeat_dur_ = parsed;
      has_repeat_dur_ = true;
    }
  } else if (std::strcmp(name, "min") == 0) {
    const double parsed = ParseClockValue(value);
    if (!std::isnan(parsed)) {
      min_ = parsed;
      has_min_ = true;
    }
  } else if (std::strcmp(name, "max") == 0) {
    const double parsed = ParseClockValue(value);
    if (!std::isnan(parsed)) {
      max_ = parsed;
      has_max_ = true;
    }
  } else if (std::strcmp(name, "repeatCount") == 0) {
    repeat_indefinite_ = std::strcmp(value, "indefinite") == 0;
    repeat_count_ = repeat_indefinite_ ? 0.0 : std::strtod(value, nullptr);
  } else if (std::strcmp(name, "fill") == 0) {
    freeze_ = std::strcmp(value, "freeze") == 0;
  } else if (std::strcmp(name, "restart") == 0) {
    if (std::strcmp(value, "never") == 0) {
      restart_ = Restart::kNever;
    } else if (std::strcmp(value, "whenNotActive") == 0) {
      restart_ = Restart::kWhenNotActive;
    } else {
      restart_ = Restart::kAlways;
    }
  } else if (std::strcmp(name, "additive") == 0) {
    additive_sum_ = std::strcmp(value, "sum") == 0;
  } else if (std::strcmp(name, "accumulate") == 0) {
    accumulate_sum_ = std::strcmp(value, "sum") == 0;
  }
  return true;
}

void SrSVGAnimation::AppendChild(SrSVGNodeBase* child) {
  if (!child || Tag() != SrSVGTag::kAnimateMotion ||
      child->Tag() != SrSVGTag::kMPath) {
    return;
  }
  const auto* mpath = static_cast<const SrSVGAnimation*>(child);
  ResetCaches();
  path_href_ = mpath->path_href_;
}

void SrSVGAnimation::ParseBegin(const char* value) {
  has_begin_ = true;
  begin_specs_.clear();
  begin_ = std::numeric_limits<double>::infinity();
  ParseTimeSpecList(value, &begin_specs_, &begin_);
}

void SrSVGAnimation::ParseEnd(const char* value) {
  has_end_ = true;
  end_specs_.clear();
  end_ = std::numeric_limits<double>::infinity();
  ParseTimeSpecList(value, &end_specs_, &end_);
}

void SrSVGAnimation::ParseTimeSpecList(const char* value,
                                       std::vector<BeginSpec>* specs,
                                       double* earliest_static) const {
  if (!specs || !earliest_static) {
    return;
  }
  if (!value) {
    return;
  }
  for (const auto& raw_part : Split(value, ';')) {
    const std::string part = RemoveSpaces(raw_part);
    if (part.empty() || part == "indefinite") {
      continue;
    }
    const double static_time = ParsePlainClockValue(part);
    if (!std::isnan(static_time)) {
      specs->push_back(BeginSpec{BeginType::kStatic, "", static_time});
      *earliest_static = std::min(*earliest_static, static_time);
      continue;
    }

    BeginType type = BeginType::kStatic;
    size_t marker = part.find(".begin");
    size_t marker_len = 6;
    if (marker != std::string::npos) {
      type = BeginType::kSyncBegin;
    } else {
      marker = part.find(".end");
      marker_len = 4;
      if (marker != std::string::npos) {
        type = BeginType::kSyncEnd;
      }
    }
    if (marker == std::string::npos || marker == 0) {
      continue;
    }
    BeginSpec spec;
    spec.type = type;
    spec.sync_id = part.substr(0, marker);
    const std::string offset = part.substr(marker + marker_len);
    if (!offset.empty()) {
      const char sign = offset[0];
      if (sign != '+' && sign != '-') {
        continue;
      }
      const double parsed = ParsePlainClockValue(offset.substr(1));
      if (std::isnan(parsed)) {
        continue;
      }
      spec.offset = sign == '-' ? -parsed : parsed;
    }
    specs->push_back(std::move(spec));
  }
}

bool SrSVGAnimation::Evaluate(double seconds, const IDMapper* id_mapper,
                              const std::string& underlying,
                              Effect* effect) const {
  if (!effect || Tag() == SrSVGTag::kMPath) {
    return false;
  }
  ActiveState state;
  if (!ActiveStateAt(seconds, id_mapper, &state)) {
    return false;
  }

  if (Tag() == SrSVGTag::kAnimateMotion) {
    MotionSample sample;
    bool has_cached_path = false;
    const bool has_mpath = !path_href_.empty();
    const bool has_path = !path_.empty();
    if (has_mpath && id_mapper) {
      const auto it = id_mapper->find(NormalizeHref(path_href_));
      if (it != id_mapper->end() && it->second &&
          it->second->Tag() == SrSVGTag::kPath) {
        const auto* path = static_cast<const SrSVGPath*>(it->second);
        has_cached_path = EnsureMotionPathCacheForPathData(
            NormalizeHref(path_href_), path->path_data());
      }
    } else if (has_path) {
      has_cached_path = EnsureMotionPathCacheForPathString(path_);
    }
    if (has_cached_path) {
      double path_progress = state.progress;
      if (key_points_.size() > 1) {
        std::vector<std::string> key_point_values;
        key_point_values.reserve(key_points_.size());
        for (const double point : key_points_) {
          key_point_values.push_back(FormatNumber(point));
        }
        const KeyedSegment segment = ResolveKeyedSegment(
            state.progress, key_point_values.size(), key_times_, key_splines_,
            EffectiveCalcMode(Tag(), calc_mode_));
        path_progress =
            std::strtod(key_point_values[segment.from_index].c_str(), nullptr);
        if (segment.from_index != segment.to_index) {
          const double from = std::strtod(
              key_point_values[segment.from_index].c_str(), nullptr);
          const double to =
              std::strtod(key_point_values[segment.to_index].c_str(), nullptr);
          path_progress = from + (to - from) * segment.local_progress;
        }
      }
      if (!SampleMotionPathCache(motion_path_cache_, path_progress, &sample)) {
        return false;
      }
    } else if (has_mpath || has_path) {
      return false;
    } else {
      const auto motion_values =
          BuildMotionValueCandidates(values_, from_, to_, by_);
      if (!SampleMotionValues(motion_values, key_times_, key_splines_,
                              EffectiveCalcMode(Tag(), calc_mode_),
                              state.progress, &sample)) {
        return false;
      }
    }
    effect->attribute = "transform";
    effect->value = MotionTransform(sample, rotate_);
    effect->additive = true;
    effect->transform = true;
    effect->motion = true;
    return true;
  }

  effect->attribute =
      Tag() == SrSVGTag::kAnimateTransform
          ? (attribute_name_.empty() ? "transform" : attribute_name_)
          : attribute_name_;
  if (effect->attribute.empty()) {
    return false;
  }
  if (effect->attribute == "d" && Tag() != SrSVGTag::kSet) {
    effect->path_data = MakePathDataValue(state, underlying, &effect->value);
  } else {
    effect->value = MakeValue(state, underlying);
  }
  effect->additive = additive_sum_ || (!by_.empty() && from_.empty());
  effect->transform = Tag() == SrSVGTag::kAnimateTransform;
  effect->motion = false;
  return effect->path_data || !effect->value.empty();
}

std::string SrSVGAnimation::MakeValue(const ActiveState& state,
                                      const std::string& underlying) const {
  if (Tag() == SrSVGTag::kSet) {
    if (!to_.empty()) {
      return to_;
    }
    if (!values_.empty()) {
      return values_.back();
    }
    return "";
  }

  const std::string calc_mode = EffectiveCalcMode(Tag(), calc_mode_);
  if (Tag() == SrSVGTag::kAnimateTransform) {
    const std::string type =
        transform_type_.empty() ? "translate" : transform_type_;
    std::vector<std::string> candidates = values_;
    if (candidates.empty()) {
      if (!from_.empty() && !to_.empty()) {
        candidates = {from_, to_};
      } else if (!from_.empty() && !by_.empty()) {
        const std::string to = AddTransformValue(from_, by_);
        if (!to.empty()) {
          candidates = {from_, to};
        }
      } else if (!to_.empty()) {
        std::string from = ResolveUnderlyingTransformValue(type, underlying);
        if (from.empty()) {
          from = DefaultTransformFromValue(type, ParseNumberList(to_));
        }
        if (!from.empty()) {
          candidates = {from, to_};
        }
      } else if (!by_.empty()) {
        const std::string from =
            DefaultTransformFromValue(type, ParseNumberList(by_));
        const std::string to = AddTransformValue(from, by_);
        if (!from.empty() && !to.empty()) {
          candidates = {from, to};
        }
      } else if (!from_.empty()) {
        return WrapTransform(type, from_);
      }
    }
    if (candidates.empty()) {
      return "";
    }
    std::vector<double> key_times = key_times_;
    if (calc_mode == "paced") {
      key_times = BuildPacedKeyTimes(candidates);
    }
    const KeyedSegment segment = ResolveKeyedSegment(
        state.progress, candidates.size(), key_times, key_splines_, calc_mode);
    std::string raw = candidates[segment.from_index];
    if (segment.from_index != segment.to_index) {
      raw = InterpolateTransformRaw(candidates[segment.from_index],
                                    candidates[segment.to_index],
                                    segment.local_progress);
    }
    if (accumulate_sum_ && state.repeat_index > 0 && candidates.size() > 1) {
      const std::string delta =
          TransformDelta(candidates.front(), candidates.back());
      const std::string accumulated =
          AddTransformRaw(raw, delta, static_cast<double>(state.repeat_index));
      if (!accumulated.empty()) {
        raw = accumulated;
      }
    }
    return WrapTransform(type, raw);
  }

  std::vector<std::string> candidates = values_;
  if (candidates.empty()) {
    if (!from_.empty() && !to_.empty()) {
      candidates = {from_, to_};
    } else if (!from_.empty() && !by_.empty()) {
      const std::string to = AddNumericValue(from_, by_, 1.0);
      if (!to.empty()) {
        candidates = {from_, to};
      }
    } else if (!to_.empty()) {
      candidates = {underlying.empty() ? to_ : underlying, to_};
    } else if (!by_.empty()) {
      const std::string zero = ZeroValueFor(by_);
      if (!zero.empty()) {
        candidates = {zero, by_};
      }
    } else if (!from_.empty()) {
      return from_;
    }
  }
  if (candidates.empty()) {
    return "";
  }
  std::vector<double> key_times = key_times_;
  if (calc_mode == "paced") {
    key_times = BuildPacedKeyTimes(candidates);
  }
  const KeyedSegment segment = ResolveKeyedSegment(
      state.progress, candidates.size(), key_times, key_splines_, calc_mode);
  std::string value = candidates[segment.from_index];
  if (segment.from_index != segment.to_index) {
    if (attribute_name_ == "d") {
      value = InterpolatePathData(candidates[segment.from_index],
                                  candidates[segment.to_index],
                                  segment.local_progress, diagnostic_sink_);
      if (value.empty()) {
        value = segment.local_progress < 1.0 ? candidates[segment.from_index]
                                             : candidates[segment.to_index];
      }
    } else if (IsNumberListAttribute(attribute_name_)) {
      value = InterpolateNumberList(candidates[segment.from_index],
                                    candidates[segment.to_index],
                                    segment.local_progress);
      if (value.empty()) {
        value = segment.local_progress < 1.0 ? candidates[segment.from_index]
                                             : candidates[segment.to_index];
      }
    } else {
      value = InterpolateScalar(candidates[segment.from_index],
                                candidates[segment.to_index],
                                segment.local_progress);
    }
  }
  if (attribute_name_ != "d" && accumulate_sum_ && state.repeat_index > 0 &&
      candidates.size() > 1) {
    std::string delta =
        DeltaNumericValue(candidates.front(), candidates.back());
    if (delta.empty() && !by_.empty()) {
      delta = by_;
    }
    const std::string accumulated =
        AddNumericValue(value, delta, static_cast<double>(state.repeat_index));
    if (!accumulated.empty()) {
      value = accumulated;
    }
  }
  return value;
}

std::vector<double> SrSVGAnimation::ResolvedTimeSpecSeconds(
    const std::vector<BeginSpec>& specs, const IDMapper* id_mapper,
    int depth) const {
  std::vector<double> resolved;
  if (depth > 8) {
    return resolved;
  }
  for (const auto& spec : specs) {
    if (spec.type == BeginType::kStatic) {
      resolved.push_back(spec.offset);
      continue;
    }
    if (!id_mapper || spec.sync_id.empty()) {
      continue;
    }
    const auto it = id_mapper->find(spec.sync_id);
    if (it == id_mapper->end() || !it->second ||
        !IsAnimationTag(it->second->Tag())) {
      continue;
    }
    const auto* sync_animation = static_cast<const SrSVGAnimation*>(it->second);
    const auto sync_begins =
        sync_animation->ResolvedBeginSecondsList(id_mapper, depth + 1);
    for (const double sync_begin : sync_begins) {
      if (std::isinf(sync_begin)) {
        continue;
      }
      double sync_time = sync_begin;
      if (spec.type == BeginType::kSyncEnd) {
        const double sync_duration =
            sync_animation->ResolvedActiveDurationForBegin(
                sync_begin, id_mapper, depth + 1);
        if (std::isinf(sync_duration)) {
          continue;
        }
        sync_time += sync_duration;
      }
      resolved.push_back(sync_time + spec.offset);
    }
  }
  SortAndUniqueTimes(&resolved);
  return resolved;
}

std::vector<double> SrSVGAnimation::ResolvedBeginSecondsList(
    const IDMapper* id_mapper, int depth) const {
  if (depth > 8) {
    return {};
  }
  if (begin_specs_.empty()) {
    return has_begin_ ? std::vector<double>{} : std::vector<double>{begin_};
  }
  return ResolvedTimeSpecSeconds(begin_specs_, id_mapper, depth);
}

std::vector<double> SrSVGAnimation::ResolvedEndSecondsList(
    const IDMapper* id_mapper, int depth) const {
  if (!has_end_ || depth > 8 || end_specs_.empty()) {
    return {};
  }
  return ResolvedTimeSpecSeconds(end_specs_, id_mapper, depth);
}

double SrSVGAnimation::ResolvedBeginSeconds(const IDMapper* id_mapper,
                                            int depth) const {
  const auto begins = ResolvedBeginSecondsList(id_mapper, depth);
  return begins.empty() ? std::numeric_limits<double>::infinity()
                        : begins.front();
}

double SrSVGAnimation::ResolvedActiveDurationForBegin(double begin,
                                                      const IDMapper* id_mapper,
                                                      int depth) const {
  if (dur_indefinite_) {
    return std::numeric_limits<double>::infinity();
  }
  if (dur_ <= 0.0) {
    return Tag() == SrSVGTag::kSet ? std::numeric_limits<double>::infinity()
                                   : 0.0;
  }

  double active_duration = repeat_indefinite_
                               ? std::numeric_limits<double>::infinity()
                               : dur_ * std::max(0.0, repeat_count_);
  if (active_duration <= 0.0) {
    active_duration = dur_;
  }
  if (has_repeat_dur_) {
    active_duration = std::min(active_duration, repeat_dur_);
  } else if (repeat_dur_indefinite_) {
    active_duration = std::numeric_limits<double>::infinity();
  }
  if (has_end_) {
    const auto ends = ResolvedEndSecondsList(id_mapper, depth + 1);
    constexpr double kEpsilon = 1e-9;
    for (const double end : ends) {
      if (end > begin + kEpsilon) {
        active_duration = std::min(active_duration, end - begin);
        break;
      }
    }
  }
  if (has_min_) {
    active_duration = std::max(active_duration, min_);
  }
  if (has_max_) {
    active_duration = std::min(active_duration, max_);
  }
  return active_duration;
}

double SrSVGAnimation::ResolvedActiveDuration(const IDMapper* id_mapper,
                                              int depth) const {
  const double begin = ResolvedBeginSeconds(id_mapper, depth + 1);
  if (std::isinf(begin)) {
    return std::numeric_limits<double>::infinity();
  }
  return ResolvedActiveDurationForBegin(begin, id_mapper, depth);
}

double SrSVGAnimation::LastChangeSeconds(const IDMapper* id_mapper) const {
  return ResolvedLastChangeSeconds(id_mapper, 0);
}

double SrSVGAnimation::ResolvedLastChangeSeconds(const IDMapper* id_mapper,
                                                 int depth) const {
  if (Tag() == SrSVGTag::kMPath || depth > 8) {
    return 0.0;
  }
  auto begins = ResolvedBeginSecondsList(id_mapper, depth + 1);
  if (begins.empty()) {
    return 0.0;
  }
  SortAndUniqueTimes(&begins);

  bool accepted_once = false;
  double accepted_end = -std::numeric_limits<double>::infinity();
  double last_change = 0.0;
  constexpr double kEpsilon = 1e-9;
  for (const double candidate_begin : begins) {
    if (std::isinf(candidate_begin)) {
      continue;
    }
    if (restart_ == Restart::kNever && accepted_once) {
      break;
    }
    if (restart_ == Restart::kWhenNotActive && accepted_once &&
        candidate_begin < accepted_end - kEpsilon) {
      continue;
    }

    double candidate_end = candidate_begin;
    if (Tag() == SrSVGTag::kSet &&
        (dur_indefinite_ || (dur_ <= 0.0 && !has_end_))) {
      candidate_end = candidate_begin;
    } else if (dur_indefinite_) {
      return std::numeric_limits<double>::infinity();
    } else if (dur_ <= 0.0) {
      continue;
    } else {
      const double candidate_duration =
          ResolvedActiveDurationForBegin(candidate_begin, id_mapper, depth + 1);
      if (std::isinf(candidate_duration)) {
        return std::numeric_limits<double>::infinity();
      }
      if (candidate_duration <= 0.0) {
        continue;
      }
      candidate_end = candidate_begin + candidate_duration;
    }

    accepted_once = true;
    accepted_end = candidate_end;
    last_change = std::max(last_change, candidate_end);
  }
  return last_change > 0.0 ? last_change : 0.0;
}

bool SrSVGAnimation::ActiveStateAt(double seconds, const IDMapper* id_mapper,
                                   ActiveState* state) const {
  if (!state) {
    return false;
  }
  auto begins = ResolvedBeginSecondsList(id_mapper, 0);
  if (begins.empty()) {
    return false;
  }
  SortAndUniqueTimes(&begins);
  if (Tag() == SrSVGTag::kSet && dur_ <= 0.0 && !has_end_) {
    const auto first_begin = begins.front();
    if (seconds < first_begin) {
      return false;
    }
    state->progress = 1.0;
    return true;
  }
  if (dur_indefinite_) {
    const auto first_begin = begins.front();
    if (seconds < first_begin) {
      return false;
    }
    state->progress = Tag() == SrSVGTag::kSet ? 1.0 : 0.0;
    return Tag() == SrSVGTag::kSet;
  }
  if (dur_ <= 0.0) {
    return false;
  }

  bool has_interval = false;
  double selected_begin = 0.0;
  double selected_duration = 0.0;
  double accepted_end = -std::numeric_limits<double>::infinity();
  bool accepted_once = false;
  constexpr double kEpsilon = 1e-9;
  for (const double candidate_begin : begins) {
    if (candidate_begin > seconds + kEpsilon) {
      break;
    }
    const double candidate_duration =
        ResolvedActiveDurationForBegin(candidate_begin, id_mapper, 0);
    if (candidate_duration <= 0.0) {
      continue;
    }
    if (restart_ == Restart::kNever && accepted_once) {
      break;
    }
    if (restart_ == Restart::kWhenNotActive && accepted_once &&
        candidate_begin < accepted_end - kEpsilon) {
      continue;
    }
    accepted_once = true;
    accepted_end = std::isinf(candidate_duration)
                       ? std::numeric_limits<double>::infinity()
                       : candidate_begin + candidate_duration;
    selected_begin = candidate_begin;
    selected_duration = candidate_duration;
    has_interval = true;
    if (restart_ == Restart::kNever) {
      break;
    }
  }
  if (!has_interval || seconds < selected_begin) {
    return false;
  }

  const double elapsed = seconds - selected_begin;
  const double active_duration = selected_duration;
  if (elapsed >= active_duration) {
    if (!freeze_) {
      return false;
    }
    const double frozen_elapsed = std::max(0.0, active_duration);
    state->repeat_index = static_cast<uint64_t>(
        std::max(0.0, std::ceil(frozen_elapsed / dur_) - 1.0));
    const double iteration_time = std::fmod(frozen_elapsed, dur_);
    state->progress =
        iteration_time == 0.0 ? 1.0 : Clamp01(iteration_time / dur_);
    state->frozen = true;
    return true;
  }
  const double iteration_time = std::fmod(elapsed, dur_);
  state->repeat_index =
      static_cast<uint64_t>(std::max(0.0, std::floor(elapsed / dur_)));
  state->progress = Clamp01(iteration_time / dur_);
  state->frozen = false;
  return true;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
