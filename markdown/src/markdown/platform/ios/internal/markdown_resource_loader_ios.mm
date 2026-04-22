// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_resource_loader_ios.h"

#include "markdown/platform/ios/internal/markdown_canvas_ios.h"

namespace serval::markdown {
namespace {
int32_t ConvertToPlatformFontWeight(MarkdownFontWeight weight) {
  switch (weight) {
    case MarkdownFontWeight::kBold:
      return 700;
    case MarkdownFontWeight::k100:
      return 100;
    case MarkdownFontWeight::k200:
      return 200;
    case MarkdownFontWeight::k300:
      return 300;
    case MarkdownFontWeight::k400:
      return 400;
    case MarkdownFontWeight::k500:
      return 500;
    case MarkdownFontWeight::k600:
      return 600;
    case MarkdownFontWeight::k700:
      return 700;
    case MarkdownFontWeight::k800:
      return 800;
    case MarkdownFontWeight::k900:
      return 900;
    case MarkdownFontWeight::kNormal:
    default:
      return 400;
  }
}
}  // namespace

MarkdownResourceLoaderIOS::~MarkdownResourceLoaderIOS() = default;

std::shared_ptr<MarkdownDrawable> MarkdownResourceLoaderIOS::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (delegate_ == nil || src == nullptr) {
    return nullptr;
  }
  UIImage* image =
      [delegate_ loadImageByURL:[NSString stringWithUTF8String:src]];
  if (image == nil) {
    return nullptr;
  }
  return std::make_shared<MarkdownImageRunDelegate>(
      image, desire_width, desire_height, max_width, max_height, border_radius);
}

std::shared_ptr<MarkdownDrawable> MarkdownResourceLoaderIOS::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  if (delegate_ == nil || id_selector == nullptr) {
    return nullptr;
  }
  id<IMarkdownPlatformViewHandle> handle =
      [delegate_ loadInlineView:[NSString stringWithUTF8String:id_selector]];
  if (handle == nil) {
    return nullptr;
  }
  return std::make_shared<MarkdownInlineViewRunDelegate>(handle);
}

void* MarkdownResourceLoaderIOS::LoadFont(const char* family,
                                          MarkdownFontWeight weight) {
  if (delegate_ == nil || family == nullptr) {
    return nullptr;
  }

  UIFont* font =
      [delegate_ loadFontByFamilyName:[NSString stringWithUTF8String:family]
                               Weight:ConvertToPlatformFontWeight(weight)
                                Style:0];
  if (font == nil) {
    return nullptr;
  }
  if (font_cache_ == nil) {
    font_cache_ = [[NSMutableSet alloc] init];
  }
  [font_cache_ addObject:font];
  return (__bridge void*)font;
}

std::shared_ptr<MarkdownDrawable> MarkdownResourceLoaderIOS::LoadGradient(
    const char* gradient, float font_size, float root_font_size) {
  return nullptr;
}

std::shared_ptr<MarkdownDrawable>
MarkdownResourceLoaderIOS::LoadReplacementView(void* ud, int32_t id,
                                               float max_width,
                                               float max_height) {
  return nullptr;
}

}  // namespace serval::markdown
