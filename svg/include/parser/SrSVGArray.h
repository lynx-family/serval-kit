// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SVG_INCLUDE_PARSER_SRSVGARRAY_H_
#define SVG_INCLUDE_PARSER_SRSVGARRAY_H_
#include <cstring>
#include <type_traits>
#include "parser/SrSVGMemory.h"

namespace serval::svg::parser {
// Parser scratch storage for trivial values. Every growth is explicitly fallible,
// including in -fno-exceptions builds; ownership never depends on a catch block.
template <typename T>
class SrSVGArray {
  static_assert(std::is_trivially_copyable_v<T>);

 public:
  SrSVGArray() = default;
  SrSVGArray(const SrSVGArray&) = delete;
  SrSVGArray& operator=(const SrSVGArray&) = delete;
  ~SrSVGArray() { SrSVGBufferRelease(&buffer_); }
  bool reserve(size_t count) {
    size_t bytes;
    return SrSVGAllocationSize(count, sizeof(T), &bytes) &&
           SrSVGBufferReserve(&buffer_, bytes);
  }
  bool push_back(const T& value) {
    return SrSVGBufferAppend(&buffer_, &value, sizeof(value));
  }
  void pop_back() { buffer_.size -= sizeof(T); }
  void clear() { buffer_.size = 0; }
  size_t size() const { return buffer_.size / sizeof(T); }
  size_t capacity() const { return buffer_.capacity / sizeof(T); }
  bool empty() const { return buffer_.size == 0; }
  T* data() { return static_cast<T*>(buffer_.data); }
  const T* data() const { return static_cast<const T*>(buffer_.data); }
  T* begin() { return data(); }
  T* end() { return empty() ? data() : data() + size(); }
  T& front() { return *data(); }
  T& back() { return data()[size() - 1]; }
  const T& back() const { return data()[size() - 1]; }

 private:
  SrSVGBuffer buffer_{};
};
}  // namespace serval::svg::parser
#endif  // SVG_INCLUDE_PARSER_SRSVGARRAY_H_
