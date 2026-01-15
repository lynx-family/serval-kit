// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_value.h"

namespace lynx::markdown {
class NullValue : public Value {
 public:
  NullValue() : Value(ValueType::kNull) {}
};

template <typename T, ValueType type>
class ValueTemplate : public Value {
 public:
  explicit ValueTemplate(T content)
      : Value(type), content_(std::move(content)) {}
  T content_;
};

using MapValue = ValueTemplate<ValueMap, ValueType::kMap>;
using ArrayValue = ValueTemplate<ValueArray, ValueType::kArray>;
using BoolValue = ValueTemplate<bool, ValueType::kBool>;
using IntValue = ValueTemplate<int32_t, ValueType::kInt>;
using LongValue = ValueTemplate<int64_t, ValueType::kLong>;
using DoubleValue = ValueTemplate<double, ValueType::kDouble>;
using StringValue = ValueTemplate<std::string, ValueType::kString>;
std::unique_ptr<Value> Value::MakeNull() {
  return std::make_unique<NullValue>();
}
std::unique_ptr<Value> Value::MakeMap(ValueMap&& content) {
  return std::make_unique<MapValue>(std::move(content));
}
std::unique_ptr<Value> Value::MakeArray(ValueArray&& content) {
  return std::make_unique<ArrayValue>(std::move(content));
}
std::unique_ptr<Value> Value::MakeBool(bool content) {
  return std::make_unique<BoolValue>(content);
}
std::unique_ptr<Value> Value::MakeInt(int32_t content) {
  return std::make_unique<IntValue>(content);
}
std::unique_ptr<Value> Value::MakeLong(int64_t content) {
  return std::make_unique<LongValue>(content);
}
std::unique_ptr<Value> Value::MakeDouble(double content) {
  return std::make_unique<DoubleValue>(content);
}
std::unique_ptr<Value> Value::MakeString(std::string&& content) {
  return std::make_unique<StringValue>(std::move(content));
}
bool& Value::AsBool() {
  return reinterpret_cast<BoolValue*>(this)->content_;
}
int32_t& Value::AsInt() {
  return reinterpret_cast<IntValue*>(this)->content_;
}
int64_t& Value::AsLong() {
  return reinterpret_cast<LongValue*>(this)->content_;
}
double& Value::AsDouble() {
  return reinterpret_cast<DoubleValue*>(this)->content_;
}
std::string& Value::AsString() {
  return reinterpret_cast<StringValue*>(this)->content_;
}
ValueArray& Value::AsArray() {
  return reinterpret_cast<ArrayValue*>(this)->content_;
}
ValueMap& Value::AsMap() {
  return reinterpret_cast<MapValue*>(this)->content_;
}

bool Value::GetBool() {
  switch (GetType()) {
    case ValueType::kBool:
      return AsBool();
    case ValueType::kInt:
      return AsInt();
    case ValueType::kLong:
      return AsLong();
    case ValueType::kDouble:
      return AsDouble();
    case ValueType::kString:
      return AsString() == "true";
    case ValueType::kNull:
    case ValueType::kMap:
    case ValueType::kArray:
      return false;
  }
}

int32_t Value::GetInt() {
  switch (GetType()) {
    case ValueType::kBool:
      return AsBool();
    case ValueType::kInt:
      return AsInt();
    case ValueType::kLong:
      return AsLong();
    case ValueType::kDouble:
      return AsDouble();
    case ValueType::kString:
      return std::strtol(AsString().c_str(), nullptr, 10);
    case ValueType::kNull:
    case ValueType::kMap:
    case ValueType::kArray:
      return 0;
  }
}

int64_t Value::GetLong() {
  switch (GetType()) {
    case ValueType::kBool:
      return AsBool();
    case ValueType::kInt:
      return AsInt();
    case ValueType::kLong:
      return AsLong();
    case ValueType::kDouble:
      return AsDouble();
    case ValueType::kString:
      return std::strtoll(AsString().c_str(), nullptr, 10);
    case ValueType::kNull:
    case ValueType::kMap:
    case ValueType::kArray:
      return 0;
  }
}
double Value::GetDouble() {
  switch (GetType()) {
    case ValueType::kBool:
      return AsBool();
    case ValueType::kInt:
      return AsInt();
    case ValueType::kLong:
      return AsLong();
    case ValueType::kDouble:
      return AsDouble();
    case ValueType::kString:
      return std::strtod(AsString().c_str(), nullptr);
    case ValueType::kNull:
    case ValueType::kMap:
    case ValueType::kArray:
      return 0;
  }
}
std::string Value::GetString() {
  switch (GetType()) {
    case ValueType::kBool:
      return AsBool() ? "true" : "false";
    case ValueType::kInt:
      return std::to_string(AsInt());
    case ValueType::kLong:
      return std::to_string(AsLong());
    case ValueType::kDouble:
      return std::to_string(AsDouble());
    case ValueType::kString:
      return AsString();
    case ValueType::kNull:
    case ValueType::kMap:
    case ValueType::kArray:
      return "";
  }
}
Value* Value::GetByIndex(uint32_t index) {
  if (GetType() != ValueType::kArray)
    return nullptr;
  if (index > AsArray().size()) {
    return nullptr;
  }
  return AsArray()[index].get();
}
Value* Value::GetByKey(const std::string& key) {
  if (GetType() != ValueType::kArray)
    return nullptr;
  auto find = AsMap().find(key);
  if (find == AsMap().end()) {
    return nullptr;
  }
  return find->second.get();
}
}  // namespace lynx::markdown
