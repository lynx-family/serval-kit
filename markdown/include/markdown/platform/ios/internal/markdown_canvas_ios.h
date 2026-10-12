// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_CANVAS_IOS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_CANVAS_IOS_H_
#import <UIKit/UIKit.h>
#include <vector>
#include "markdown/draw/markdown_canvas.h"
#import "textra/platform/ios/ios_canvas_base.h"

NS_ASSUME_NONNULL_BEGIN

namespace serval::markdown {

class MarkdownCanvasIOS : public IOSCanvasBase, public MarkdownCanvasExtend {
 public:
  MarkdownCanvasIOS(CGContextRef _Nullable context, UIView* view);
  ~MarkdownCanvasIOS() override = default;
  NSSet<UIView*>* GetGifViews() const { return gif_views_; }

 public:
  void Save() override;
  void Restore() override;
  void Translate(float dx, float dy) override;

  void DrawRunDelegate(const tttext::RunDelegate* run_delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* _Nullable painter) override;

  void ClipPath(MarkdownPath* path) override;
  void DrawDelegateOnPath(tttext::RunDelegate* run_delegate, MarkdownPath* path,
                          tttext::Painter* painter) override;
  void DrawMarkdownPath(serval::markdown::MarkdownPath* path,
                        tttext::Painter* _Nullable painter) override;
  void DrawLinearGradientOnRect(
      serval::markdown::MarkdownLinearGradient* gradient,
      serval::markdown::RectF rect,
      tttext::Painter* _Nullable painter) override;
  void DrawLinearGradientOnPath(
      serval::markdown::MarkdownLinearGradient* gradient,
      serval::markdown::MarkdownPath* path, serval::markdown::RectF bounds,
      tttext::Painter* _Nullable painter) override;

 protected:
  void AddPath(MarkdownPath* path, CGMutablePathRef result);
  CGPathRef CreatePath(MarkdownPath* path);

  CGPathDrawingMode ApplyPainterStyle(tttext::Painter* painter);

 private:
  UIView* view_;
  NSMutableSet<UIView*>* gif_views_;
  std::vector<PointF> translate_stack_;
  PointF translate_point_;
};

}  // namespace serval::markdown

NS_ASSUME_NONNULL_END

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_IOS_INTERNAL_MARKDOWN_CANVAS_IOS_H_
