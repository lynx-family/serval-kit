// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SERIALIZED_JAVA_CANVAS_HELPER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SERIALIZED_JAVA_CANVAS_HELPER_H_

#include <textra/i_canvas_helper.h>

#include <memory>

#include "markdown/platform/android/buffer_output_stream.h"
/**
 * @brief A canvas helper implementation that writes serialized drawing commands
 * to a BufferOutputStream.
 */
namespace serval::markdown::internal {
class SerializedJavaCanvasHelper : public tttext::ICanvasHelper {
 public:
  SerializedJavaCanvasHelper() = default;
  ~SerializedJavaCanvasHelper() override = default;

 public:
  std::unique_ptr<tttext::Painter> CreatePainter() override {
    return std::make_unique<tttext::Painter>();
  }

  void StartPaint() override {}

  void EndPaint() override {}

  void Save() override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kSave));
  }

  void Restore() override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kRestore));
  }

  void Translate(float dx, float dy) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kTranslate));
    stream_.WriteType(dx);
    stream_.WriteType(dy);
  }

  void Scale(float sx, float sy) override {}

  void Rotate(float degrees) override {}

  void Skew(float sx, float sy) override {}

  void ClipRect(float left, float top, float right, float bottom,
                bool doAntiAlias) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kClipRect));
    stream_.WriteType(left);
    stream_.WriteType(top);
    stream_.WriteType(right);
    stream_.WriteType(bottom);
  }

  void Clear() override {}

  void ClearRect(float left, float top, float right, float bottom) override {}

  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override {}
  void DrawColor(uint32_t color) override {}
  void DrawLine(float x1, float y1, float x2, float y2,
                tttext::Painter* painter) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kDrawLine));
    stream_.WriteType(x1);
    stream_.WriteType(y1);
    stream_.WriteType(x2);
    stream_.WriteType(y2);
    WritePaint(painter);
  }
  void DrawRect(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kDrawRect));
    stream_.WriteType(left);
    stream_.WriteType(top);
    stream_.WriteType(right);
    stream_.WriteType(bottom);
    WritePaint(painter);
  }
  void DrawOval(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {}
  void DrawCircle(float x, float y, float radius,
                  tttext::Painter* painter) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kDrawCircle));
    stream_.WriteType(x);
    stream_.WriteType(y);
    stream_.WriteType(radius);
    WritePaint(painter);
  }
  void DrawArc(float left, float top, float right, float bottom,
               float startAngle, float sweepAngle, bool useCenter,
               tttext::Painter* painter) override {}
  void DrawPath(tttext::Path* path, tttext::Painter* painter) override {}

  void DrawArcTo(float start_x, float start_y, float mid_x, float mid_y,
                 float end_x, float end_y, float radius,
                 tttext::Painter* painter) override {}

  void DrawText(const tttext::ITypefaceHelper* font, const char* text,
                uint32_t text_bytes, float x, float y,
                tttext::Painter* painter) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kDrawText));
    stream_.WriteInt64(font->GetUniqueId());
    stream_.WriteType(x);
    stream_.WriteType(y);
    WritePaint(painter);
  }
  void DrawGlyphs(const tttext::ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text, uint32_t text_bytes,
                  float ox, float oy, float* x, float* y,
                  tttext::Painter* painter) override {
    if (glyph_count == 0)
      return;
    stream_.WriteType((uint8_t)tttext::CanvasOp::kDrawText);
    stream_.WriteType(font->GetUniqueId());
    stream_.WriteInt32(glyph_count);
    bool rtl = text_bytes == 1;
    for (uint32_t k = 0; k < glyph_count; k++) {
      auto rk = rtl ? glyph_count - 1 - k : k;
      stream_.WriteType(glyphs[rk]);
    }
    stream_.WriteType(x[0] + ox);
    stream_.WriteType(oy);
    WritePaint(painter);
  }
  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override {
    //    auto* java_delegate = (TTTextRunDelegate*)delegate;
    //    stream_.WriteInt32((uint8_t)tttext::CanvasOp::kDrawRunDelegate);
    //    stream_.WriteInt32(java_delegate->GetID());
    //    stream_.WriteFloat(left);
    //    stream_.WriteFloat(top);
    //    stream_.WriteFloat(right);
    //    stream_.WriteFloat(bottom);
    //    WritePaint(painter);
  }
  void DrawBackgroundDelegate(const tttext::RunDelegate* delegate,
                              tttext::Painter* painter) override {}

  void DrawImage(const char* src, float left, float top, float right,
                 float bottom, tttext::Painter* painter) override {}

  void DrawImageRect(const char* src, float src_left, float src_top,
                     float src_right, float src_bottom, float dst_left,
                     float dst_top, float dst_right, float dst_bottom,
                     tttext::Painter* painter, bool srcRectPercent) override {}

  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius, tttext::Painter* painter) override {
    stream_.WriteType(static_cast<uint8_t>(tttext::CanvasOp::kDrawRoundRect));
    stream_.WriteType(left);
    stream_.WriteType(top);
    stream_.WriteType(right);
    stream_.WriteType(bottom);
    stream_.WriteType(radius);
    WritePaint(painter);
  }

  void ClearBuffer() { stream_.Clear(); }
  BufferOutputStream& GetBuffer() { return stream_; }

 protected:
  void WritePaint(tttext::Painter* painter) {
    stream_.WriteFloat(painter->GetStrokeWidth());
    stream_.WriteInt32(painter->GetFillColor());
    stream_.WriteInt32(painter->GetStrokeColor());
    stream_.WriteFloat(painter->GetTextSize());
    stream_.WriteFloat(painter->GetTextSkew());
    int8_t flag = 0;
    if (painter->IsBold()) {
      flag = flag | (1 << 2);
    }
    if (painter->IsItalic()) {
      flag = flag | (1 << 3);
    }
    if (painter->IsUnderLine()) {
      flag = flag | (1 << 4);
    }
    flag = flag | (static_cast<int8_t>(painter->GetCap()) & 0x3);
    if (painter->GetFillGradient())
      flag |= (1 << 5);
    stream_.WriteInt8(flag);
    if (const auto& gradient = painter->GetFillGradient()) {
      stream_.WriteInt8(static_cast<int8_t>(gradient->type));
      switch (gradient->type) {
        case tttext::GradientType::kLinear:
          stream_.WriteFloat(gradient->start.x);
          stream_.WriteFloat(gradient->start.y);
          stream_.WriteFloat(gradient->end.x);
          stream_.WriteFloat(gradient->end.y);
          break;
        case tttext::GradientType::kRadial:
          stream_.WriteFloat(gradient->center.x);
          stream_.WriteFloat(gradient->center.y);
          stream_.WriteFloat(gradient->radius_x);
          stream_.WriteFloat(gradient->radius_y);
          break;
        case tttext::GradientType::kConic:
          stream_.WriteFloat(gradient->center.x);
          stream_.WriteFloat(gradient->center.y);
          stream_.WriteFloat(gradient->start_angle);
          break;
      }
      stream_.WriteInt32(static_cast<int32_t>(gradient->colors.size()));
      for (auto color : gradient->colors)
        stream_.WriteInt32(static_cast<int32_t>(color));
      for (auto position : gradient->positions)
        stream_.WriteFloat(position);
    }
  }

 protected:
  BufferOutputStream stream_{true};
};
}  // namespace serval::markdown::internal
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_INTERNAL_SERIALIZED_JAVA_CANVAS_HELPER_H_
