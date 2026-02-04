// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "testing/markdown/mock_markdown_canvas.h"

#include "base/include/string/string_utils.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_document.h"
#include "rapidjson/prettywriter.h"
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
  value.AddMember("stroke_width", painter->GetStrokeWidth(), allocator);
  value.AddMember("fill_color", painter->GetFillColor(), allocator);
  value.AddMember("stroke_color", painter->GetStrokeColor(), allocator);
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
  value.SetString(resource_loader_->family_cache_[id], allocator);
  return value;
}

std::string MockMarkdownCanvas::GetResult() const {
  rapidjson::StringBuffer s;
  rapidjson::PrettyWriter writer(s);
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
  const auto t = base::U16StringToU8(std::u16string_view(
      reinterpret_cast<const char16_t*>(glyphs), glyph_count));
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
  document_->GetInlineViewOrigin(src);
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "view", result_.GetAllocator());
  op.AddMember("id", std::string(src), result_.GetAllocator());
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

void MockMarkdownCanvas::ClipPath(MarkdownPath* path) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "clip path", result_.GetAllocator());
  op.AddMember("path", MakePath(path), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}

void MockMarkdownCanvas::DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                                            MarkdownPath* path,
                                            tttext::Painter* painter) {
  ClipPath(path);
  auto type = static_cast<MockDelegate*>(run_delegate)->type_;
  if (type == MockDelegateType::kGradient) {
    auto gradient = static_cast<MockGradient*>(run_delegate);
    rapidjson::Value op;
    op.SetObject();
    op.AddMember("op", "gradient", result_.GetAllocator());
    op.AddMember("gradient", std::string(gradient->gradient_),
                 result_.GetAllocator());
    result_.PushBack(op, result_.GetAllocator());
  } else {
    run_delegate->Draw(this, 0, 0);
  }
}

void MockMarkdownCanvas::DrawMarkdownPath(MarkdownPath* path,
                                          tttext::Painter* painter) {
  rapidjson::Value op;
  op.SetObject();
  op.AddMember("op", "draw path", result_.GetAllocator());
  op.AddMember("painter", MakePainter(painter), result_.GetAllocator());
  op.AddMember("path", MakePath(path), result_.GetAllocator());
  result_.PushBack(op, result_.GetAllocator());
}

rapidjson::Value MockMarkdownCanvas::MakePath(MarkdownPath* path) {
  rapidjson::Value p;
  p.SetArray();
  for (auto& op : path->path_ops_) {
    rapidjson::Value o;
    o.SetObject();
    switch (op.op_) {
      case MarkdownPath::kArc: {
        o.AddMember("type", "arc", result_.GetAllocator());
        auto& arc = op.data_.arc_;
        o.AddMember("center", MakePoint(arc.center_.x_, arc.center_.y_),
                    result_.GetAllocator());
        o.AddMember("radius", arc.radius_, result_.GetAllocator());
        o.AddMember("start", arc.start_angle_, result_.GetAllocator());
        o.AddMember("end", arc.end_angle_, result_.GetAllocator());
      } break;
      case MarkdownPath::kOval: {
        o.AddMember("type", "oval", result_.GetAllocator());
        auto& rect = op.data_.rect_;
        o.AddMember("rect",
                    MakeRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                             rect.GetBottom()),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kRect: {
        o.AddMember("type", "rect", result_.GetAllocator());
        auto& rect = op.data_.rect_;
        o.AddMember("rect",
                    MakeRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                             rect.GetBottom()),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kRoundRect: {
        o.AddMember("type", "round rect", result_.GetAllocator());
        auto& rect = op.data_.round_rect_;
        o.AddMember("rect",
                    MakeRect(rect.rect_.GetLeft(), rect.rect_.GetTop(),
                             rect.rect_.GetRight(), rect.rect_.GetBottom()),
                    result_.GetAllocator());
        o.AddMember("radius", MakePoint(rect.radius_x_, rect.radius_y_),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kMoveTo: {
        o.AddMember("type", "move", result_.GetAllocator());
        auto& point = op.data_.point_;
        o.AddMember("point", MakePoint(point.x_, point.y_),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kLineTo: {
        o.AddMember("type", "line", result_.GetAllocator());
        auto& point = op.data_.point_;
        o.AddMember("point", MakePoint(point.x_, point.y_),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kCubicTo: {
        o.AddMember("type", "cubic", result_.GetAllocator());
        auto& cubic = op.data_.cubic_;
        o.AddMember("c1", MakePoint(cubic.control_1_.x_, cubic.control_1_.y_),
                    result_.GetAllocator());
        o.AddMember("c2", MakePoint(cubic.control_2_.x_, cubic.control_2_.y_),
                    result_.GetAllocator());
        o.AddMember("end", MakePoint(cubic.end_.x_, cubic.end_.y_),
                    result_.GetAllocator());
      } break;
      case MarkdownPath::kQuadTo: {
        o.AddMember("type", "quad", result_.GetAllocator());
        auto& quad = op.data_.quad_;
        o.AddMember("control", MakePoint(quad.control_.x_, quad.control_.y_),
                    result_.GetAllocator());
        o.AddMember("end", MakePoint(quad.end_.x_, quad.end_.y_),
                    result_.GetAllocator());
      } break;
      default:
        break;
    }
    p.PushBack(o, result_.GetAllocator());
  }
  return p;
}
}  // namespace lynx::markdown::testing
