// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_

#include <memory>
#include <vector>

#include "markdown/draw/markdown_drawer.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx {
namespace markdown {
class MarkdownCharTypewriterDrawer : public MarkdownDrawer {
 public:
  MarkdownCharTypewriterDrawer(tttext::ICanvasHelper* canvas,
                               int32_t max_char_count,
                               MarkdownResourceLoader* loader,
                               const MarkdownTypewriterCursorStyle& style,
                               bool draw_cursor_if_complete,
                               tttext::RunDelegate* custom_typewriter_cursor);
  ~MarkdownCharTypewriterDrawer() override = default;

  void DrawPage(const MarkdownPage& page) override;

  PointF CalculateCursorPosition(const MarkdownPage* page);

  PointF GetCursorPosition() const { return cursor_position_; }

  float GetMaxDrawHeight() const { return max_draw_height_; }

  void DrawRegion(const MarkdownPage& page, uint32_t region_index) override;

 protected:
  std::unique_ptr<tttext::RunDelegate> CreateEllipsis(float text_size,
                                                      uint32_t color);
  PointF CalculateCursorPosition(tttext::TextLine* cursor_line,
                                 PointF cursor_position, PointF region_offset,
                                 tttext::RunDelegate* cursor, float page_width,
                                 MarkdownVerticalAlign align);
  void DrawTypewriterCursor();

  void DrawTextRegion(tttext::LayoutRegion* page,
                      tttext::LayoutDrawer* drawer) override;

  tttext::RunDelegate* LoadTypewriterCursor(float size, uint32_t color);
  void DrawAttachment(const MarkdownPage& page,
                      MarkdownTextAttachment* attachment) override;
  void DrawAttachmentOnRegion(const MarkdownPage& page,
                              MarkdownTextAttachment* attachment,
                              int32_t region_char_start,
                              int32_t region_char_end) override;

 private:
  const MarkdownPage* page_{nullptr};
  const MarkdownTypewriterCursorStyle* style_{nullptr};

  int32_t max_char_count_{std::numeric_limits<int32_t>::max()};
  int32_t draw_char_count_{0};
  bool draw_cursor_if_complete_{false};

  tttext::RunDelegate* typewriter_cursor_{nullptr};
  std::unique_ptr<tttext::RunDelegate> default_typewriter_cursor_{nullptr};
  tttext::RunDelegate* custom_typewriter_cursor_{nullptr};

  PointF cursor_position_{0, 0};
  float max_draw_height_{0};
};
}  // namespace markdown
}  // namespace lynx

#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_
