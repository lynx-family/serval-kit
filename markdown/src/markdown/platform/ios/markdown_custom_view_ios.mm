// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "markdown/platform/ios/internal/markdown_custom_view_ios.h"
#include "markdown/platform/ios/internal/markdown_canvas.h"

@interface MarkdownCustomViewImpl () {
  lynx::markdown::MarkdownCustomViewIOS* _markdownViewHandle;
}

@end

@implementation MarkdownCustomViewImpl
- (instancetype)init {
  self = [super init];
  if (self != nil) {
    [self initHandle];
  }
  return self;
}
- (void)initHandle {
  _markdownViewHandle = new lynx::markdown::MarkdownCustomViewIOS(self);
}

- (void)setMarkdownViewHandle:
    (lynx::markdown::MarkdownCustomViewIOS*)markdownViewHandle {
  _markdownViewHandle = markdownViewHandle;
}

- (lynx::markdown::MarkdownCustomViewIOS*)markdownViewHandle {
  return _markdownViewHandle;
}

- (void)dealloc {
  if (_markdownViewHandle != nullptr) {
    delete _markdownViewHandle;
  }
}

- (void)requestMeasure {
  [self setNeedsLayout];
}

- (void)layoutSubviews {
  [self measureByWidth:self.frame.size.width
             WidthMode:kServalMarkdownLayoutModeDefinite
                Height:self.frame.size.height
            HeightMode:kServalMarkdownLayoutModeDefinite];
}

- (CGSize)measureByWidth:(CGFloat)width
               WidthMode:(ServalMarkdownLayoutMode)widthMode
                  Height:(CGFloat)height
              HeightMode:(ServalMarkdownLayoutMode)heightMode {
  lynx::markdown::MeasureSpec spec;
  spec.width_ = width;
  spec.height_ = height;
  spec.width_mode_ = static_cast<tttext::LayoutMode>(widthMode);
  spec.height_mode_ = static_cast<tttext::LayoutMode>(heightMode);
  self.markdownViewHandle->Measure(spec);
  return self.frame.size;
}

- (void)drawRect:(CGRect)rect {
}
- (void)displayLayer:(CALayer*)layer {
  layer.contentsGravity =
      layer.contentsAreFlipped ? kCAGravityBottomLeft : kCAGravityTopLeft;
  CGSize size = self.frame.size;
  CGFloat scale = [UIScreen mainScreen].scale;
  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  size.width *= scale;
  size.height *= scale;
  size_t context_width = ceil(size.width);
  size_t context_height = ceil(size.height);
  CGContextRef context = CGBitmapContextCreate(
      NULL, context_width, context_height, 8, 4 * context_width, color_space,
      kCGImageAlphaPremultipliedLast);
  CGContextScaleCTM(context, 1, -1);
  CGContextTranslateCTM(context, 0, -ceil(size.height));
  CGContextScaleCTM(context, scale, scale);
  CGColorSpaceRelease(color_space);
  CGContextClearRect(context, CGRectMake(0, 0, size.width, size.height));
  CGContextSaveGState(context);
  CGContextClipToRect(context, CGRectMake(0, 0, size.width, size.height));

  MarkdownCanvas canvas(context);
  self.markdownViewHandle->GetDrawable()->Draw(&canvas, 0, 0);

  CGImageRef image = CGBitmapContextCreateImage(context);
  CGContextRestoreGState(context);
  CGContextRelease(context);
  id result = (__bridge_transfer id)image;
  layer.contents = result;
}
@end

namespace lynx::markdown {
MarkdownCustomViewIOS::MarkdownCustomViewIOS(MarkdownCustomViewImpl* view)
    : MarkdownPlatformViewIOS(view) {}
void MarkdownCustomViewIOS::RequestLayout() {
  [((MarkdownCustomViewImpl*)view_) requestMeasure];
}
void MarkdownCustomViewIOS::Measure(MeasureSpec spec) {
  auto size = drawable_->Measure(spec);
  SetMeasuredSize(size);
}
void MarkdownCustomViewIOS::Align(float left, float top) {
  SetAlignPosition({left, top});
}
}  // namespace lynx::markdown
