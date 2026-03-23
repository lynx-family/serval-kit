// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
#import <ServalMarkdown/IMarkdownPlatformViewHandle.h>
#import <UIKit/UIKit.h>
#include <vector>
#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/utils/markdown_definition.h"
#import "textra/platform/ios/ios_canvas_base.h"
#import "textra/run_delegate.h"
enum class MarkdownRunDelegateType : uint8_t {
  kImage = 0,
  kView,
  kBackground,
};

class MarkdownRunDelegate : public serval::markdown::MarkdownDrawable {
 public:
  MarkdownRunDelegate(float desire_width, float desire_height,
                      MarkdownRunDelegateType type)
      : desire_width_(desire_width),
        desire_height_(desire_height),
        delegate_type_(type) {}
  ~MarkdownRunDelegate() override = default;
  MarkdownRunDelegateType GetMarkdownRunDelegateType() const {
    return delegate_type_;
  };

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    canvas->DrawRunDelegate(this, x, y, x + GetAdvance(),
                            y + GetDescent() - GetAscent(), nullptr);
  }

 protected:
  serval::markdown::MeasureResult OnMeasure(
      serval::markdown::MeasureSpec spec) override {
    const float width = desire_width_;
    const float height = desire_height_;
    const float baseline = desire_height_;
    return {.width_ = width, .height_ = height, .baseline_ = baseline};
  }

 protected:
  MarkdownRunDelegateType delegate_type_;
  float desire_width_;
  float desire_height_;
};

class MarkdownImage : public MarkdownRunDelegate {
 public:
  MarkdownImage(UIImage* image, float desire_width, float desire_height,
                float max_width, float max_height, float border_radius)
      : image_(nullptr),
        border_radius_(border_radius > 0 ? border_radius : 0),
        MarkdownRunDelegate(0, 0, MarkdownRunDelegateType::kImage) {
    if (image != nullptr && image.size.width != 0 && image.size.height != 0) {
      float img_w = image.size.width;
      float img_h = image.size.height;
      float width, height;
      if (desire_width > 0 && desire_height > 0) {
        width = desire_width;
        height = desire_height;
      } else if (desire_width > 0) {
        width = desire_width;
        height = desire_width * img_h / img_w;
      } else if (desire_height > 0) {
        height = desire_height;
        width = desire_height * img_w / img_h;
      } else {
        width = img_w;
        height = img_h;
      }
      if (max_width > 0 && width > max_width) {
        height *= max_width / width;
        width = max_width;
      }
      if (max_height > 0 && height > max_height) {
        width *= max_height / height;
        height = max_height;
      }
      float rate = image.size.width / width * image.scale;
      desire_width_ = width;
      desire_height_ = height;
      image_ = [UIImage imageWithCGImage:image.CGImage
                                   scale:rate
                             orientation:UIImageOrientationUp];
    }
  }
  ~MarkdownImage() override = default;
  UIImage* GetImage() const { return image_; }
  float GetBorderRadius() const { return border_radius_; }

 private:
  UIImage* image_;
  float border_radius_;
};

class MarkdownInlineView : public MarkdownRunDelegate {
 public:
  MarkdownInlineView(id<IMarkdownPlatformViewHandle> handle)
      : MarkdownRunDelegate(0, 0, MarkdownRunDelegateType::kView),
        handle_(handle) {}
  ~MarkdownInlineView() override = default;
  void Align(float x, float y) override { [handle_ align:x top:y]; }
  id<IMarkdownPlatformViewHandle> GetHandle() const { return handle_; }

 protected:
  serval::markdown::MeasureResult OnMeasure(
      serval::markdown::MeasureSpec spec) override {
    auto result = [handle_
        measureByWidth:spec.width_
             WidthMode:static_cast<ServalMarkdownLayoutMode>(spec.width_mode_)
                Height:spec.height_
            HeightMode:static_cast<ServalMarkdownLayoutMode>(
                           spec.height_mode_)];
    return {
        result.width,
        result.height,
        result.baseline,
    };
  }

 private:
  id<IMarkdownPlatformViewHandle> handle_;
};

class MarkdownCanvasIOS : public IOSCanvasBase,
                          public serval::markdown::MarkdownCanvasExtend {
 public:
  explicit MarkdownCanvasIOS(CGContextRef context);
  ~MarkdownCanvasIOS() override = default;

 public:
  void Save() override;
  void Restore() override;
  void Translate(float dx, float dy) override;

  void DrawRunDelegate(const tttext::RunDelegate* run_delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override;

  void ClipPath(serval::markdown::MarkdownPath* path) override;
  void DrawDelegateOnPath(tttext::RunDelegate* run_delegate,
                          serval::markdown::MarkdownPath* path,
                          tttext::Painter* painter) override;
  void DrawMarkdownPath(serval::markdown::MarkdownPath* path,
                        tttext::Painter* painter) override;

 protected:
  void AddPath(serval::markdown::MarkdownPath* path, CGMutablePathRef result);
  CGPathRef CreatePath(serval::markdown::MarkdownPath* path);

  CGPathDrawingMode ApplyPainterStyle(tttext::Painter* painter);

 private:
  std::vector<serval::markdown::PointF> translate_stack_;
  serval::markdown::PointF translate_point_;
};
#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
