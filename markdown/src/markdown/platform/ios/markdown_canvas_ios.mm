// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_canvas_ios.h"
#include "markdown/draw/markdown_path.h"
#import "textra/platform/ios/typeface_coretext.h"

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
        reinterpret_cast<const MarkdownInlineView*>(markdown_delegate);
    [callback_
        SetInlineViewVisible:[NSString
                                 stringWithUTF8String:inline_view_delegate
                                                          ->GetIdSelector()
                                                          .c_str()]];
  } else if (markdown_delegate->GetMarkdownRunDelegateType() ==
             MarkdownRunDelegateType::kImage) {
    auto* m_image = reinterpret_cast<const MarkdownImage*>(markdown_delegate);
    UIImage* image = m_image->GetImage();
    CGContextSaveGState(context_);
    CGContextTranslateCTM(context_, rect.origin.x,
                          rect.origin.y + rect.size.height);
    CGRect image_rect = CGRectMake(0, 0, image.size.width, image.size.height);
    CGContextScaleCTM(context_, rect.size.width / image_rect.size.width,
                      -rect.size.height / image_rect.size.height);
    CGContextDrawImage(context_, image_rect, image.CGImage);
    CGContextRestoreGState(context_);
  }
}

void MarkdownCanvasIOS::ClipPath(lynx::markdown::MarkdownPath* path) {
  CGPathRef p = CreatePath(path);
  CGContextAddPath(context_, p);
  CGContextClip(context_);
  CGPathRelease(p);
}

void MarkdownCanvasIOS::DrawMarkdownPath(lynx::markdown::MarkdownPath* path,
                                         tttext::Painter* painter) {
  CGPathRef p = CreatePath(path);
  CGContextAddPath(context_, p);
  CGContextDrawPath(context_, ApplyPainterStyle(painter));
  CGPathRelease(p);
}

void MarkdownCanvasIOS::DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                                           lynx::markdown::MarkdownPath* path,
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

void MarkdownCanvasIOS::AddPath(lynx::markdown::MarkdownPath* path,
                                CGMutablePathRef result) {
  for (auto& op : path->path_ops_) {
    switch (op.op_) {
      case lynx::markdown::MarkdownPath::PathOpType::kArc:
        CGPathAddArc(result, NULL, op.data_.arc_.center_.x_,
                     op.data_.arc_.center_.y_, op.data_.arc_.radius_,
                     op.data_.arc_.start_angle_, op.data_.arc_.end_angle_,
                     true);
        break;
      case lynx::markdown::MarkdownPath::PathOpType::kOval: {
        auto rect = op.data_.rect_;
        CGPathAddEllipseInRect(result, NULL,
                               CGRectMake(rect.GetLeft(), rect.GetTop(),
                                          rect.GetWidth(), rect.GetHeight()));
        break;
      }
      case lynx::markdown::MarkdownPath::PathOpType::kRect: {
        auto rect = op.data_.rect_;
        CGPathAddRect(result, NULL,
                      CGRectMake(rect.GetLeft(), rect.GetTop(), rect.GetWidth(),
                                 rect.GetHeight()));
        break;
      }
      case lynx::markdown::MarkdownPath::PathOpType::kRoundRect: {
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
      case lynx::markdown::MarkdownPath::PathOpType::kMoveTo:
        CGPathMoveToPoint(result, NULL, op.data_.point_.x_, op.data_.point_.y_);
        break;
      case lynx::markdown::MarkdownPath::PathOpType::kLineTo:
        CGPathAddLineToPoint(result, NULL, op.data_.point_.x_,
                             op.data_.point_.y_);
        break;
      case lynx::markdown::MarkdownPath::PathOpType::kQuadTo: {
        auto& quad = op.data_.quad_;
        CGPathAddQuadCurveToPoint(result, NULL, quad.control_.x_,
                                  quad.control_.y_, quad.end_.x_, quad.end_.y_);
        break;
      }
      case lynx::markdown::MarkdownPath::PathOpType::kCubicTo: {
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

CGPathRef MarkdownCanvasIOS::CreatePath(lynx::markdown::MarkdownPath* path) {
  CGMutablePathRef result = CGPathCreateMutable();
  AddPath(path, result);
  return result;
}
