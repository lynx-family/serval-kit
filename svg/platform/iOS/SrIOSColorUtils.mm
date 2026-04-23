// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrIOSColorUtils.h"

#include <cmath>

#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace ios {

static uint32_t SrIOSClampColorComponent(CGFloat component) {
  CGFloat clamped = component;
  if (clamped < 0.f) {
    clamped = 0.f;
  } else if (clamped > 1.f) {
    clamped = 1.f;
  }
  return static_cast<uint32_t>(std::round(clamped * 255.f));
}

UIColor* SrIOSColorUtils::UIColorFromARGB(uint32_t color) {
  CGFloat alpha = ((color & 0xFF000000) >> 24) / 255.f;
  CGFloat red = ((color & 0x00FF0000) >> 16) / 255.f;
  CGFloat green = ((color & 0x0000FF00) >> 8) / 255.f;
  CGFloat blue = (color & 0x000000FF) / 255.f;
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

bool SrIOSColorUtils::UIColorToARGB(UIColor* color, uint32_t* out_color) {
  if (!color || !out_color) {
    return false;
  }
  CGFloat red = 0.f;
  CGFloat green = 0.f;
  CGFloat blue = 0.f;
  CGFloat alpha = 0.f;
  if (![color getRed:&red green:&green blue:&blue alpha:&alpha]) {
    CGFloat white = 0.f;
    if (![color getWhite:&white alpha:&alpha]) {
      return false;
    }
    red = white;
    green = white;
    blue = white;
  }
  *out_color = NSVG_RGBA(
      SrIOSClampColorComponent(red), SrIOSClampColorComponent(green),
      SrIOSClampColorComponent(blue), SrIOSClampColorComponent(alpha));
  return true;
}

CGFloat SrIOSColorUtils::GetRedFromARGB(uint32_t color) {
  return ((color & 0x00FF0000) >> 16) / 255.f;
}

CGFloat SrIOSColorUtils::GetGreenFromARGB(uint32_t color) {
  return ((color & 0x0000FF00) >> 8) / 255.f;
}

CGFloat SrIOSColorUtils::GetBlueFromARGB(uint32_t color) {
  return (color & 0x000000FF) / 255.f;
}

CGFloat SrIOSColorUtils::GetAlphaFromARGB(uint32_t color, float opacity) {
  return ((color & 0xFF000000) >> 24) / 255.f * opacity;
}

}  // namespace ios
}  // namespace svg
}  // namespace serval
