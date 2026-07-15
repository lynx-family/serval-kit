// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/android/buffer_output_stream.h"

namespace serval::markdown {

void BufferOutputStream::WriteInt8(int8_t value) {
  WriteType(value);
}

void BufferOutputStream::WriteInt32(int32_t value) {
  WriteType(value);
}

void BufferOutputStream::WriteInt64(int64_t value) {
  WriteType(value);
}

void BufferOutputStream::WriteFloat(float value) {
  WriteType(value);
}

void BufferOutputStream::WriteDouble(double value) {
  WriteType(value);
}

void BufferOutputStream::WriteBool(bool value) {
  WriteType(value);
}

void BufferOutputStream::WriteString(const char* value) {
  if (value == nullptr) {
    WriteInt32(0);
    return;
  }
  WriteStdString(value);
}

void BufferOutputStream::WriteStdString(std::string_view value) {
  WriteInt32(static_cast<int32_t>(value.size()));
  WriteBuffer(reinterpret_cast<const uint8_t*>(value.data()), value.size());
}

void BufferOutputStream::WriteBuffer(const uint8_t* buffer, size_t len) {
  if (buffer == nullptr || len == 0) {
    return;
  }
  buffer_.insert(buffer_.end(), buffer, buffer + len);
}

template <typename T>
void BufferOutputStream::WriteType(T value) {
  WriteInternal(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void BufferOutputStream::WriteInternal(const uint8_t* buffer, size_t len) {
  if (little_endian_ || len == 1) {
    WriteBuffer(buffer, len);
    return;
  }
  for (size_t index = 0; index < len; ++index) {
    buffer_.emplace_back(buffer[len - index - 1]);
  }
}

}  // namespace serval::markdown
