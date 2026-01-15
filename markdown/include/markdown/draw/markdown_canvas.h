// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
#include <memory>
#include <vector>
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"

namespace lynx::markdown {
class MarkdownCanvas : public tttext::ICanvasHelper {
 public:
  explicit MarkdownCanvas(tttext::ICanvasHelper* canvas)
      : canvas_helper_(canvas) {}
  ~MarkdownCanvas() override = default;

  std::unique_ptr<tttext::Painter> CreatePainter() override {
    return canvas_helper_->CreatePainter();
  }
  void Save() override {
    canvas_helper_->Save();
    state_stack_.emplace_back(CanvasState{translate_});
  }
  void Restore() override {
    translate_ = state_stack_.back().translate_;
    state_stack_.pop_back();
    canvas_helper_->Restore();
  }
  void Translate(float dx, float dy) override {
    canvas_helper_->Translate(dx, dy);
    translate_ += PointF{dx, dy};
  }
  void ClipRect(float left, float top, float right, float bottom,
                bool doAntiAlias) override {
    canvas_helper_->ClipRect(left, top, right, bottom, doAntiAlias);
  }
  void ClearRect(float left, float top, float right, float bottom) override {
    canvas_helper_->ClearRect(left, top, right, bottom);
  }
  void DrawLine(float x1, float y1, float x2, float y2,
                tttext::Painter* painter) override {
    canvas_helper_->DrawLine(x1, y1, x2, y2, painter);
  }
  void DrawRect(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {
    canvas_helper_->DrawRect(left, top, right, bottom, painter);
  }
  void DrawCircle(float x, float y, float radius,
                  tttext::Painter* painter) override {
    canvas_helper_->DrawCircle(x, y, radius, painter);
  }
  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius, tttext::Painter* painter) override {
    canvas_helper_->DrawRoundRect(left, top, right, bottom, radius, painter);
  }
  void StartPaint() override { canvas_helper_->StartPaint(); }
  void EndPaint() override { canvas_helper_->EndPaint(); }
  void Scale(float sx, float sy) override { canvas_helper_->Scale(sx, sy); }
  void Rotate(float degrees) override { canvas_helper_->Rotate(degrees); }
  void Skew(float sx, float sy) override { canvas_helper_->Skew(sx, sy); }
  void Clear() override { canvas_helper_->Clear(); }
  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override {
    canvas_helper_->FillRect(left, top, right, bottom, color);
  }
  void DrawColor(uint32_t color) override { canvas_helper_->DrawColor(color); }
  void DrawOval(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {
    canvas_helper_->DrawOval(left, top, right, bottom, painter);
  }
  void DrawArc(float left, float top, float right, float bottom,
               float startAngle, float sweepAngle, bool useCenter,
               tttext::Painter* painter) override {
    canvas_helper_->DrawArc(left, top, right, bottom, startAngle, sweepAngle,
                            useCenter, painter);
  }
  void DrawPath(tttext::Path* path, tttext::Painter* painter) override {
    canvas_helper_->DrawPath(path, painter);
  }
  void DrawArcTo(float start_x, float start_y, float mid_x, float mid_y,
                 float end_x, float end_y, float radius,
                 tttext::Painter* painter) override {
    canvas_helper_->DrawArcTo(start_x, start_y, mid_x, mid_y, end_x, end_y,
                              radius, painter);
  }
  void DrawText(const tttext::ITypefaceHelper* font, const char* text,
                uint32_t text_bytes, float x, float y,
                tttext::Painter* painter) override {
    canvas_helper_->DrawText(font, text, text_bytes, x, y, painter);
  }
  void DrawGlyphs(const tttext::ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text, uint32_t text_bytes,
                  float origin_x, float origin_y, float* x, float* y,
                  tttext::Painter* painter) override {
    canvas_helper_->DrawGlyphs(font, glyph_count, glyphs, text, text_bytes,
                               origin_x, origin_y, x, y, painter);
  }
  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override {
    canvas_helper_->DrawRunDelegate(delegate, left, top, right, bottom,
                                    painter);
  }
  void DrawBackgroundDelegate(const tttext::RunDelegate* delegate,
                              tttext::Painter* painter) override {
    canvas_helper_->DrawBackgroundDelegate(delegate, painter);
  }
  void DrawImage(const char* src, float left, float top, float right,
                 float bottom, tttext::Painter* painter) override {
    canvas_helper_->DrawImage(src, left, top, right, bottom, painter);
  }
  void DrawImageRect(const char* src, float src_left, float src_top,
                     float src_right, float src_bottom, float dst_left,
                     float dst_top, float dst_right, float dst_bottom,
                     tttext::Painter* painter, bool srcRectPercent) override {
    canvas_helper_->DrawImageRect(src, src_left, src_top, src_right, src_bottom,
                                  dst_left, dst_top, dst_right, dst_bottom,
                                  painter, srcRectPercent);
  }

  // Markdown Canvas Extensions
  virtual void ClipRoundRect(float left, float top, float right, float bottom,
                             float radiusX, float radiusY,
                             bool doAntiAlias) = 0;

  PointF GetCurrentTranslate() const { return translate_; }

  tttext::ICanvasHelper* canvas_helper_;

  struct CanvasState {
    PointF translate_;
  };
  std::vector<CanvasState> state_stack_;
  PointF translate_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_MARKDOWN_CANVAS_H_
