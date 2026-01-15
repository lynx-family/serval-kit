// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
#define THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
#include <vector>
#include "markdown/element/markdown_run_delegates.h"
#include "markdown/platform/ios/internal/markdown_canvas_callback.h"
#include "markdown/utils/markdown_definition.h"
#import "textra/platform/ios/ios_canvas_base.h"
#import "textra/run_delegate.h"

enum class MarkdownRunDelegateType : uint8_t {
  kImage = 0,
  kView,
  kBackground,
};

class MarkdownRunDelegate : public tttext::RunDelegate {
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

  float GetAscent() const override { return -desire_height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return desire_width_; }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    canvas->DrawRunDelegate(this, x, y, x + GetAdvance(),
                            y + GetDescent() - GetAscent(), nullptr);
  }

 protected:
  MarkdownRunDelegateType delegate_type_;
  float desire_width_;
  float desire_height_;
};

class MarkdownImage : public MarkdownRunDelegate {
 public:
  MarkdownImage(UIImage* image, float desire_width, float desire_height,
                float max_width, float max_height)
      : image_(nullptr),
        MarkdownRunDelegate(desire_width, desire_height,
                            MarkdownRunDelegateType::kImage) {
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

  float GetAscent() const override {
    return desire_height_ > 0 ? -desire_height_
                              : (image_ == nullptr ? 0 : -image_.size.height);
  }
  float GetAdvance() const override {
    return desire_width_ > 0 ? desire_width_
                             : (image_ == nullptr ? 0 : image_.size.width);
  }

  UIImage* GetImage() const { return image_; }

 private:
  UIImage* image_;
};

class MarkdownInlineView : public MarkdownRunDelegate {
 public:
  MarkdownInlineView(std::string id_selector, float desire_width,
                     float desire_height, float baseline)
      : id_selector_(id_selector),
        baseline_(baseline),
        MarkdownRunDelegate(desire_width, desire_height,
                            MarkdownRunDelegateType::kView) {}
  ~MarkdownInlineView() override = default;

  std::string GetIdSelector() const { return id_selector_; }
  void SetVerticalAlign(lynx::markdown::MarkdownVerticalAlign align,
                        float value, float font_size) {
    if (align == lynx::markdown::MarkdownVerticalAlign::kCenter) {
      // temporary assume text ascent = -0.9*font_size descent = 0.3*font_size
      // shift = (height2 + |ascent1| - descent1) / 2 - |ascent2|
      baseline_shift_ = (desire_height_ + 0.6 * font_size) / 2 - baseline_;
    } else if (align == lynx::markdown::MarkdownVerticalAlign::kTop) {
      baseline_shift_ = value;
    } else {
      baseline_shift_ = 0;
    }
  }

  float GetAscent() const override { return -(baseline_ + baseline_shift_); }

  float GetDescent() const override {
    return desire_height_ - (baseline_ + baseline_shift_);
  }

 private:
  std::string id_selector_;
  float baseline_{0};
  float baseline_shift_{0};
};

class MarkdownCanvas : public IOSCanvasBase {
 public:
  MarkdownCanvas() = default;
  explicit MarkdownCanvas(CGContextRef context) : IOSCanvasBase(context) {}
  ~MarkdownCanvas() override = default;

  void SetCallback(id<MarkdownCanvasCallback> callback) {
    callback_ = callback;
  }

 public:
  void Save() override;
  void Restore() override;
  void Translate(float dx, float dy) override;

  void DrawGlyphs(const ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text, uint32_t text_bytes,
                  float ox, float oy, float* x, float* y,
                  tttext::Painter* painter) override;
  void DrawRunDelegate(const tttext::RunDelegate* run_delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override;

 private:
  id<MarkdownCanvasCallback> callback_{nil};
  std::vector<lynx::markdown::PointF> translate_stack_;
  lynx::markdown::PointF translate_point_;
};
#endif  // THIRD_PARTY_MARKDOWN_IOS_MARKDOWN_CANVAS_H_
