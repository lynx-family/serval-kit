// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/style/markdown_style_value.h"

#include <vector>

#include "markdown/utils/markdown_screen_metrics.h"
namespace lynx::markdown {
float MarkdownLengthValue::CalculateLengthValue(
    const MarkdownLengthContext& context) const {
  switch (type_) {
    case StyleValuePattern::kPx:
    case StyleValuePattern::kNumber:
      return value_ * context.dpi_;
    case StyleValuePattern::kEm:
      return context.font_size_ * value_;
    case StyleValuePattern::kRem:
      return context.root_font_size_ * value_;
    case StyleValuePattern::kVH:
      return value_ * 0.01 * context.screen_height_;
    case StyleValuePattern::kVW:
      return value_ * 0.01 * context.screen_width_;
    case StyleValuePattern::kPercent:
      return value_ * 0.01 * context.base_length_;
    default:
      return 0;
  }
}
float MarkdownCalculateValue::CalculateLengthValue(
    const MarkdownLengthContext& context) const {
  const float left =
      left_value_ == nullptr ? 0 : left_value_->CalculateLengthValue(context);
  const float right =
      right_value_ == nullptr ? 0 : right_value_->CalculateLengthValue(context);
  switch (operator_) {
    case OperatorType::kUndefined:
      return 0;
    case OperatorType::kAdd:
      return left + right;
    case OperatorType::kMinus:
      return left - right;
    case OperatorType::kMultiple:
      return left * right;
    case OperatorType::kDivide:
      return right == 0 ? 0 : left / right;
  }
  return 0;
}

constexpr std::string_view ToString(const StyleValuePattern unit) {
  switch (unit) {
    case StyleValuePattern::kPx:
      return "px";
    case StyleValuePattern::kEm:
      return "em";
    case StyleValuePattern::kRem:
      return "rem";
    case StyleValuePattern::kVH:
      return "vh";
    case StyleValuePattern::kVW:
      return "vw";
    case StyleValuePattern::kPercent:
      return "%";
    default:
      break;
  }
  return "";
}

StyleValuePattern ParseUnit(const std::string_view str) {
  for (auto& var : {StyleValuePattern::kPx, StyleValuePattern::kEm,
                    StyleValuePattern::kRem, StyleValuePattern::kVH,
                    StyleValuePattern::kVW, StyleValuePattern::kPercent}) {
    if (str == ToString(var)) {
      return var;
    }
  }
  return StyleValuePattern::kEmpty;
}

std::unique_ptr<MarkdownStyleValue> ParseLength(
    const std::string_view content) {
  char* end;
  const auto number = strtof(content.data(), &end);
  const size_t value_len = end - content.data();
  if (value_len == content.length()) {
    return std::make_unique<MarkdownNumberValue>(number);
  }
  const StyleValuePattern unit = ParseUnit(content.substr(value_len));
  if (unit == StyleValuePattern::kEmpty) {
    return nullptr;
  }
  return std::make_unique<MarkdownLengthValue>(number, unit);
}

bool Contains(const char c, const std::string_view keywords) {
  for (const auto keyword : keywords) {
    if (keyword == c) {
      return true;
    }
  }
  return false;
}

size_t NextKeyword(const std::string_view content,
                   const std::string_view keywords) {
  for (size_t index = 0; index < content.size(); index++) {
    if (Contains(content[index], keywords)) {
      return index;
    }
  }
  return content.size();
}

size_t SkipSpace(const std::string_view content) {
  for (size_t index = 0; index < content.size(); index++) {
    if (content[index] != ' ') {
      return index;
    }
  }
  return content.size();
}

bool AddValue(MarkdownCalculateValue* expression,
              std::unique_ptr<MarkdownStyleValue> value) {
  if (expression->left_value_ == nullptr) {
    expression->left_value_ = std::move(value);
  } else if (expression->operator_ == OperatorType::kUndefined) {
    return false;
  } else if (expression->right_value_ == nullptr) {
    expression->right_value_ = std::move(value);
  } else {
    return false;
  }
  return true;
}

OperatorType GetOperatorType(char c) {
  switch (c) {
    case '+':
      return OperatorType::kAdd;
    case '-':
      return OperatorType::kMinus;
    case '*':
      return OperatorType::kMultiple;
    case '/':
      return OperatorType::kDivide;
    default:
      return OperatorType::kUndefined;
  }
}

int32_t OperatorPriority(const OperatorType op) {
  switch (op) {
    case OperatorType::kUndefined:
      return 0;
    case OperatorType::kAdd:
      return 1;
    case OperatorType::kMinus:
      return 1;
    case OperatorType::kMultiple:
      return 2;
    case OperatorType::kDivide:
      return 2;
  }
  return 0;
}

struct ExpressionOperator {
  OperatorType operator_{OperatorType::kUndefined};
  int32_t priority_{0};
};
using OperatorStack = std::vector<ExpressionOperator>;
using ValueStack = std::vector<std::unique_ptr<MarkdownStyleValue>>;
bool MakeTopExpression(OperatorStack* stack, ValueStack* values) {
  if (values->empty()) {
    return false;
  }
  auto expression = std::make_unique<MarkdownCalculateValue>();
  expression->right_value_ = std::move(values->back());
  values->pop_back();
  expression->operator_ = stack->back().operator_;
  stack->pop_back();
  if (!values->empty()) {
    expression->left_value_ = std::move(values->back());
    values->pop_back();
  }
  values->emplace_back(std::move(expression));
  return true;
}
bool PushOp(OperatorStack* stack, ValueStack* values, const OperatorType op) {
  const auto priority = OperatorPriority(op);
  while (!stack->empty() && stack->back().priority_ >= priority) {
    if (!MakeTopExpression(stack, values)) {
      return false;
    }
  }
  stack->emplace_back(
      ExpressionOperator{.operator_ = op, .priority_ = priority});
  return true;
}

std::unique_ptr<MarkdownStyleValue> ParseExpression(
    const std::string_view content, size_t* len) {
  constexpr std::string_view keywords{"+-*/() ", 7};
  size_t index = 0;
  OperatorStack op_stack;
  ValueStack value_stack;
  while (index < content.size()) {
    const auto offset = NextKeyword(content.substr(index), keywords);
    if (index + offset >= content.size()) {
      return nullptr;
    }
    if (offset > 0) {
      const auto token = content.substr(index, offset);
      auto value = ParseLength(token);
      if (value == nullptr || !value->IsValid()) {
        return nullptr;
      }
      value_stack.emplace_back(std::move(value));
    }
    index += offset;
    const char next_char = content[index];
    if (next_char == ')') {
      PushOp(&op_stack, &value_stack, OperatorType::kUndefined);
      index++;
      break;
    }
    if (next_char == ' ') {
      index += SkipSpace(content.substr(index));
    } else if (next_char == '(') {
      size_t sub_len = 0;
      auto sub_expression =
          ParseExpression(content.substr(index + 1), &sub_len);
      if (sub_expression == nullptr) {
        return nullptr;
      }
      value_stack.emplace_back(std::move(sub_expression));
      index += sub_len + 1;
    } else {
      if (const auto op = GetOperatorType(next_char);
          !PushOp(&op_stack, &value_stack, op)) {
        return nullptr;
      }
      index++;
    }
  }
  if (value_stack.size() > 1) {
    return nullptr;
  }
  *len = index;
  return std::move(value_stack.back());
}

std::unique_ptr<MarkdownStyleValue> MarkdownStyleValue::ParseValue(
    const std::string_view content) {
  if (content.empty()) {
    return nullptr;
  }
  if (content.length() > 6 && strncmp(content.data(), "calc(", 5) == 0) {
    size_t len = 0;
    return ParseExpression(content.substr(5), &len);
  }
  if ((content[0] >= '0' && content[0] <= '9') || content[0] == '-' ||
      content[0] == '+') {
    return ParseLength(content);
  }
  return nullptr;
}

float MarkdownLengthValue::GetPx() const {
  if (unit_ == StyleValuePattern::kDp) {
    return MarkdownScreenMetrics::DPToPx(value_);
  }
  return value_;
}

}  // namespace lynx::markdown
