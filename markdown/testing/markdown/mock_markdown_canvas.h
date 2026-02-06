// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_CANVAS_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_CANVAS_H_
#include <memory>
#include <string>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_document.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
namespace lynx::markdown::testing {
class MockMarkdownResourceLoader;
class MockMarkdownCanvas final : public tttext::ICanvasHelper,
                                 public MarkdownCanvasExtend {
 public:
  explicit MockMarkdownCanvas(MockMarkdownResourceLoader* resource_loader,
                              MarkdownDocument* document)
      : resource_loader_(resource_loader), document_(document) {
    result_.SetArray();
  }
  ~MockMarkdownCanvas() override = default;
  std::unique_ptr<tttext::Painter> CreatePainter() override;
  void StartPaint() override;
  void EndPaint() override;
  void Save() override;
  void Restore() override;
  void Translate(float dx, float dy) override;
  void Scale(float sx, float sy) override;
  void Rotate(float degrees) override;
  void Skew(float sx, float sy) override;
  void ClipRect(float left, float top, float right, float bottom,
                bool doAntiAlias) override;
  void Clear() override;
  void ClearRect(float left, float top, float right, float bottom) override;
  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override;
  void DrawColor(uint32_t color) override;
  void DrawLine(float x1, float y1, float x2, float y2,
                tttext::Painter* painter) override;
  void DrawRect(float left, float top, float right, float bottom,
                tttext::Painter* painter) override;
  void DrawOval(float left, float top, float right, float bottom,
                tttext::Painter* painter) override;
  void DrawCircle(float x, float y, float radius,
                  tttext::Painter* painter) override;
  void DrawArc(float left, float top, float right, float bottom,
               float startAngle, float sweepAngle, bool useCenter,
               tttext::Painter* painter) override;
  void DrawPath(tttext::Path* path, tttext::Painter* painter) override;
  void DrawArcTo(float start_x, float start_y, float mid_x, float mid_y,
                 float end_x, float end_y, float radius,
                 tttext::Painter* painter) override;
  void DrawText(const tttext::ITypefaceHelper* font, const char* text,
                uint32_t text_bytes, float x, float y,
                tttext::Painter* painter) override;
  void DrawGlyphs(const tttext::ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text, uint32_t text_bytes,
                  float origin_x, float origin_y, float* x, float* y,
                  tttext::Painter* painter) override;
  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override;
  void DrawBackgroundDelegate(const tttext::RunDelegate* delegate,
                              tttext::Painter* painter) override;
  void DrawImage(const char* src, float left, float top, float right,
                 float bottom, tttext::Painter* painter) override;
  void DrawView(const char* src, float left, float top, float right,
                float bottom);
  void DrawImageRect(const char* src, float src_left, float src_top,
                     float src_right, float src_bottom, float dst_left,
                     float dst_top, float dst_right, float dst_bottom,
                     tttext::Painter* painter, bool srcRectPercent) override;
  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius, tttext::Painter* painter) override;
  void ClipPath(MarkdownPath* path) override;
  void DrawMarkdownPath(MarkdownPath* path, tttext::Painter* painter) override;
  void DrawDelegateOnPath(tttext::RunDelegate* run_delegate, MarkdownPath* path,
                          tttext::Painter* painter) override;

  std::string GetResult() const;
  const rapidjson::Document& GetJson() const { return result_; }

 private:
  rapidjson::Value MakeRect(float left, float top, float right, float bottom);
  rapidjson::Value MakePoint(float x, float y);
  rapidjson::Value MakePoints(float* x, float* y, uint32_t count);
  rapidjson::Value MakePainter(tttext::Painter* painter);
  rapidjson::Value MakeFont(uint32_t id);
  rapidjson::Value MakePath(MarkdownPath* path);

  struct Context {
    PointF translate_{0, 0};
  };

  MockMarkdownResourceLoader* resource_loader_;
  MarkdownDocument* document_;
  Context context_;
  std::vector<Context> context_stack_;

  rapidjson::Document result_;
};
}  // namespace lynx::markdown::testing
#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_MARKDOWN_CANVAS_H_
