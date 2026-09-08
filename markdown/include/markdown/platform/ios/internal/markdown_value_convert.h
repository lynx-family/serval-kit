// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
#import <Foundation/Foundation.h>

#include <memory>
#include <string>

#include "markdown/platform/ios/ServalMarkdownConstants.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_value.h"

NS_ASSUME_NONNULL_BEGIN

namespace serval::markdown {

class MarkdownValueConvert {
 public:
  static MarkdownVerticalAlign ConvertVerticalAlign(
      ServalMarkdownVerticalAlign align);
  static std::unique_ptr<Value> ConvertObject(NSObject* _Nullable object);
  static std::unique_ptr<Value> ConvertMap(NSDictionary* _Nullable dictionary);
  static std::unique_ptr<Value> ConvertArray(NSArray* _Nullable array);
  static std::unique_ptr<Value> ConvertString(NSString* string);
  static std::unique_ptr<Value> ConvertNumber(NSNumber* number);
};

}  // namespace serval::markdown

NS_ASSUME_NONNULL_END

#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_VALUE_CONVERT_H_
