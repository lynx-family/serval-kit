// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_INPUT_STREAM_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_INPUT_STREAM_H_

#include <cstdint>
#include <memory>
#include <string>

class BufferInputStream {
 public:
  BufferInputStream(const uint8_t* buff, size_t len,
                    bool little_endian = false);
  ~BufferInputStream();

 public:
  int32_t ReadInt() { return ReadInt32(); }
  int8_t ReadInt8();
  int32_t ReadInt32();
  int64_t ReadInt64();
  float ReadFloat();
  double ReadDouble();
  bool ReadBool();
  const char* ReadString();
  std::string ReadStdString();
  bool Empty() { return ((size_t)(current_ - buffer_) >= buffer_len_); }
  void SetEndian(bool is_little) { little_endian_ = is_little; }
  bool IsLittleEndian() const { return little_endian_; }

 public:
  template <typename T>
  T ReadType();
  std::unique_ptr<char[]> CopyBuffer(int len);

 private:
  const uint8_t* buffer_;
  const uint8_t* current_;
  size_t buffer_len_;
  bool little_endian_ = false;
};

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_BUFFER_INPUT_STREAM_H_
