// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_THIRD_PARTY_BASE_INCLUDE_AUTO_CREATE_OPTIONAL_H_
#define MARKDOWN_THIRD_PARTY_BASE_INCLUDE_AUTO_CREATE_OPTIONAL_H_

#include <memory>

namespace lynx {
namespace base {

template <typename T>
class auto_create_optional {
 public:
  auto_create_optional() = default;

  auto_create_optional(const auto_create_optional& other) {
    if (other.data_) {
      data_ = std::make_unique<T>(*other.data_);
    }
  }

  auto_create_optional& operator=(const auto_create_optional& other) {
    if (this == &other) {
      return *this;
    }
    if (other.data_) {
      data_ = std::make_unique<T>(*other.data_);
    } else {
      data_ = nullptr;
    }
    return *this;
  }

  auto_create_optional(auto_create_optional&& other) = default;
  auto_create_optional& operator=(auto_create_optional&& other) = default;

  T& operator*() const { return *create_if_null(); }

  T* operator->() const { return create_if_null(); }

  T* get() const { return data_.get(); }

  void reset() { data_.reset(); }

  bool has_value() const noexcept { return data_ != nullptr; }

  explicit operator bool() const noexcept { return has_value(); }

 private:
  mutable std::unique_ptr<T> data_;

  T* create_if_null() const {
    if (!data_) {
      data_ = std::make_unique<T>();
    }
    return data_.get();
  }
};

}  // namespace base
}  // namespace lynx

#endif  // MARKDOWN_THIRD_PARTY_BASE_INCLUDE_AUTO_CREATE_OPTIONAL_H_
