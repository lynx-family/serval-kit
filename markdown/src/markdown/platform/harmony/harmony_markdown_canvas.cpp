// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_point.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>
#include <native_drawing/drawing_shader_effect.h>

#include <vector>

#include "markdown/draw/markdown_path.h"

namespace serval::markdown {
namespace {

bool HasFill(tttext::Painter* painter) {
  return painter != nullptr &&
         painter->GetFillColor() != tttext::TTColor::UNDEFINED;
}

bool HasStroke(tttext::Painter* painter) {
  return painter != nullptr &&
         painter->GetStrokeColor() != tttext::TTColor::UNDEFINED &&
         painter->GetStrokeWidth() > 0;
}

void AttachPainter(OH_Drawing_Canvas* canvas, tttext::Painter* painter,
                   OH_Drawing_Brush* brush, OH_Drawing_Pen* pen) {
  if (canvas == nullptr) {
    return;
  }
  if (HasFill(painter) && brush != nullptr) {
    OH_Drawing_BrushSetColor(brush, painter->GetFillColor());
    OH_Drawing_CanvasAttachBrush(canvas, brush);
  }
  if (HasStroke(painter) && pen != nullptr) {
    OH_Drawing_PenSetColor(pen, painter->GetStrokeColor());
    OH_Drawing_PenSetWidth(pen, painter->GetStrokeWidth());
    OH_Drawing_CanvasAttachPen(canvas, pen);
  }
}

void AttachGradientPainter(OH_Drawing_Canvas* canvas, tttext::Painter* painter,
                           OH_Drawing_ShaderEffect* shader,
                           OH_Drawing_Brush* brush, OH_Drawing_Pen* pen) {
  if (canvas == nullptr || shader == nullptr) {
    return;
  }
  if (HasFill(painter) && brush != nullptr) {
    OH_Drawing_BrushSetShaderEffect(brush, shader);
    OH_Drawing_CanvasAttachBrush(canvas, brush);
  }
  if (HasStroke(painter) && pen != nullptr) {
    OH_Drawing_PenSetWidth(pen, painter->GetStrokeWidth());
    OH_Drawing_PenSetShaderEffect(pen, shader);
    OH_Drawing_CanvasAttachPen(canvas, pen);
  }
}

void DetachPainter(OH_Drawing_Canvas* canvas, tttext::Painter* painter) {
  if (canvas == nullptr) {
    return;
  }
  if (HasFill(painter)) {
    OH_Drawing_CanvasDetachBrush(canvas);
  }
  if (HasStroke(painter)) {
    OH_Drawing_CanvasDetachPen(canvas);
  }
}

void AppendPathOp(OH_Drawing_Path* drawing_path,
                  const MarkdownPath::PathOp& op) {
  switch (op.op_) {
    case MarkdownPath::kArc: {
      const auto& arc = op.data_.arc_;
      auto* rect = OH_Drawing_RectCreate(
          arc.center_.x_ - arc.radius_, arc.center_.y_ - arc.radius_,
          arc.center_.x_ + arc.radius_, arc.center_.y_ + arc.radius_);
      OH_Drawing_PathAddArc(drawing_path, rect, arc.start_angle_,
                            arc.end_angle_ - arc.start_angle_);
      OH_Drawing_RectDestroy(rect);
      break;
    }
    case MarkdownPath::kOval: {
      const auto& rect = op.data_.rect_;
      auto* drawing_rect = OH_Drawing_RectCreate(
          rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom());
      OH_Drawing_PathAddOval(drawing_path, drawing_rect, PATH_DIRECTION_CW);
      OH_Drawing_RectDestroy(drawing_rect);
      break;
    }
    case MarkdownPath::kRect: {
      const auto& rect = op.data_.rect_;
      OH_Drawing_PathAddRect(drawing_path, rect.GetLeft(), rect.GetTop(),
                             rect.GetRight(), rect.GetBottom(),
                             PATH_DIRECTION_CW);
      break;
    }
    case MarkdownPath::kRoundRect: {
      const auto& rr = op.data_.round_rect_;
      auto* rect =
          OH_Drawing_RectCreate(rr.rect_.GetLeft(), rr.rect_.GetTop(),
                                rr.rect_.GetRight(), rr.rect_.GetBottom());
      auto* round_rect =
          OH_Drawing_RoundRectCreate(rect, rr.radius_x_, rr.radius_y_);
      OH_Drawing_PathAddRoundRect(drawing_path, round_rect, PATH_DIRECTION_CW);
      OH_Drawing_RoundRectDestroy(round_rect);
      OH_Drawing_RectDestroy(rect);
      break;
    }
    case MarkdownPath::kMoveTo:
      OH_Drawing_PathMoveTo(drawing_path, op.data_.point_.x_,
                            op.data_.point_.y_);
      break;
    case MarkdownPath::kLineTo:
      OH_Drawing_PathLineTo(drawing_path, op.data_.point_.x_,
                            op.data_.point_.y_);
      break;
    case MarkdownPath::kCubicTo: {
      const auto& cubic = op.data_.cubic_;
      OH_Drawing_PathCubicTo(drawing_path, cubic.control_1_.x_,
                             cubic.control_1_.y_, cubic.control_2_.x_,
                             cubic.control_2_.y_, cubic.end_.x_, cubic.end_.y_);
      break;
    }
    case MarkdownPath::kQuadTo: {
      const auto& quad = op.data_.quad_;
      OH_Drawing_PathQuadTo(drawing_path, quad.control_.x_, quad.control_.y_,
                            quad.end_.x_, quad.end_.y_);
      break;
    }
  }
}

OH_Drawing_Path* CreateDrawingPath(MarkdownPath* path) {
  if (path == nullptr) {
    return nullptr;
  }
  auto* result = OH_Drawing_PathCreate();
  for (const auto& op : path->path_ops_) {
    AppendPathOp(result, op);
  }
  return result;
}

OH_Drawing_Path* CreateClipPath(OH_Drawing_Path* path,
                                tttext::Painter* painter) {
  if (path == nullptr) {
    return nullptr;
  }
  if (!HasStroke(painter)) {
    return OH_Drawing_PathCopy(path);
  }

  auto* pen = OH_Drawing_PenCreate();
  OH_Drawing_PenSetWidth(pen, painter->GetStrokeWidth());
  OH_Drawing_PenSetColor(pen, painter->GetStrokeColor());

  auto* stroke_path = OH_Drawing_PathCreate();
  const bool has_stroke_path =
      OH_Drawing_PenGetFillPath(pen, path, stroke_path, nullptr, nullptr);
  OH_Drawing_PenDestroy(pen);
  if (!has_stroke_path) {
    OH_Drawing_PathDestroy(stroke_path);
    return OH_Drawing_PathCopy(path);
  }
  if (!HasFill(painter)) {
    return stroke_path;
  }

  auto* result = OH_Drawing_PathCopy(path);
  if (result != nullptr) {
    OH_Drawing_PathOp(result, stroke_path, PATH_OP_MODE_UNION);
  }
  OH_Drawing_PathDestroy(stroke_path);
  return result;
}

OH_Drawing_ShaderEffect* CreateLinearGradientShader(
    MarkdownLinearGradient* gradient) {
  if (gradient == nullptr || gradient->colors.size() < 2) {
    return nullptr;
  }
  auto* start = OH_Drawing_PointCreate(gradient->start.x_, gradient->start.y_);
  auto* end = OH_Drawing_PointCreate(gradient->end.x_, gradient->end.y_);
  auto* shader = OH_Drawing_ShaderEffectCreateLinearGradient(
      start, end, gradient->colors.data(),
      gradient->stops.size() == gradient->colors.size() ? gradient->stops.data()
                                                        : nullptr,
      static_cast<uint32_t>(gradient->colors.size()), CLAMP);
  OH_Drawing_PointDestroy(start);
  OH_Drawing_PointDestroy(end);
  return shader;
}

}  // namespace

void HarmonyMarkdownCanvas::ClipPath(MarkdownPath* path) {
  auto* drawing_path = CreateDrawingPath(path);
  if (drawing_path == nullptr) {
    return;
  }
  OH_Drawing_CanvasClipPath(canvas_, drawing_path, INTERSECT, true);
  OH_Drawing_PathDestroy(drawing_path);
}

void HarmonyMarkdownCanvas::DrawMarkdownPath(MarkdownPath* path,
                                             tttext::Painter* painter) {
  auto* drawing_path = CreateDrawingPath(path);
  if (drawing_path == nullptr) {
    return;
  }
  auto* brush = OH_Drawing_BrushCreate();
  auto* pen = OH_Drawing_PenCreate();
  AttachPainter(canvas_, painter, brush, pen);
  OH_Drawing_CanvasDrawPath(canvas_, drawing_path);
  DetachPainter(canvas_, painter);
  OH_Drawing_BrushDestroy(brush);
  OH_Drawing_PenDestroy(pen);
  OH_Drawing_PathDestroy(drawing_path);
}

void HarmonyMarkdownCanvas::DrawDelegateOnPath(
    tttext::RunDelegate* run_delegate, MarkdownPath* path,
    tttext::Painter* painter) {
  if (run_delegate == nullptr) {
    return;
  }
  auto* drawing_path = CreateDrawingPath(path);
  if (drawing_path == nullptr) {
    return;
  }
  auto* clip_path = CreateClipPath(drawing_path, painter);
  OH_Drawing_CanvasSave(canvas_);
  if (clip_path != nullptr) {
    OH_Drawing_CanvasClipPath(canvas_, clip_path, INTERSECT, true);
  }
  run_delegate->Draw(this, 0, 0);
  OH_Drawing_CanvasRestore(canvas_);
  if (clip_path != nullptr) {
    OH_Drawing_PathDestroy(clip_path);
  }
  OH_Drawing_PathDestroy(drawing_path);
}

void HarmonyMarkdownCanvas::DrawLinearGradientOnRect(
    MarkdownLinearGradient* gradient, RectF rect, tttext::Painter* painter) {
  auto* shader = CreateLinearGradientShader(gradient);
  if (shader == nullptr) {
    return;
  }
  auto* brush = OH_Drawing_BrushCreate();
  auto* pen = OH_Drawing_PenCreate();
  AttachGradientPainter(canvas_, painter, shader, brush, pen);
  auto* drawing_rect = OH_Drawing_RectCreate(rect.GetLeft(), rect.GetTop(),
                                             rect.GetRight(), rect.GetBottom());
  OH_Drawing_CanvasDrawRect(canvas_, drawing_rect);
  DetachPainter(canvas_, painter);
  OH_Drawing_RectDestroy(drawing_rect);
  OH_Drawing_BrushDestroy(brush);
  OH_Drawing_PenDestroy(pen);
  OH_Drawing_ShaderEffectDestroy(shader);
}

void HarmonyMarkdownCanvas::DrawLinearGradientOnPath(
    MarkdownLinearGradient* gradient, MarkdownPath* path,
    tttext::Painter* painter) {
  auto* shader = CreateLinearGradientShader(gradient);
  auto* drawing_path = CreateDrawingPath(path);
  if (shader == nullptr || drawing_path == nullptr) {
    if (shader != nullptr) {
      OH_Drawing_ShaderEffectDestroy(shader);
    }
    if (drawing_path != nullptr) {
      OH_Drawing_PathDestroy(drawing_path);
    }
    return;
  }
  auto* brush = OH_Drawing_BrushCreate();
  auto* pen = OH_Drawing_PenCreate();
  AttachGradientPainter(canvas_, painter, shader, brush, pen);
  OH_Drawing_CanvasDrawPath(canvas_, drawing_path);
  DetachPainter(canvas_, painter);
  OH_Drawing_BrushDestroy(brush);
  OH_Drawing_PenDestroy(pen);
  OH_Drawing_PathDestroy(drawing_path);
  OH_Drawing_ShaderEffectDestroy(shader);
}
}  // namespace serval::markdown
