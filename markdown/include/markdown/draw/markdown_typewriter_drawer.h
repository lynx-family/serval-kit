// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_

#include <memory>
#include <vector>
#include "markdown/draw/markdown_drawer.h"
#include "markdown/markdown_resource_loader.h"
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx {
namespace markdown {
// TODO(zhouchaoying): refactor typewriter drawer, Requires textlayout drawer to
// be able to draw by char range
class MarkdownTypewriterDrawer : public MarkdownDrawer, public MarkdownCanvas {
 public:
  MarkdownTypewriterDrawer(MarkdownCanvas* canvas, int32_t max_glyph_count,
                           MarkdownResourceLoader* loader,
                           const MarkdownTypewriterCursorStyle& style,
                           bool draw_cursor_if_complete,
                           tttext::RunDelegate* custom_typewriter_cursor);

  std::unique_ptr<tttext::Painter> CreatePainter() override {
    return origin_canvas_ ? origin_canvas_->CreatePainter()
                          : std::make_unique<tttext::Painter>();
  }
  void DrawPage(const lynx::markdown::MarkdownPage& page) override;

  bool PageDrawCompleted() const { return page_completed_; }

  PointF GetCursorPosition() const { return cursor_position_; }

  float GetMaxDrawHeight() const { return max_draw_height_; }

  int32_t GetDrawGlyphCount() const { return draw_glyph_count_; }

 protected:
  PointF CalculateCursorPosition(tttext::TextLine* line, PointF cursor_position,
                                 PointF region_offset,
                                 tttext::RunDelegate* cursor);
  void DrawTextRegion(tttext::LayoutRegion* page,
                      tttext::LayoutDrawer* drawer) override;

  tttext::RunDelegate* LoadTypewriterCursor(tttext::Painter* painter);

  MarkdownCanvas* origin_canvas_{nullptr};
  const MarkdownPage* page_{nullptr};
  MarkdownResourceLoader* loader_{nullptr};
  const MarkdownTypewriterCursorStyle* style_{nullptr};

  int32_t max_glyph_count_{-1};
  int32_t draw_glyph_count_{0};
  bool draw_cursor_if_complete_{false};
  tttext::RunDelegate* typewriter_cursor_{nullptr};
  std::unique_ptr<tttext::RunDelegate> default_typewriter_cursor_{nullptr};
  tttext::RunDelegate* custom_typewriter_cursor_{nullptr};
  PointF cursor_position_{0, 0};
  float max_draw_height_{0};
  tttext::LayoutRegion* last_draw_region_{nullptr};
  PointF region_offset_{0, 0};
  const tttext::ITypefaceHelper* last_draw_font_{nullptr};
  bool page_completed_{false};

  std::vector<PointF> translate_stack_;
  PointF translate_offset_;

 public:
  /*
   * canvas wrapper, used for typewriter
   * **/
  void Save() override {
    if (origin_canvas_ != nullptr) {
      origin_canvas_->Save();
    }
    translate_stack_.push_back(translate_offset_);
  }
  void Restore() override {
    if (origin_canvas_ != nullptr) {
      origin_canvas_->Restore();
    }
    if (!translate_stack_.empty()) {
      translate_offset_ = translate_stack_.back();
      translate_stack_.pop_back();
    }
  }
  void Translate(float dx, float dy) override {
    if (!terminated_) {
      if (origin_canvas_ != nullptr) {
        origin_canvas_->Translate(dx, dy);
      }
      translate_offset_.Translate(dx, dy);
    }
  }
  void DrawLine(float x1, float y1, float x2, float y2,
                tttext::Painter* painter) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->DrawLine(x1, y1, x2, y2, painter);
    }
  }
  void DrawRect(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->DrawRect(left, top, right, bottom, painter);
    }
  }
  void DrawGlyphs(const tttext::ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text, uint32_t text_bytes,
                  float origin_x, float origin_y, float* x, float* y,
                  tttext::Painter* painter) override;
  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->DrawRunDelegate(delegate, left, top, right, bottom,
                                      painter);
    }
  }
  void DrawCircle(float x, float y, float radius,
                  tttext::Painter* painter) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->DrawCircle(x, y, radius, painter);
    }
  }
  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius, tttext::Painter* painter) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->DrawRoundRect(left, top, right, bottom, radius, painter);
    }
  }
  void ClipRect(float left, float top, float right, float bottom,
                bool doAntiAlias) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->ClipRect(left, top, right, bottom, doAntiAlias);
    }
  }
  void ClipRoundRect(float left, float top, float right, float bottom,
                     float radiusX, float radiusY, bool doAntiAlias) override {
    if (!terminated_ && origin_canvas_ != nullptr) {
      origin_canvas_->ClipRoundRect(left, top, right, bottom, radiusX, radiusY,
                                    doAntiAlias);
    }
  }

  /* unused canvas function */
  void StartPaint() override {}
  void EndPaint() override {}
  void Scale(float sx, float sy) override {}
  void Rotate(float degrees) override {}
  void Skew(float sx, float sy) override {}
  void Clear() override {}
  void ClearRect(float left, float top, float right, float bottom) override {}
  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override {}
  void DrawColor(uint32_t color) override {}
  void DrawOval(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {}
  void DrawArc(float left, float top, float right, float bottom,
               float startAngle, float sweepAngle, bool useCenter,
               tttext::Painter* painter) override {}
  void DrawPath(tttext::Path* path, tttext::Painter* painter) override {}
  void DrawArcTo(float start_x, float start_y, float mid_x, float mid_y,
                 float end_x, float end_y, float radius,
                 tttext::Painter* painter) override {}
  void DrawText(const tttext::ITypefaceHelper* font, const char* text,
                uint32_t text_bytes, float x, float y,
                tttext::Painter* painter) override {}
  void DrawBackgroundDelegate(const tttext::RunDelegate* delegate,
                              tttext::Painter* painter) override {}
  void DrawImage(const char* src, float left, float top, float right,
                 float bottom, tttext::Painter* painter) override {}
  void DrawImageRect(const char* src, float src_left, float src_top,
                     float src_right, float src_bottom, float dst_left,
                     float dst_top, float dst_right, float dst_bottom,
                     tttext::Painter* painter, bool srcRectPercent) override {}
};
}  // namespace markdown
}  // namespace lynx

#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_TYPEWRITER_DRAWER_H_
