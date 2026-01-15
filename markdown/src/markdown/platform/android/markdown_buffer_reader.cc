// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/android/markdown_buffer_reader.h"

#include <memory>
#include <utility>

std::unique_ptr<lynx::markdown::Value> MarkdownBufferReader::ReadValue() {
  auto type = static_cast<lynx::markdown::ValueType>(stream_.ReadInt32());
  switch (type) {
    case lynx::markdown::ValueType::kNull:
      return lynx::markdown::Value::MakeNull();
    case lynx::markdown::ValueType::kMap:
      return lynx::markdown::Value::MakeMap(ReadMap());
    case lynx::markdown::ValueType::kArray:
      return lynx::markdown::Value::MakeArray(ReadArray());
    case lynx::markdown::ValueType::kBool:
      return lynx::markdown::Value::MakeBool(stream_.ReadBool());
    case lynx::markdown::ValueType::kInt:
      return lynx::markdown::Value::MakeInt(stream_.ReadInt32());
    case lynx::markdown::ValueType::kLong:
      return lynx::markdown::Value::MakeLong(stream_.ReadInt64());
    case lynx::markdown::ValueType::kDouble:
      return lynx::markdown::Value::MakeDouble(stream_.ReadDouble());
    case lynx::markdown::ValueType::kString:
      return lynx::markdown::Value::MakeString(stream_.ReadStdString());
  }
}

lynx::markdown::ValueMap MarkdownBufferReader::ReadMap() {
  auto size = stream_.ReadInt32();
  lynx::markdown::ValueMap map;
  map.reserve(size);
  for (int i = 0; i < size; i++) {
    auto key = stream_.ReadStdString();
    auto value = ReadValue();
    map.emplace(std::move(key), std::move(value));
  }
  return map;
}

lynx::markdown::ValueArray MarkdownBufferReader::ReadArray() {
  auto size = stream_.ReadInt32();
  lynx::markdown::ValueArray array;
  array.reserve(size);
  for (int i = 0; i < size; i++) {
    auto value = ReadValue();
    array.emplace_back(std::move(value));
  }
  return array;
}
