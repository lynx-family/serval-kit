// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "testing/markdown/mock_markdown_canvas.h"

#include "base/include/string/string_utils.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "testing/markdown/mock_markdown_resource_loader.h"
namespace lynx::markdown::testing {

rapidjson::Value MockMarkdownCanvas::MakeRect(float left, float top,
                                              float right, float bottom) {
  auto& allocator = result_.GetAllocator();
  rapidjson::Value value;
  value.SetObject();
  value.AddMember("left", left + context_.translate_.x_, allocator);
  value.AddMember("top", top + context_.translate_.y_, allocator);
  value.AddMember("right", right + context_.translate_.x_, allocator);
  value.AddMember("bottom", bottom + context_.translate_.y_, allocator);
  return value;
}

rapidjson::Value MockMarkdownCanvas::MakePoint(float x, float y) {
  auto& allocator = result_.GetAllocator();
  rapidjson::Value value;
  value.SetObject();
  value.AddMember("x", x + context_.translate_.x_, allocator);
  value.AddMember("y", y + context_.translate_.y_, allocator);
  return value;
}
std::string FillStyleToString(tttext::FillStyle fill_style) {
  switch (fill_style) {
    case tttext::FillStyle::kFill:
      return "fill";
    case tttext::FillStyle::kStroke:
      return "stroke";
    case tttext::FillStyle::kStrokeAndFill:
      return "both";
  }
  return "";
}
rapidjson::Value MockMarkdownCanvas::MakePainter(tttext::Painter* painter) {
  auto& allocator = result_.GetAllocator();
  rapidjson::Value value;
  value.SetObject();
  if (painter == nullptr)
    return value;
  value.AddMember("fill_style", FillStyleToString(painter->GetFillStyle()),
                  allocator);
  value.AddMember("stroke_width", painter->GetStrokeWidth(), allocator);
  value.AddMember("color", painter->GetColor(), allocator);
  value.AddMember("text_size", painter->GetTextSize(), allocator);
  value.AddMember("bold", painter->IsBold(), allocator);
  value.AddMember("italic", painter->IsItalic(), allocator);
  return value;
}
rapidjson::Value MockMarkdownCanvas::MakePoints(float* x, float* y,
                                                uint32_t count) {
  auto& allocator = result_.GetAllocator();
  rapidjson::Value value;
  value.SetArray();
  for (auto i = 0u; i < count; i++) {
    value.PushBack(MakePoint(x[i], y[i]), allocator);
  }
  return value;
}
rapidjson::Value MockMarkdownCanvas::MakeFont(uint32_t id) {
  auto& allocator = result_.GetAllocator();
  rapidjson::Value value;
  value.SetString(resource_loader_->family_cache_[id].c_str(), allocator);
  return value;
}

std::string MockMarkdownCanvas::GetResult() const {
  rapidjson::StringBuffer s;
  rapidjson::Writer writer(s);
  result_.Accept(writer);
  return s.GetString();
}

std::unique_ptr<tttext::Painter> MockMarkdownCanvas::CreatePainter() {
  return std::make_unique<tttext::Painter>();
}
void MockMarkdownCanvas::StartPaint() {}
void MockMarkdownCanvas::EndPaint() {}
void MockMarkdownCanvas::Save() {
  context_stack_.emplace_back(context_);
}
void MockMarkdownCanvas::Restore() {
  context_ = context_stack_.back();
  context_stack_.pop_back();
}
void MockMarkdownCanvas::Translate(const float dx, const float dy) {
  context_.translate_ += PointF{dx, dy};
}
void MockMarkdownCanvas::Scale(float sx, float sy) {}
void MockMarkdownCanvas::Rotate(float degrees) {}
void MockMarkdownCanvas::Skew(float sx, float sy) {}
void MockMarkdownCanvas::ClipRect(float left, float top, float right,
                                  float bottom, bool doAntiAlias) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "clip", result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::Clear() {}
void MockMarkdownCanvas::ClearRect(float left, float top, float right,
                                   float bottom) {}
void MockMarkdownCanvas::FillRect(float left, float top, float right,
                                  float bottom, uint32_t color) {}
void MockMarkdownCanvas::DrawColor(uint32_t color) {}
void MockMarkdownCanvas::DrawLine(float x1, float y1, float x2, float y2,
                                  tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "line", result_.GetAllocator());
  op.AddMember("p1", MakePoint(x1, y1), result_.GetAllocator());
  op.AddMember("p2", MakePoint(x2, y2), result_.GetAllocator());
  op.AddMember("painter", MakePainter(painter), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawRect(float left, float top, float right,
                                  float bottom, tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "rect", result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  op.AddMember("painter", MakePainter(painter), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawOval(float left, float top, float right,
                                  float bottom, tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawCircle(float x, float y, float radius,
                                    tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "circle", result_.GetAllocator());
  op.AddMember("center", MakePoint(x, y), result_.GetAllocator());
  op.AddMember("radius", radius, result_.GetAllocator());
  op.AddMember("painter", MakePainter(painter), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawArc(float left, float top, float right,
                                 float bottom, float startAngle,
                                 float sweepAngle, bool useCenter,
                                 tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawPath(tttext::Path* path,
                                  tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawArcTo(float start_x, float start_y, float mid_x,
                                   float mid_y, float end_x, float end_y,
                                   float radius, tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawText(const tttext::ITypefaceHelper* font,
                                  const char* text, uint32_t text_bytes,
                                  float x, float y, tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawGlyphs(const tttext::ITypefaceHelper* font,
                                    uint32_t glyph_count,
                                    const uint16_t* glyphs, const char* text,
                                    uint32_t text_bytes, float origin_x,
                                    float origin_y, float* x, float* y,
                                    tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "glyphs", result_.GetAllocator());
  const auto t =
      base::Utf16ToUtf8(reinterpret_cast<const char16_t*>(glyphs), glyph_count);
  op.AddMember("text", t, result_.GetAllocator());
  op.AddMember("font", MakeFont(font->GetUniqueId()), result_.GetAllocator());
  op.AddMember("origin", MakePoint(origin_x, origin_y), result_.GetAllocator());
  op.AddMember("painter", MakePainter(painter), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawRunDelegate(const tttext::RunDelegate* delegate,
                                         float left, float top, float right,
                                         float bottom,
                                         tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawBackgroundDelegate(
    const tttext::RunDelegate* delegate, tttext::Painter* painter) {}
void MockMarkdownCanvas::DrawImage(const char* src, float left, float top,
                                   float right, float bottom,
                                   tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "image", result_.GetAllocator());
  op.AddMember("src", std::string(src), result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawView(const char* src, float left, float top,
                                  float right, float bottom) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "view", result_.GetAllocator());
  op.AddMember("id", std::string(src), result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::DrawBackground(const char* content, float left,
                                        float top, float right, float bottom) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "background", result_.GetAllocator());
  op.AddMember("content", std::string(content), result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}

void MockMarkdownCanvas::DrawImageRect(
    const char* src, float src_left, float src_top, float src_right,
    float src_bottom, float dst_left, float dst_top, float dst_right,
    float dst_bottom, tttext::Painter* painter, bool srcRectPercent) {}
void MockMarkdownCanvas::DrawRoundRect(float left, float top, float right,
                                       float bottom, float radius,
                                       tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "round rect", result_.GetAllocator());
  op.AddMember("radius", radius, result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}
void MockMarkdownCanvas::ClipRoundRect(float left, float top, float right,
                                       float bottom, float radiusX,
                                       float radiusY, bool doAntiAlias) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "clip round rect", result_.GetAllocator());
  op.AddMember("radiusX", radiusX, result_.GetAllocator());
  op.AddMember("radiusY", radiusY, result_.GetAllocator());
  op.AddMember("rect", MakeRect(left, top, right, bottom),
               result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}

}  // namespace lynx::markdown::testing
