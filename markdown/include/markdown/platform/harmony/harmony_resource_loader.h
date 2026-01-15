// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_RESOURCE_LOADER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_RESOURCE_LOADER_H_
#include "arkui/native_type.h"
#include "native_drawing/drawing_types.h"
namespace lynx::markdown {
class IHarmonyResourceLoader {
 public:
  virtual OH_Drawing_Font* LoadFont(const char* family) = 0;
  virtual ArkUI_NodeHandle LoadImageView(const char* src, float desire_width,
                                         float desire_height, float max_width,
                                         float max_height,
                                         float border_radius) = 0;
  virtual ArkUI_NodeHandle LoadInlineView(const char* id, float max_width,
                                          float max_height) = 0;
  virtual ArkUI_NodeHandle LoadReplacementView(void* ud, float max_width,
                                               float max_height) = 0;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_HARMONY_RESOURCE_LOADER_H_
