// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGAnimation.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace serval {
namespace svg {
namespace element {

namespace {

std::vector<std::string> Split(const std::string& input, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  std::stringstream stream(input);
  while (std::getline(stream, current, delimiter)) {
    if (!current.empty()) {
      parts.push_back(current);
    }
  }
  return parts;
}

double ParseClockValue(const char* value) {
  if (!value || value[0] == '\0') {
    return 0.0;
  }
  char* end = nullptr;
  const double number = std::strtod(value, &end);
  if (!end || *end == '\0' || std::strcmp(end, "s") == 0) {
    return number;
  }
  if (std::strcmp(end, "ms") == 0) {
    return number / 1000.0;
  }
  if (std::strcmp(end, "min") == 0) {
    return number * 60.0;
  }
  return number;
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

std::string InterpolateScalar(const std::string& from, const std::string& to,
                              double progress) {
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

std::string InterpolateTransform(const std::string& type,
                                 const std::string& from,
                                 const std::string& to, double progress) {
  const auto from_numbers = ParseNumberList(from);
  const auto to_numbers = ParseNumberList(to);
  if (from_numbers.empty() || from_numbers.size() != to_numbers.size()) {
    return type + "(" + (progress < 1.0 ? from : to) + ")";
  }

  std::ostringstream stream;
  stream << type << "(";
  for (size_t i = 0; i < from_numbers.size(); ++i) {
    if (i != 0) {
      stream << " ";
    }
    stream << from_numbers[i] + (to_numbers[i] - from_numbers[i]) * progress;
  }
  stream << ")";
  return stream.str();
}

}  // namespace

bool SrSVGAnimation::ParseAndSetAttribute(const char* name, const char* value) {
  if (std::strcmp(name, "attributeName") == 0) {
    attribute_name_ = value;
  } else if (std::strcmp(name, "from") == 0) {
    from_ = value;
  } else if (std::strcmp(name, "to") == 0) {
    to_ = value;
  } else if (std::strcmp(name, "by") == 0) {
    by_ = value;
  } else if (std::strcmp(name, "values") == 0) {
    values_ = Split(value, ';');
  } else if (std::strcmp(name, "type") == 0) {
    transform_type_ = value;
  } else if (std::strcmp(name, "begin") == 0) {
    begin_ = ParseClockValue(value);
  } else if (std::strcmp(name, "dur") == 0) {
    dur_ = ParseClockValue(value);
  } else if (std::strcmp(name, "repeatCount") == 0) {
    repeat_indefinite_ = std::strcmp(value, "indefinite") == 0;
    repeat_count_ = repeat_indefinite_ ? 0.0 : std::strtod(value, nullptr);
  } else if (std::strcmp(name, "fill") == 0) {
    freeze_ = std::strcmp(value, "freeze") == 0;
  }
  return true;
}

bool SrSVGAnimation::Evaluate(double seconds, std::string* attribute,
                              std::string* value) const {
  if (!attribute || !value || dur_ <= 0.0) {
    return false;
  }
  if (Tag() == SrSVGTag::kAnimateTransform) {
    *attribute = "transform";
  } else {
    *attribute = attribute_name_;
  }
  if (attribute->empty()) {
    return false;
  }
  *value = MakeValue(seconds);
  return !value->empty();
}

std::string SrSVGAnimation::MakeValue(double seconds) const {
  const double progress = ActiveProgress(seconds);
  if (std::isnan(progress)) {
    return "";
  }

  if (values_.size() == 1) {
    return values_.front();
  }
  if (values_.size() > 1) {
    const double scaled = progress * static_cast<double>(values_.size() - 1);
    const size_t index = static_cast<size_t>(
        std::min<double>(std::floor(scaled), values_.size() - 2));
    const double local_progress = scaled - static_cast<double>(index);
    if (Tag() == SrSVGTag::kAnimateTransform) {
      const std::string type = transform_type_.empty() ? "translate"
                                                       : transform_type_;
      return InterpolateTransform(type, values_[index], values_[index + 1],
                                  local_progress);
    }
    return InterpolateScalar(values_[index], values_[index + 1],
                             local_progress);
  }

  if (Tag() == SrSVGTag::kAnimateTransform) {
    const std::string type = transform_type_.empty() ? "translate"
                                                     : transform_type_;
    return InterpolateTransform(type, from_, to_, progress);
  }
  if (!from_.empty() && !to_.empty()) {
    return InterpolateScalar(from_, to_, progress);
  }
  if (!to_.empty()) {
    return to_;
  }
  if (!from_.empty()) {
    return from_;
  }
  return "";
}

double SrSVGAnimation::ActiveProgress(double seconds) const {
  if (seconds < begin_) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double elapsed = seconds - begin_;
  const double active_duration =
      repeat_indefinite_ ? dur_ : dur_ * std::max(1.0, repeat_count_);
  if (!repeat_indefinite_ && elapsed >= active_duration) {
    return freeze_ ? 1.0 : std::numeric_limits<double>::quiet_NaN();
  }
  const double iteration_time = std::fmod(elapsed, dur_);
  return dur_ <= 0.0 ? 0.0 : iteration_time / dur_;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
