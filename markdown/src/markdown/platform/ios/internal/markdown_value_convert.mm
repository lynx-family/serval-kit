// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_value_convert.h"

std::unique_ptr<lynx::markdown::Value> MarkdownValueConvert::ConvertObject(
    NSObject* object) {
  if (object == nil) {
    return lynx::markdown::Value::MakeNull();
  }
  if ([object isKindOfClass:NSDictionary.class]) {
    return ConvertMap((NSDictionary*)object);
  } else if ([object isKindOfClass:NSArray.class]) {
    return ConvertArray((NSArray*)object);
  } else if ([object isKindOfClass:NSNumber.class]) {
    return ConvertNumber((NSNumber*)object);
  } else if ([object isKindOfClass:NSString.class]) {
    return ConvertString((NSString*)object);
  } else {
    return lynx::markdown::Value::MakeNull();
  }
}

std::unique_ptr<lynx::markdown::Value> MarkdownValueConvert::ConvertMap(
    NSDictionary* dictionary) {
  lynx::markdown::ValueMap map;
  if (dictionary == nil) {
    return lynx::markdown::Value::MakeMap(std::move(map));
  }
  map.reserve(dictionary.count);
  auto it = dictionary.keyEnumerator;
  NSString* key = nil;
  while ((key = it.nextObject)) {
    NSObject* value = dictionary[key];
    auto* key_str = [key UTF8String];
    map.emplace(key_str, ConvertObject(value));
  }
  return lynx::markdown::Value::MakeMap(std::move(map));
}

std::unique_ptr<lynx::markdown::Value> MarkdownValueConvert::ConvertArray(
    NSArray* array) {
  lynx::markdown::ValueArray result;
  if (array == nil) {
    return lynx::markdown::Value::MakeArray(std::move(result));
  }
  result.reserve(array.count);
  for (int i = 0; i < array.count; i++) {
    result.emplace_back(ConvertObject(array[i]));
  }
  return lynx::markdown::Value::MakeArray(std::move(result));
}

std::unique_ptr<lynx::markdown::Value> MarkdownValueConvert::ConvertString(
    NSString* string) {
  auto cstr = [string UTF8String];
  return lynx::markdown::Value::MakeString(std::string(cstr));
}

std::unique_ptr<lynx::markdown::Value> MarkdownValueConvert::ConvertNumber(
    NSNumber* number) {
  CFNumberType type = CFNumberGetType((CFNumberRef)number);
  switch (type) {
    case kCFNumberIntType:
    case kCFNumberSInt16Type:
    case kCFNumberSInt32Type:
    case kCFNumberLongType:
      return lynx::markdown::Value::MakeInt(number.intValue);
    case kCFNumberLongLongType:
    case kCFNumberSInt64Type:
    case kCFNumberCFIndexType:
    case kCFNumberNSIntegerType:
      return lynx::markdown::Value::MakeLong(number.longLongValue);
    case kCFNumberSInt8Type:
    case kCFNumberCharType:
      return lynx::markdown::Value::MakeBool(number.boolValue);
    case kCFNumberFloatType:
    case kCFNumberFloat32Type:
    case kCFNumberFloat64Type:
    case kCFNumberDoubleType:
    case kCFNumberCGFloatType:
      return lynx::markdown::Value::MakeDouble(number.doubleValue);
    default:
      return lynx::markdown::Value::MakeNull();
  }
}
