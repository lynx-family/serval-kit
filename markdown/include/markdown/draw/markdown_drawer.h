// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_DRAWER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_DRAWER_H_

#include <memory>
#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_page.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx {
namespace markdown {
class MarkdownTableRegion;

class MarkdownDrawer {
 public:
  explicit MarkdownDrawer(MarkdownCanvas* canvas)
      : canvas_(canvas), painter_(nullptr) {}
  virtual ~MarkdownDrawer() = default;

  virtual void DrawPage(const MarkdownPage& page);
  void DrawRegion(const MarkdownPage& page, uint32_t region_index);
  void DrawQuoteBorder(const MarkdownPage& page, uint32_t border_index);

 protected:
  void DrawQuoteLine(const MarkdownQuoteBorder& border);
  void DrawBorder(const MarkdownPageRegionBorder& border);
  void DrawRegion(const MarkdownPageRegion& region,
                  tttext::LayoutDrawer* drawer);
  void DrawTable(const MarkdownTableRegion& table,
                 const MarkdownElement& element, tttext::LayoutDrawer* drawer);
  virtual void DrawTextRegion(tttext::LayoutRegion* page,
                              tttext::LayoutDrawer* drawer);

  MarkdownCanvas* canvas_;
  std::unique_ptr<tttext::Painter> painter_;
  bool terminated_{false};
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_DRAWER_H_
