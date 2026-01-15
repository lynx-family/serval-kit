// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_VALUE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_VALUE_H_
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace lynx::markdown {
enum class ValueType : uint8_t {
  kNull = 0,
  kMap,
  kArray,
  kBool,
  kInt,
  kLong,
  kDouble,
  kString,
};
class Value;
using ValueArray = std::vector<std::unique_ptr<Value>>;
using ValueMap = std::unordered_map<std::string, std::unique_ptr<Value>>;
class Value {
 public:
  ValueType GetType() const { return type_; }
  static std::unique_ptr<Value> MakeNull();
  static std::unique_ptr<Value> MakeInt(int32_t content = 0);
  static std::unique_ptr<Value> MakeLong(int64_t content = 0);
  static std::unique_ptr<Value> MakeDouble(double content = 0);
  static std::unique_ptr<Value> MakeBool(bool content = false);
  static std::unique_ptr<Value> MakeString(std::string&& content = "");
  static std::unique_ptr<Value> MakeMap(ValueMap&& content = {});
  static std::unique_ptr<Value> MakeArray(ValueArray&& content = {});

  bool& AsBool();
  int32_t& AsInt();
  int64_t& AsLong();
  double& AsDouble();
  std::string& AsString();
  ValueMap& AsMap();
  ValueArray& AsArray();

  bool GetBool();
  int32_t GetInt();
  int64_t GetLong();
  double GetDouble();
  std::string GetString();
  Value* GetByIndex(uint32_t index);
  Value* GetByKey(const std::string& key);

 protected:
  explicit Value(ValueType type) : type_(type) {}
  void SetType(ValueType type) { type_ = type; }

 protected:
  ValueType type_{};
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_VALUE_H_
