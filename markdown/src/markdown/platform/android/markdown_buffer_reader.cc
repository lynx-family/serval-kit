// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/android/markdown_buffer_reader.h"

#include <memory>
#include <utility>

std::unique_ptr<serval::markdown::Value> MarkdownBufferReader::ReadValue() {
  auto type = static_cast<serval::markdown::ValueType>(stream_.ReadInt32());
  switch (type) {
    case serval::markdown::ValueType::kNull:
      return serval::markdown::Value::MakeNull();
    case serval::markdown::ValueType::kMap:
      return serval::markdown::Value::MakeMap(ReadMap());
    case serval::markdown::ValueType::kArray:
      return serval::markdown::Value::MakeArray(ReadArray());
    case serval::markdown::ValueType::kBool:
      return serval::markdown::Value::MakeBool(stream_.ReadBool());
    case serval::markdown::ValueType::kInt:
      return serval::markdown::Value::MakeInt(stream_.ReadInt32());
    case serval::markdown::ValueType::kLong:
      return serval::markdown::Value::MakeLong(stream_.ReadInt64());
    case serval::markdown::ValueType::kDouble:
      return serval::markdown::Value::MakeDouble(stream_.ReadDouble());
    case serval::markdown::ValueType::kString:
      return serval::markdown::Value::MakeString(stream_.ReadStdString());
  }
}

serval::markdown::ValueMap MarkdownBufferReader::ReadMap() {
  auto size = stream_.ReadInt32();
  serval::markdown::ValueMap map;
  map.reserve(size);
  for (int i = 0; i < size; i++) {
    auto key = stream_.ReadStdString();
    auto value = ReadValue();
    map.emplace(std::move(key), std::move(value));
  }
  return map;
}

serval::markdown::ValueArray MarkdownBufferReader::ReadArray() {
  auto size = stream_.ReadInt32();
  serval::markdown::ValueArray array;
  array.reserve(size);
  for (int i = 0; i < size; i++) {
    auto value = ReadValue();
    array.emplace_back(std::move(value));
  }
  return array;
}
