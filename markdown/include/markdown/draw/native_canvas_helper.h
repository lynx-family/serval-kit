// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef MARKDOWN_INCLUDE_MARKDOWN_DRAW_NATIVE_CANVAS_HELPER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_DRAW_NATIVE_CANVAS_HELPER_H_
#include <textra/platform_helper.h>

#include <memory>
namespace serval {
namespace markdown {
// Keeps delegate overrides in the host while native drawing stays in Textra.
class NativeCanvasHelper : public ttoffice::tttext::ICanvasHelper {
 public:
  explicit NativeCanvasHelper(ttoffice::tttext::RenderingBackend backend,
                              void* target)
      : canvas_(ttoffice::tttext::PlatformHelper::CreateCanvasHelper(backend,
                                                                     target)) {}
  std::unique_ptr<ttoffice::tttext::Painter> CreatePainter() override {
    return canvas_ ? canvas_->CreatePainter()
                   : std::make_unique<ttoffice::tttext::Painter>();
  }
  void StartPaint() override {
    if (canvas_)
      canvas_->StartPaint();
  }
  void EndPaint() override {
    if (canvas_)
      canvas_->EndPaint();
  }
  void Save() override {
    if (canvas_)
      canvas_->Save();
  }
  void Restore() override {
    if (canvas_)
      canvas_->Restore();
  }
  void Translate(float dx, float dy) override {
    if (canvas_)
      canvas_->Translate(dx, dy);
  }
  void Scale(float sx, float sy) override {
    if (canvas_)
      canvas_->Scale(sx, sy);
  }
  void Rotate(float degrees) override {
    if (canvas_)
      canvas_->Rotate(degrees);
  }
  void Skew(float sx, float sy) override {
    if (canvas_)
      canvas_->Skew(sx, sy);
  }
  void ClipRect(float left, float top, float right, float bottom,
                bool doAntiAlias) override {
    if (canvas_)
      canvas_->ClipRect(left, top, right, bottom, doAntiAlias);
  }
  void Clear() override {
    if (canvas_)
      canvas_->Clear();
  }
  void ClearRect(float left, float top, float right, float bottom) override {
    if (canvas_)
      canvas_->ClearRect(left, top, right, bottom);
  }
  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override {
    if (canvas_)
      canvas_->FillRect(left, top, right, bottom, color);
  }
  void DrawColor(uint32_t color) override {
    if (canvas_)
      canvas_->DrawColor(color);
  }
  void DrawLine(float x1, float y1, float x2, float y2,
                ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawLine(x1, y1, x2, y2, painter);
  }
  void DrawRect(float left, float top, float right, float bottom,
                ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawRect(left, top, right, bottom, painter);
  }
  void DrawOval(float left, float top, float right, float bottom,
                ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawOval(left, top, right, bottom, painter);
  }
  void DrawCircle(float x, float y, float radius,
                  ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawCircle(x, y, radius, painter);
  }
  void DrawArc(float left, float top, float right, float bottom,
               float startAngle, float sweepAngle, bool useCenter,
               ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawArc(left, top, right, bottom, startAngle, sweepAngle,
                       useCenter, painter);
  }
  void DrawPath(ttoffice::tttext::Path* path,
                ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawPath(path, painter);
  }
  void DrawArcTo(float start_x, float start_y, float mid_x, float mid_y,
                 float end_x, float end_y, float radius,
                 ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawArcTo(start_x, start_y, mid_x, mid_y, end_x, end_y, radius,
                         painter);
  }
  void DrawText(const ttoffice::tttext::ITypefaceHelper* font, const char* text,
                uint32_t text_bytes, float x, float y,
                ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawText(font, text, text_bytes, x, y, painter);
  }
  void DrawGlyphs(const ttoffice::tttext::ITypefaceHelper* font,
                  uint32_t glyph_count, const uint16_t* glyphs,
                  const char* text, uint32_t text_bytes, float origin_x,
                  float origin_y, float* x, float* y,
                  ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawGlyphs(font, glyph_count, glyphs, text, text_bytes, origin_x,
                          origin_y, x, y, painter);
  }
  void DrawRunDelegate(const ttoffice::tttext::RunDelegate* delegate,
                       float left, float top, float right, float bottom,
                       ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawRunDelegate(delegate, left, top, right, bottom, painter);
  }
  void DrawBackgroundDelegate(const ttoffice::tttext::RunDelegate* delegate,
                              ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawBackgroundDelegate(delegate, painter);
  }
  void DrawImage(const char* src, float left, float top, float right,
                 float bottom, ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawImage(src, left, top, right, bottom, painter);
  }
  void DrawImageRect(const char* src, float src_left, float src_top,
                     float src_right, float src_bottom, float dst_left,
                     float dst_top, float dst_right, float dst_bottom,
                     ttoffice::tttext::Painter* painter,
                     bool srcRectPercent = false) override {
    if (canvas_)
      canvas_->DrawImageRect(src, src_left, src_top, src_right, src_bottom,
                             dst_left, dst_top, dst_right, dst_bottom, painter,
                             srcRectPercent);
  }
  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius,
                     ttoffice::tttext::Painter* painter) override {
    if (canvas_)
      canvas_->DrawRoundRect(left, top, right, bottom, radius, painter);
  }

 private:
  std::unique_ptr<ttoffice::tttext::ICanvasHelper> canvas_;
};
}  // namespace markdown
}  // namespace serval
#endif  // MARKDOWN_INCLUDE_MARKDOWN_DRAW_NATIVE_CANVAS_HELPER_H_
