// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
#include <cstdint>
#include <memory>
#include <string_view>
namespace lynx::markdown {
enum class StyleValuePattern {
  kEmpty,
  kString,
  kNumber,
  kBoolean,
  kEnum,
  kPx,
  kDp,
  kEm,
  kRem,
  kVH,
  kVW,
  kPercent,
  kCalculate,
};

struct MarkdownLengthContext {
  float screen_width_{0};
  float screen_height_{0};
  float font_size_{0};
  float root_font_size_{0};
  float base_length_{0};
  float dpi_{1};
};

class MarkdownStyleValue {
 public:
  explicit MarkdownStyleValue(const StyleValuePattern type) : type_(type) {}
  virtual ~MarkdownStyleValue() = default;
  StyleValuePattern GetType() const { return type_; }

  virtual float CalculateLengthValue(
      const MarkdownLengthContext& context) const {
    return 0;
  }
  bool IsValid() const { return type_ != StyleValuePattern::kEmpty; }

  static std::unique_ptr<MarkdownStyleValue> ParseValue(
      std::string_view content);

 protected:
  StyleValuePattern type_;
};

class MarkdownLengthValue final : public MarkdownStyleValue {
 public:
  MarkdownLengthValue() : MarkdownStyleValue(StyleValuePattern::kEmpty) {}
  MarkdownLengthValue(const float value, const StyleValuePattern unit)
      : MarkdownStyleValue(unit), value_(value), unit_(unit) {}

  float GetValue() const { return value_; }
  void SetValue(const float value) { value_ = value; }

  float CalculateLengthValue(
      const MarkdownLengthContext& context) const override;
  float GetPx() const;
  static MarkdownLengthValue FromDp(float value) {
    return {value, StyleValuePattern::kDp};
  }

 private:
  float value_{0};
  StyleValuePattern unit_{StyleValuePattern::kPx};
};

class MarkdownNumberValue final : public MarkdownStyleValue {
 public:
  MarkdownNumberValue() : MarkdownStyleValue(StyleValuePattern::kEmpty) {}
  explicit MarkdownNumberValue(double number)
      : MarkdownStyleValue(StyleValuePattern::kNumber), value_(number) {}
  ~MarkdownNumberValue() override = default;
  float CalculateLengthValue(
      const MarkdownLengthContext& context) const override {
    return value_;
  }
  double value_{0};
};

enum class OperatorType {
  kUndefined,
  kAdd,
  kMinus,
  kMultiple,
  kDivide,
};
class MarkdownCalculateValue final : public MarkdownStyleValue {
 public:
  MarkdownCalculateValue()
      : MarkdownStyleValue(StyleValuePattern::kCalculate) {}
  MarkdownCalculateValue(std::unique_ptr<MarkdownStyleValue> left,
                         OperatorType op,
                         std::unique_ptr<MarkdownStyleValue> right)
      : MarkdownStyleValue(StyleValuePattern::kCalculate),
        operator_(op),
        left_value_(std::move(left)),
        right_value_(std::move(right)) {}
  ~MarkdownCalculateValue() override = default;

  float CalculateLengthValue(
      const MarkdownLengthContext& context) const override;

  OperatorType operator_{OperatorType::kUndefined};
  std::unique_ptr<MarkdownStyleValue> left_value_;
  std::unique_ptr<MarkdownStyleValue> right_value_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_STYLE_VALUE_H_
