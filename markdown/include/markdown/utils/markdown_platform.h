// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_PLATFORM_H_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_PLATFORM_H_
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {
class MarkdownCanvasExtend;
class MarkdownPlatform {
 public:
  static tttext::TextLayout* GetTextLayout();
  static float GetMdLayoutRegionWidth(tttext::LayoutRegion* region) {
    return region->GetWidthMode() == tttext::LayoutMode::kDefinite
               ? region->GetPageWidth()
               : region->GetLayoutedWidth();
  }
  static float GetMdLayoutRegionHeight(tttext::LayoutRegion* region) {
    return region->GetHeightMode() == tttext::LayoutMode::kDefinite
               ? region->GetPageHeight()
               : region->GetLayoutedHeight();
  }
  static MarkdownCanvasExtend* GetMarkdownCanvasExtend(
      tttext::ICanvasHelper* canvas);
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_PLATFORM_H_
