// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_value_convert.h"

std::unique_ptr<serval::markdown::Value> MarkdownValueConvert::ConvertObject(
    NSObject* object) {
  if (object == nil) {
    return serval::markdown::Value::MakeNull();
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
    return serval::markdown::Value::MakeNull();
  }
}

std::unique_ptr<serval::markdown::Value> MarkdownValueConvert::ConvertMap(
    NSDictionary* dictionary) {
  serval::markdown::ValueMap map;
  if (dictionary == nil) {
    return serval::markdown::Value::MakeMap(std::move(map));
  }
  map.reserve(dictionary.count);
  auto it = dictionary.keyEnumerator;
  NSString* key = nil;
  while ((key = it.nextObject)) {
    NSObject* value = dictionary[key];
    auto* key_str = [key UTF8String];
    map.emplace(key_str, ConvertObject(value));
  }
  return serval::markdown::Value::MakeMap(std::move(map));
}

std::unique_ptr<serval::markdown::Value> MarkdownValueConvert::ConvertArray(
    NSArray* array) {
  serval::markdown::ValueArray result;
  if (array == nil) {
    return serval::markdown::Value::MakeArray(std::move(result));
  }
  result.reserve(array.count);
  for (int i = 0; i < array.count; i++) {
    result.emplace_back(ConvertObject(array[i]));
  }
  return serval::markdown::Value::MakeArray(std::move(result));
}

std::unique_ptr<serval::markdown::Value> MarkdownValueConvert::ConvertString(
    NSString* string) {
  auto cstr = [string UTF8String];
  return serval::markdown::Value::MakeString(std::string(cstr));
}

std::unique_ptr<serval::markdown::Value> MarkdownValueConvert::ConvertNumber(
    NSNumber* number) {
  CFNumberType type = CFNumberGetType((CFNumberRef)number);
  switch (type) {
    case kCFNumberIntType:
    case kCFNumberSInt16Type:
    case kCFNumberSInt32Type:
    case kCFNumberLongType:
      return serval::markdown::Value::MakeInt(number.intValue);
    case kCFNumberLongLongType:
    case kCFNumberSInt64Type:
    case kCFNumberCFIndexType:
    case kCFNumberNSIntegerType:
      return serval::markdown::Value::MakeLong(number.longLongValue);
    case kCFNumberSInt8Type:
    case kCFNumberCharType:
      return serval::markdown::Value::MakeBool(number.boolValue);
    case kCFNumberFloatType:
    case kCFNumberFloat32Type:
    case kCFNumberFloat64Type:
    case kCFNumberDoubleType:
    case kCFNumberCGFloatType:
      return serval::markdown::Value::MakeDouble(number.doubleValue);
    default:
      return serval::markdown::Value::MakeNull();
  }
}
