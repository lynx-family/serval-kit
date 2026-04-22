// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#include <memory>
#include <string>

#include "markdown/utils/markdown_value.h"

namespace serval::markdown {

class MarkdownValueConvert {
 public:
  static std::unique_ptr<Value> ConvertObject(NSObject* object);
  static std::unique_ptr<Value> ConvertMap(NSDictionary* dictionary);
  static std::unique_ptr<Value> ConvertArray(NSArray* array);
  static std::unique_ptr<Value> ConvertString(NSString* string);
  static std::unique_ptr<Value> ConvertNumber(NSNumber* number);
};

}  // namespace serval::markdown

#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
