// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_canvas_ios.h"
#include "markdown/draw/markdown_path.h"
#import "textra/platform/ios/typeface_coretext.h"

namespace serval::markdown {
namespace {

bool HasFill(tttext::Painter* painter) {
  return painter == nullptr ||
         painter->GetFillColor() != tttext::TTColor::UNDEFINED;
}

bool HasStroke(tttext::Painter* painter) {
  return painter != nullptr &&
         painter->GetStrokeColor() != tttext::TTColor::UNDEFINED &&
         painter->GetStrokeWidth() > 0;
}

std::vector<CGFloat> MakeGradientComponents(
    const serval::markdown::MarkdownLinearGradient* gradient) {
  std::vector<CGFloat> components;
  if (gradient == nullptr) {
    return components;
  }
  components.reserve(gradient->colors.size() * 4);
  for (const auto color_value : gradient->colors) {
    const tttext::TTColor color(color_value);
    components.emplace_back(color.GetRedRatio());
    components.emplace_back(color.GetGreenRatio());
    components.emplace_back(color.GetBlueRatio());
    components.emplace_back(color.GetAlphaRatio());
  }
  return components;
}

std::vector<CGFloat> MakeGradientLocations(
    const serval::markdown::MarkdownLinearGradient* gradient) {
  std::vector<CGFloat> locations;
  if (gradient == nullptr ||
      gradient->stops.size() != gradient->colors.size()) {
    return locations;
  }
  locations.reserve(gradient->stops.size());
  for (const auto stop : gradient->stops) {
    locations.emplace_back(static_cast<CGFloat>(stop));
  }
  return locations;
}

CGGradientRef CreateGradient(
    const serval::markdown::MarkdownLinearGradient* gradient) {
  if (gradient == nullptr || gradient->colors.size() < 2) {
    return nullptr;
  }
  const auto components = MakeGradientComponents(gradient);
  const auto locations = MakeGradientLocations(gradient);
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGGradientRef result = CGGradientCreateWithColorComponents(
      color_space, components.data(),
      locations.empty() ? nullptr : locations.data(), gradient->colors.size());
  CGColorSpaceRelease(color_space);
  return result;
}

void DrawLinearGradient(
    CGContextRef context, CGGradientRef cg_gradient,
    const serval::markdown::MarkdownLinearGradient* gradient) {
  if (context == nullptr || cg_gradient == nullptr || gradient == nullptr) {
    return;
  }

  const CGPoint start = CGPointMake(gradient->start.x_, gradient->start.y_);
  const CGPoint end = CGPointMake(gradient->end.x_, gradient->end.y_);
  CGContextDrawLinearGradient(
      context, cg_gradient, start, end,
      kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
}

void DrawLinearGradientInPath(
    CGContextRef context, CGGradientRef cg_gradient,
    const serval::markdown::MarkdownLinearGradient* gradient,
    CGPathRef clip_path) {
  if (context == nullptr || clip_path == nullptr) {
    return;
  }
  CGContextSaveGState(context);
  CGContextAddPath(context, clip_path);
  CGContextClip(context);
  DrawLinearGradient(context, cg_gradient, gradient);
  CGContextRestoreGState(context);
}

}  // namespace

MarkdownCanvasIOS::MarkdownCanvasIOS(CGContextRef context)
    : IOSCanvasBase(context) {}

void MarkdownCanvasIOS::Save() {
  if (context_ == nullptr)
    return;
  CGContextSaveGState(context_);
  translate_stack_.emplace_back(translate_point_);
}

void MarkdownCanvasIOS::Restore() {
  if (context_ == nullptr)
    return;
  CGContextRestoreGState(context_);
  translate_point_ = translate_stack_.back();
  translate_stack_.pop_back();
}

void MarkdownCanvasIOS::Translate(float dx, float dy) {
  if (context_ == nullptr)
    return;
  CGContextTranslateCTM(context_, dx, dy);
  translate_point_.Translate(dx, dy);
}

void MarkdownCanvasIOS::DrawRunDelegate(const tttext::RunDelegate* run_delegate,
                                        float left, float top, float right,
                                        float bottom,
                                        tttext::Painter* painter) {
  if (context_ == nullptr)
    return;
  CGRect rect = CGRectMake(left, top, right - left, bottom - top);
  auto* markdown_delegate =
      reinterpret_cast<const MarkdownRunDelegate*>(run_delegate);
  if (markdown_delegate->GetMarkdownRunDelegateType() ==
      MarkdownRunDelegateType::kView) {
    auto* inline_view_delegate =
        reinterpret_cast<const MarkdownInlineViewRunDelegate*>(
            markdown_delegate);
    [inline_view_delegate->GetHandle() setVisibility:true];
  } else if (markdown_delegate->GetMarkdownRunDelegateType() ==
             MarkdownRunDelegateType::kImage) {
    auto* m_image =
        reinterpret_cast<const MarkdownImageRunDelegate*>(markdown_delegate);
    UIImage* image = m_image->GetImage();
    if (image == nil || image.CGImage == nil || rect.size.width <= 0 ||
        rect.size.height <= 0) {
      return;
    }
    CGContextSaveGState(context_);
    CGFloat radius = static_cast<CGFloat>(m_image->GetBorderRadius());
    radius =
        std::min(radius, std::min(rect.size.width, rect.size.height) * 0.5f);
    if (radius > 0) {
      UIBezierPath* clip_path = [UIBezierPath bezierPathWithRoundedRect:rect
                                                           cornerRadius:radius];
      [clip_path addClip];
    }
    CGContextTranslateCTM(context_, rect.origin.x,
                          rect.origin.y + rect.size.height);
    CGRect image_rect = CGRectMake(0, 0, image.size.width, image.size.height);
    CGContextScaleCTM(context_, rect.size.width / image_rect.size.width,
                      -rect.size.height / image_rect.size.height);
    CGContextDrawImage(context_, image_rect, image.CGImage);
    CGContextRestoreGState(context_);
  }
}

void MarkdownCanvasIOS::ClipPath(serval::markdown::MarkdownPath* path) {
  CGPathRef p = CreatePath(path);
  CGContextAddPath(context_, p);
  CGContextClip(context_);
  CGPathRelease(p);
}

void MarkdownCanvasIOS::DrawMarkdownPath(serval::markdown::MarkdownPath* path,
                                         tttext::Painter* painter) {
  if (context_ == nullptr || path == nullptr) {
    return;
  }
  const bool has_fill = HasFill(painter);
  const bool has_stroke = HasStroke(painter);
  if (!has_fill && !has_stroke) {
    return;
  }

  CGPathRef p = CreatePath(path);
  if (has_fill) {
    if (painter != nullptr) {
      auto fill_color = painter->GetFillColor();
      CGContextSetRGBFillColor(
          context_, fill_color.GetRedRatio(), fill_color.GetGreenRatio(),
          fill_color.GetBlueRatio(), fill_color.GetAlphaRatio());
      CGContextSetTextDrawingMode(context_, kCGTextFill);
    }
    CGContextAddPath(context_, p);
    CGContextDrawPath(context_, kCGPathFill);
  }
  if (has_stroke) {
    auto stroke_color = painter->GetStrokeColor();
    CGContextSetRGBStrokeColor(
        context_, stroke_color.GetRedRatio(), stroke_color.GetGreenRatio(),
        stroke_color.GetBlueRatio(), stroke_color.GetAlphaRatio());
    CGContextSetLineWidth(context_, painter->GetStrokeWidth());
    CGContextSetTextDrawingMode(context_, kCGTextStroke);
    CGContextAddPath(context_, p);
    CGContextDrawPath(context_, kCGPathStroke);
  }
  CGPathRelease(p);
}

void MarkdownCanvasIOS::DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                                           serval::markdown::MarkdownPath* path,
                                           tttext::Painter* painter) {
  CGContextSaveGState(context_);
  CGPathRef p = CreatePath(path);
  if (painter->GetStrokeColor() != tttext::TTColor::UNDEFINED) {
    CGPathRef tmp =
        CGPathCreateCopyByStrokingPath(p, NULL, painter->GetStrokeWidth(),
                                       kCGLineCapButt, kCGLineJoinMiter, 0);
    CGPathRelease(p);
    p = tmp;
  }
  CGContextAddPath(context_, p);
  CGContextClip(context_);
  CGRect rect = CGPathGetBoundingBox(p);
  CGContextTranslateCTM(context_, rect.origin.x, rect.origin.y);
  DrawRunDelegate(run_delegate, 0, 0, rect.size.width, rect.size.height,
                  painter);
  CGContextRestoreGState(context_);
  CGPathRelease(p);
}

void MarkdownCanvasIOS::DrawLinearGradientOnRect(
    serval::markdown::MarkdownLinearGradient* gradient,
    serval::markdown::RectF rect, tttext::Painter* painter) {
  (void)painter;
  if (context_ == nullptr || gradient == nullptr) {
    return;
  }
  CGGradientRef cg_gradient = CreateGradient(gradient);
  if (cg_gradient == nullptr) {
    return;
  }

  const CGRect cg_rect = CGRectMake(rect.GetLeft(), rect.GetTop(),
                                    rect.GetWidth(), rect.GetHeight());

  CGContextSaveGState(context_);
  CGContextClipToRect(context_, cg_rect);
  DrawLinearGradient(context_, cg_gradient, gradient);
  CGContextRestoreGState(context_);
  CGGradientRelease(cg_gradient);
}

void MarkdownCanvasIOS::DrawLinearGradientOnPath(
    serval::markdown::MarkdownLinearGradient* gradient,
    serval::markdown::MarkdownPath* path, serval::markdown::RectF bounds,
    tttext::Painter* painter) {
  (void)bounds;
  if (context_ == nullptr || gradient == nullptr || path == nullptr) {
    return;
  }
  const bool has_fill = HasFill(painter);
  const bool has_stroke = HasStroke(painter);
  if (!has_fill && !has_stroke) {
    return;
  }
  CGGradientRef cg_gradient = CreateGradient(gradient);
  if (cg_gradient == nullptr) {
    return;
  }

  CGPathRef cg_path = CreatePath(path);
  if (has_fill) {
    DrawLinearGradientInPath(context_, cg_gradient, gradient, cg_path);
  }
  if (has_stroke) {
    CGPathRef stroke_path = CGPathCreateCopyByStrokingPath(
        cg_path, nullptr, painter->GetStrokeWidth(), kCGLineCapButt,
        kCGLineJoinMiter, 0);
    DrawLinearGradientInPath(context_, cg_gradient, gradient, stroke_path);
    if (stroke_path != nullptr) {
      CGPathRelease(stroke_path);
    }
  }
  CGPathRelease(cg_path);
  CGGradientRelease(cg_gradient);
}

void MarkdownCanvasIOS::AddPath(serval::markdown::MarkdownPath* path,
                                CGMutablePathRef result) {
  for (auto& op : path->path_ops_) {
    switch (op.op_) {
      case serval::markdown::MarkdownPath::PathOpType::kArc:
        CGPathAddArc(result, NULL, op.data_.arc_.center_.x_,
                     op.data_.arc_.center_.y_, op.data_.arc_.radius_,
                     op.data_.arc_.start_angle_, op.data_.arc_.end_angle_,
                     true);
        break;
      case serval::markdown::MarkdownPath::PathOpType::kOval: {
        auto rect = op.data_.rect_;
        CGPathAddEllipseInRect(result, NULL,
                               CGRectMake(rect.GetLeft(), rect.GetTop(),
                                          rect.GetWidth(), rect.GetHeight()));
        break;
      }
      case serval::markdown::MarkdownPath::PathOpType::kRect: {
        auto rect = op.data_.rect_;
        CGPathAddRect(result, NULL,
                      CGRectMake(rect.GetLeft(), rect.GetTop(), rect.GetWidth(),
                                 rect.GetHeight()));
        break;
      }
      case serval::markdown::MarkdownPath::PathOpType::kRoundRect: {
        auto& rr = op.data_.round_rect_;
        auto& rect = rr.rect_;
        float radius_x = std::min(rr.radius_x_, rect.GetWidth() / 2);
        float radius_y = std::min(rr.radius_y_, rect.GetHeight() / 2);
        CGPathAddRoundedRect(result, NULL,
                             CGRectMake(rect.GetLeft(), rect.GetTop(),
                                        rect.GetWidth(), rect.GetHeight()),
                             radius_x, radius_y);
        break;
      }
      case serval::markdown::MarkdownPath::PathOpType::kMoveTo:
        CGPathMoveToPoint(result, NULL, op.data_.point_.x_, op.data_.point_.y_);
        break;
      case serval::markdown::MarkdownPath::PathOpType::kLineTo:
        CGPathAddLineToPoint(result, NULL, op.data_.point_.x_,
                             op.data_.point_.y_);
        break;
      case serval::markdown::MarkdownPath::PathOpType::kQuadTo: {
        auto& quad = op.data_.quad_;
        CGPathAddQuadCurveToPoint(result, NULL, quad.control_.x_,
                                  quad.control_.y_, quad.end_.x_, quad.end_.y_);
        break;
      }
      case serval::markdown::MarkdownPath::PathOpType::kCubicTo: {
        auto& cubic = op.data_.cubic_;
        CGPathAddCurveToPoint(result, nullptr, cubic.control_1_.x_,
                              cubic.control_1_.y_, cubic.control_2_.x_,
                              cubic.control_2_.y_, cubic.end_.x_,
                              cubic.end_.y_);
        break;
      }
      default:
        break;
    }
  }
}

CGPathDrawingMode MarkdownCanvasIOS::ApplyPainterStyle(
    tttext::Painter* painter) {
  CGPathDrawingMode mode = kCGPathFillStroke;
  auto fill_color = painter->GetFillColor();
  auto stroke_color = painter->GetStrokeColor();
  if (fill_color != TTColor::UNDEFINED && stroke_color != TTColor::UNDEFINED) {
    mode = kCGPathFillStroke;
    CGContextSetTextDrawingMode(context_, kCGTextFillStroke);
  } else {
    if (fill_color != TTColor::UNDEFINED) {
      mode = kCGPathFill;
      CGContextSetTextDrawingMode(context_, kCGTextFill);
    }
    if (stroke_color != TTColor::UNDEFINED) {
      mode = kCGPathStroke;
      CGContextSetTextDrawingMode(context_, kCGTextStroke);
    }
  }
  if (fill_color != TTColor::UNDEFINED) {
    CGContextSetRGBFillColor(
        context_, fill_color.GetRedRatio(), fill_color.GetGreenRatio(),
        fill_color.GetBlueRatio(), fill_color.GetAlphaRatio());
  }
  if (stroke_color != TTColor::UNDEFINED) {
    CGContextSetRGBStrokeColor(
        context_, stroke_color.GetRedRatio(), stroke_color.GetGreenRatio(),
        stroke_color.GetBlueRatio(), stroke_color.GetAlphaRatio());
    CGContextSetLineWidth(context_, painter->GetStrokeWidth());
  }
  return mode;
}

CGPathRef MarkdownCanvasIOS::CreatePath(serval::markdown::MarkdownPath* path) {
  CGMutablePathRef result = CGPathCreateMutable();
  AddPath(path, result);
  return result;
}

}  // namespace serval::markdown
