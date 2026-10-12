// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/ios/internal/markdown_run_delegate_ios.h"

#include "markdown/platform/ios/internal/markdown_value_convert.h"

namespace serval::markdown {

MarkdownRunDelegate::MarkdownRunDelegate(float desire_width,
                                         float desire_height,
                                         MarkdownRunDelegateType type)
    : delegate_type_(type),
      desire_width_(desire_width),
      desire_height_(desire_height) {}

void MarkdownRunDelegate::Draw(tttext::ICanvasHelper* canvas, float x,
                               float y) {
  canvas->DrawRunDelegate(this, x, y, x + GetAdvance(),
                          y + GetDescent() - GetAscent(), nullptr);
}

MeasureResult MarkdownRunDelegate::OnMeasure(MeasureSpec spec) {
  const float width = desire_width_;
  const float height = desire_height_;
  const float baseline = desire_height_;
  return {.width_ = width, .height_ = height, .baseline_ = baseline};
}

MarkdownImageRunDelegate::MarkdownImageRunDelegate(
    CGSize image_size, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius)
    : MarkdownRunDelegate(0, 0, MarkdownRunDelegateType::kImage),
      border_radius_(border_radius > 0 ? border_radius : 0) {
  if (image_size.width == 0 || image_size.height == 0) {
    return;
  }
  float width, height;
  if (desire_width > 0 && desire_height > 0) {
    width = desire_width;
    height = desire_height;
  } else if (desire_width > 0) {
    width = desire_width;
    height = desire_width * image_size.height / image_size.width;
  } else if (desire_height > 0) {
    height = desire_height;
    width = desire_height * image_size.width / image_size.height;
  } else {
    width = image_size.width;
    height = image_size.height;
  }
  if (max_width > 0 && width > max_width) {
    height *= max_width / width;
    width = max_width;
  }
  if (max_height > 0 && height > max_height) {
    width *= max_height / height;
    height = max_height;
  }
  desire_width_ = width;
  desire_height_ = height;
}

MarkdownImageRunDelegate::MarkdownImageRunDelegate(
    UIImage* image, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius)
    : MarkdownImageRunDelegate(image.size, desire_width, desire_height,
                               max_width, max_height, border_radius) {
  if (desire_width_ != 0 && desire_height_ != 0) {
    float rate = image.size.width / desire_width_ * image.scale;
    image_ = [UIImage imageWithCGImage:image.CGImage
                                 scale:rate
                           orientation:UIImageOrientationUp];
  }
}

MarkdownImageRunDelegate::MarkdownImageRunDelegate(
    UIImageView* gif_view, float desire_width, float desire_height,
    float max_width, float max_height, float border_radius)
    : MarkdownImageRunDelegate(gif_view.image.size, desire_width, desire_height,
                               max_width, max_height, border_radius) {
  gif_view_ = gif_view;
}

MarkdownInlineViewRunDelegate::MarkdownInlineViewRunDelegate(
    id<IMarkdownPlatformViewHandle> handle)
    : MarkdownRunDelegate(0, 0, MarkdownRunDelegateType::kView),
      handle_(handle) {}

void MarkdownInlineViewRunDelegate::Align(float x, float y) {
  [handle_ align:x top:y];
}

void MarkdownInlineViewRunDelegate::SetBounds(RectF bounds) {
  bool new_visible = !bounds.IsEmpty();
  if (new_visible != visible_) {
    [handle_ setVisibility:new_visible];
    visible_ = new_visible;
  }
  if (!visible_) {
    return;
  }
  [handle_ setSize:bounds.GetWidth() height:bounds.GetHeight()];
  [handle_ setPosition:bounds.GetLeft() top:bounds.GetTop()];
}

MarkdownVerticalAlign MarkdownInlineViewRunDelegate::GetVerticalAlign() const {
  return [handle_ respondsToSelector:@selector(getVerticalAlign)]
             ? MarkdownValueConvert::ConvertVerticalAlign(
                   [handle_ getVerticalAlign])
             : MarkdownVerticalAlign::kBaseline;
}

float MarkdownInlineViewRunDelegate::GetVerticalAlignLength() const {
  return [handle_ respondsToSelector:@selector(getVerticalAlignLength)]
             ? [handle_ getVerticalAlignLength]
             : 0;
}

MeasureResult MarkdownInlineViewRunDelegate::OnMeasure(MeasureSpec spec) {
  auto result = [handle_
      measureByWidth:spec.width_
           WidthMode:static_cast<ServalMarkdownLayoutMode>(spec.width_mode_)
              Height:spec.height_
          HeightMode:static_cast<ServalMarkdownLayoutMode>(spec.height_mode_)];
  return {
      result.width,
      result.height,
      result.baseline,
  };
}

}  // namespace serval::markdown
