// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#include "markdown/markdown_resource_loader.h"
#include "markdown/utils/markdown_value.h"
class MarkdownValueConvert {
 public:
  static std::unique_ptr<lynx::markdown::Value> ConvertObject(NSObject* object);
  static std::unique_ptr<lynx::markdown::Value> ConvertMap(
      NSDictionary* dictionary);
  static std::unique_ptr<lynx::markdown::Value> ConvertArray(NSArray* array);
  static std::unique_ptr<lynx::markdown::Value> ConvertString(NSString* string);
  static std::unique_ptr<lynx::markdown::Value> ConvertNumber(NSNumber* number);
};
#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
