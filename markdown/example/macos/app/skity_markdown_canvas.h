// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_MACOS_APP_SKITY_MARKDOWN_CANVAS_H_
#define MARKDOWN_EXAMPLE_MACOS_APP_SKITY_MARKDOWN_CANVAS_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/draw/markdown_path.h"
#include "skity/skity.hpp"
#include "textra/platform/skity/skity_canvas_helper.h"

namespace serval::markdown::example {

class SkityMarkdownCanvas final : public tttext::SkityCanvasHelper,
                                  public MarkdownCanvasExtend {
 public:
  explicit SkityMarkdownCanvas(skity::Canvas* canvas)
      : tttext::SkityCanvasHelper(canvas), canvas_(canvas) {}
  ~SkityMarkdownCanvas() override = default;

  void Clear() override { canvas_->Clear(skity::Color_TRANSPARENT); }
  void ClearRect(float left, float top, float right, float bottom) override {
    auto paint = MakeFillPaint(skity::Color_TRANSPARENT);
    paint.SetBlendMode(skity::BlendMode::kSrc);
    canvas_->DrawRect(ToSkityRect(left, top, right, bottom), paint);
  }
  void FillRect(float left, float top, float right, float bottom,
                uint32_t color) override {
    canvas_->DrawRect(ToSkityRect(left, top, right, bottom),
                      MakeFillPaint(color));
  }
  void DrawOval(float left, float top, float right, float bottom,
                tttext::Painter* painter) override {
    canvas_->DrawOval(ToSkityRect(left, top, right, bottom),
                      *ToSkityPaint(painter));
  }
  void DrawCircle(float x, float y, float radius,
                  tttext::Painter* painter) override {
    canvas_->DrawCircle(x, y, radius, *ToSkityPaint(painter));
  }
  void DrawArc(float left, float top, float right, float bottom,
               float start_angle, float sweep_angle, bool use_center,
               tttext::Painter* painter) override {
    canvas_->DrawArc(ToSkityRect(left, top, right, bottom), start_angle,
                     sweep_angle, use_center, *ToSkityPaint(painter));
  }
  void DrawRoundRect(float left, float top, float right, float bottom,
                     float radius, tttext::Painter* painter) override {
    canvas_->DrawRoundRect(ToSkityRect(left, top, right, bottom), radius,
                           radius, *ToSkityPaint(painter));
  }
  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override {
    (void)right;
    (void)bottom;
    (void)painter;
    if (delegate == nullptr) {
      return;
    }
    const_cast<tttext::RunDelegate*>(delegate)->Draw(this, left, top);
  }

  void ClipPath(MarkdownPath* path) override {
    canvas_->ClipPath(ToSkityPath(path));
  }

  void DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                          MarkdownPath* path,
                          tttext::Painter* painter) override {
    if (run_delegate == nullptr || path == nullptr) {
      return;
    }
    canvas_->Save();
    const auto skity_path = ToSkityPath(path);
    canvas_->ClipPath(skity_path);
    const auto bounds = PathBounds(path);
    DrawRunDelegate(run_delegate, bounds.GetLeft(), bounds.GetTop(),
                    bounds.GetRight(), bounds.GetBottom(), painter);
    canvas_->Restore();
  }

  void DrawMarkdownPath(MarkdownPath* path, tttext::Painter* painter) override {
    if (path == nullptr) {
      return;
    }
    canvas_->DrawPath(ToSkityPath(path), *ToSkityPaint(painter));
  }

  void DrawLinearGradientOnRect(MarkdownLinearGradient* gradient, RectF rect,
                                tttext::Painter* painter) override {
    auto paint = MakeGradientPaint(gradient, painter);
    canvas_->DrawRect(ToSkityRect(rect), paint);
  }

  void DrawLinearGradientOnPath(MarkdownLinearGradient* gradient,
                                MarkdownPath* path, RectF bounds,
                                tttext::Painter* painter) override {
    (void)bounds;
    if (path == nullptr) {
      return;
    }
    auto paint = MakeGradientPaint(gradient, painter);
    canvas_->DrawPath(ToSkityPath(path), paint);
  }

 private:
  static skity::Rect ToSkityRect(float left, float top, float right,
                                 float bottom) {
    return skity::Rect::MakeLTRB(left, top, right, bottom);
  }

  static skity::Rect ToSkityRect(RectF rect) {
    return ToSkityRect(rect.GetLeft(), rect.GetTop(), rect.GetRight(),
                       rect.GetBottom());
  }

  static skity::Color4f ToSkityColor4f(uint32_t color_value) {
    const tttext::TTColor color(color_value);
    return {color.GetRedRatio(), color.GetGreenRatio(), color.GetBlueRatio(),
            color.GetAlphaRatio()};
  }

  static skity::Paint MakeFillPaint(uint32_t color) {
    skity::Paint paint;
    paint.SetAntiAlias(true);
    paint.SetStyle(skity::Paint::kFill_Style);
    paint.SetColor(skity::Color4fToColor(ToSkityColor4f(color)));
    return paint;
  }

  static skity::Path ToSkityPath(MarkdownPath* path) {
    skity::Path result;
    if (path == nullptr) {
      return result;
    }
    for (const auto& op : path->path_ops_) {
      switch (op.op_) {
        case MarkdownPath::PathOpType::kArc: {
          const auto& arc = op.data_.arc_;
          const auto rect = skity::Rect::MakeLTRB(
              arc.center_.x_ - arc.radius_, arc.center_.y_ - arc.radius_,
              arc.center_.x_ + arc.radius_, arc.center_.y_ + arc.radius_);
          result.ArcTo(rect, arc.start_angle_,
                       arc.end_angle_ - arc.start_angle_, false);
          break;
        }
        case MarkdownPath::PathOpType::kOval:
          result.AddOval(ToSkityRect(op.data_.rect_));
          break;
        case MarkdownPath::PathOpType::kRect:
          result.AddRect(ToSkityRect(op.data_.rect_));
          break;
        case MarkdownPath::PathOpType::kRoundRect: {
          const auto& round_rect = op.data_.round_rect_;
          result.AddRoundRect(ToSkityRect(round_rect.rect_),
                              round_rect.radius_x_, round_rect.radius_y_);
          break;
        }
        case MarkdownPath::PathOpType::kMoveTo:
          result.MoveTo(op.data_.point_.x_, op.data_.point_.y_);
          break;
        case MarkdownPath::PathOpType::kLineTo:
          result.LineTo(op.data_.point_.x_, op.data_.point_.y_);
          break;
        case MarkdownPath::PathOpType::kQuadTo:
          result.QuadTo(op.data_.quad_.control_.x_,
                        op.data_.quad_.control_.y_, op.data_.quad_.end_.x_,
                        op.data_.quad_.end_.y_);
          break;
        case MarkdownPath::PathOpType::kCubicTo:
          result.CubicTo(op.data_.cubic_.control_1_.x_,
                         op.data_.cubic_.control_1_.y_,
                         op.data_.cubic_.control_2_.x_,
                         op.data_.cubic_.control_2_.y_,
                         op.data_.cubic_.end_.x_, op.data_.cubic_.end_.y_);
          break;
      }
    }
    return result;
  }

  static RectF PathBounds(MarkdownPath* path) {
    RectF bounds = RectF::MakeEmpty();
    bool has_point = false;
    auto add_point = [&](PointF point) {
      if (!has_point) {
        bounds = RectF::MakeLTRB(point.x_, point.y_, point.x_, point.y_);
        has_point = true;
        return;
      }
      bounds.Union(RectF::MakeLTRB(point.x_, point.y_, point.x_, point.y_));
    };
    for (const auto& op : path->path_ops_) {
      switch (op.op_) {
        case MarkdownPath::PathOpType::kArc:
          add_point({op.data_.arc_.center_.x_ - op.data_.arc_.radius_,
                     op.data_.arc_.center_.y_ - op.data_.arc_.radius_});
          add_point({op.data_.arc_.center_.x_ + op.data_.arc_.radius_,
                     op.data_.arc_.center_.y_ + op.data_.arc_.radius_});
          break;
        case MarkdownPath::PathOpType::kOval:
        case MarkdownPath::PathOpType::kRect:
          add_point({op.data_.rect_.GetLeft(), op.data_.rect_.GetTop()});
          add_point({op.data_.rect_.GetRight(), op.data_.rect_.GetBottom()});
          break;
        case MarkdownPath::PathOpType::kRoundRect:
          add_point({op.data_.round_rect_.rect_.GetLeft(),
                     op.data_.round_rect_.rect_.GetTop()});
          add_point({op.data_.round_rect_.rect_.GetRight(),
                     op.data_.round_rect_.rect_.GetBottom()});
          break;
        case MarkdownPath::PathOpType::kMoveTo:
        case MarkdownPath::PathOpType::kLineTo:
          add_point(op.data_.point_);
          break;
        case MarkdownPath::PathOpType::kQuadTo:
          add_point(op.data_.quad_.control_);
          add_point(op.data_.quad_.end_);
          break;
        case MarkdownPath::PathOpType::kCubicTo:
          add_point(op.data_.cubic_.control_1_);
          add_point(op.data_.cubic_.control_2_);
          add_point(op.data_.cubic_.end_);
          break;
      }
    }
    return bounds;
  }

  skity::Paint MakeGradientPaint(MarkdownLinearGradient* gradient,
                                 tttext::Painter* painter) {
    skity::Paint paint = *ToSkityPaint(painter);
    paint.SetAntiAlias(true);
    paint.SetStyle(skity::Paint::kFill_Style);
    if (gradient == nullptr || gradient->colors.size() < 2) {
      return paint;
    }

    std::vector<skity::Color4f> colors;
    colors.reserve(gradient->colors.size());
    for (const auto color : gradient->colors) {
      colors.emplace_back(ToSkityColor4f(color));
    }

    const skity::Point points[2] = {
        {gradient->start.x_, gradient->start.y_, 0.f, 1.f},
        {gradient->end.x_, gradient->end.y_, 0.f, 1.f},
    };
    const float* stops =
        gradient->stops.size() == gradient->colors.size()
            ? gradient->stops.data()
            : nullptr;
    paint.SetShader(skity::Shader::MakeLinear(
        points, colors.data(), stops, static_cast<int>(colors.size())));
    return paint;
  }

 private:
  skity::Canvas* canvas_{nullptr};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_MACOS_APP_SKITY_MARKDOWN_CANVAS_H_
