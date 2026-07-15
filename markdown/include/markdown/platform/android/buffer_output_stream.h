// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_OUTPUT_STREAM_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_OUTPUT_STREAM_H_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace serval::markdown {

class BufferOutputStream {
 public:
  explicit BufferOutputStream(bool little_endian = false)
      : little_endian_(little_endian) {}
  ~BufferOutputStream() = default;

 public:
  void WriteInt(int32_t value) { WriteInt32(value); }
  void WriteInt8(int8_t value);
  void WriteInt32(int32_t value);
  void WriteInt64(int64_t value);
  void WriteFloat(float value);
  void WriteDouble(double value);
  void WriteBool(bool value);
  void WriteString(const char* value);
  void WriteStdString(std::string_view value);
  void WriteBuffer(const uint8_t* buffer, size_t len);

  const uint8_t* GetBuffer() const { return buffer_.data(); }
  size_t GetSize() const { return buffer_.size(); }
  bool Empty() const { return buffer_.empty(); }
  void Clear() { buffer_.clear(); }
  void SetEndian(bool is_little) { little_endian_ = is_little; }
  bool IsLittleEndian() const { return little_endian_; }

 public:
  template <typename T>
  void WriteType(T value);

 private:
  void WriteInternal(const uint8_t* buffer, size_t len);

  std::vector<uint8_t> buffer_;
  bool little_endian_ = false;
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_OUTPUT_STREAM_H_
