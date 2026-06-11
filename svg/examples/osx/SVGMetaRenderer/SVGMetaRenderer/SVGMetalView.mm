// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SVGMetalView.h"

#import <Foundation/Foundation.h>
#include <memory>
#include <string>
#include <vector>

#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"

#include <skity/gpu/gpu_context_mtl.h>
#include <skity/gpu/gpu_surface.hpp>
#include <skity/skity.hpp>

using serval::svg::parser::SrSVGDiagnostic;
using serval::svg::parser::SrSVGDOM;
using serval::svg::skity::SrSkityCanvas;

static bool SrSVGParseDefaultColorString(NSString* color_string,
                                         uint32_t* out_color) {
  if (color_string.length != 7 || ![color_string hasPrefix:@"#"] ||
      out_color == nullptr) {
    return false;
  }
  unsigned int color_value = 0;
  NSScanner* scanner =
      [NSScanner scannerWithString:[color_string substringFromIndex:1]];
  if (![scanner scanHexInt:&color_value]) {
    return false;
  }
  uint32_t rgb = (uint32_t)color_value;
  *out_color = ((rgb & 0x00FF0000) | (rgb & 0x0000FF00) | (rgb & 0x000000FF) |
                0xFF000000);
  return true;
}

static bool SrSVGParseSVGStringDoc(std::unique_ptr<SrSVGDOM>& svg_dom,
                                   NSString* svg_doc,
                                   std::vector<SrSVGDiagnostic>* diagnostics) {
  if (svg_doc.length == 0) {
    svg_dom.reset();
    return false;
  }
  std::string svg_cpp_string([svg_doc UTF8String]);
  svg_dom = SrSVGDOM::make(svg_cpp_string.c_str(), svg_cpp_string.length(),
                           diagnostics);
  return svg_dom != nullptr;
}

@interface SVGMetalView () {
  std::unique_ptr<SrSVGDOM> _svgDom;
  std::unique_ptr<skity::GPUContext> _gpuContext;
  std::unique_ptr<skity::GPUSurface> _gpuSurface;
}

@end

@implementation SVGMetalView

- (CAMetalLayer*)metalLayer {
  return (CAMetalLayer*)self.layer;
}

- (instancetype)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self commonInit];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  self = [super initWithCoder:coder];
  if (self) {
    [self commonInit];
  }
  return self;
}

- (void)commonInit {
  self.wantsLayer = YES;
  CAMetalLayer* metalLayer = [CAMetalLayer layer];
  metalLayer.device = MTLCreateSystemDefaultDevice();
  metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  metalLayer.opaque = NO;
  metalLayer.frame = self.bounds;
  self.layer = metalLayer;

  self.device = metalLayer.device;
  self.commandQueue = [self.device newCommandQueue];
  [self recreateSurface];
}

- (void)recreateSurface {
  if (!self.device || !self.commandQueue) {
    return;
  }

  CGFloat scale = self.window.backingScaleFactor;
  if (scale <= 0.0) {
    scale = NSScreen.mainScreen.backingScaleFactor;
  }
  if (scale <= 0.0) {
    scale = 1.0;
  }

  CAMetalLayer* metalLayer = self.metalLayer;
  metalLayer.contentsScale = scale;
  metalLayer.frame = self.bounds;
  metalLayer.drawableSize =
      NSMakeSize(NSWidth(self.bounds) * scale, NSHeight(self.bounds) * scale);

  if (!_gpuContext) {
    _gpuContext = skity::MTLContextCreate(self.device, self.commandQueue);
  }
  if (!_gpuContext || NSWidth(self.bounds) <= 0.0 ||
      NSHeight(self.bounds) <= 0.0) {
    _gpuSurface.reset();
    return;
  }

  skity::GPUSurfaceDescriptorMTL desc{};
  desc.backend = skity::GPUBackendType::kMetal;
  desc.width = NSWidth(self.bounds);
  desc.height = NSHeight(self.bounds);
  desc.content_scale = metalLayer.contentsScale;
  desc.sample_count = 4;
  desc.surface_type = skity::MTLSurfaceType::kLayer;
  desc.layer = metalLayer;

  _gpuSurface = _gpuContext->CreateSurface(&desc);
}

- (void)setSVGContent:(NSString*)content {
  std::vector<SrSVGDiagnostic> diagnostics;
  if (![content isKindOfClass:[NSString class]] || content.length == 0) {
    _svgDom.reset();
  } else {
    SrSVGParseSVGStringDoc(_svgDom, content, &diagnostics);
  }
  if (!diagnostics.empty()) {
    NSLog(@"SVGDiagnostic errorMessage=%s",
          diagnostics.front().message.c_str());
  }
  [self render];
}

- (void)loadSVGFromFile:(NSString*)path {
  NSError* error = nil;
  NSString* content = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
  if (content.length == 0) {
    NSLog(@"Failed to load SVG file at %@ error=%@", path, error);
    [self setSVGContent:nil];
    return;
  }
  [self setSVGContent:content];
}

- (void)setColor:(NSString*)color {
  NSString* normalized = nil;
  if (color.length > 0) {
    normalized = [color
        stringByTrimmingCharactersInSet:NSCharacterSet
                                            .whitespaceAndNewlineCharacterSet];
  }
  if ((_color == nil && normalized == nil) ||
      [_color isEqualToString:normalized]) {
    return;
  }
  _color = [normalized copy];
  [self render];
}

- (void)drawContentOnCanvas:(skity::Canvas*)canvas
                      width:(float)width
                     height:(float)height {
  if (!canvas) {
    return;
  }
  if (_svgDom) {
    if (self.color.length > 0) {
      uint32_t default_color = 0;
      if (parse_svg_color(self.color.UTF8String, &default_color)) {
        _svgDom->SetDefaultColor(default_color);
      } else {
        _svgDom->ResetDefaultColor();
      }
    } else {
      _svgDom->ResetDefaultColor();
    }

    SrSkityCanvas sr_canvas(
        canvas, [](std::string url) -> std::shared_ptr<skity::Image> {
          return nullptr;
        });

    SrSVGBox view_port{0.f, 0.f, width, height};
    //canvas->ClipRect(skity::Rect::MakeXYWH(0.f, 0.f, width, height));
    _svgDom->Render(&sr_canvas, view_port);
  } else {
    skity::Paint paint;
    paint.SetTextSize(20.f);
    paint.SetFillColor(0, 0, 0, 1);
    paint.SetAntiAlias(true);
    paint.SetTypeface(skity::Typeface::GetDefaultTypeface());
    skity::TextBlobBuilder builder;
    auto blob = builder.BuildTextBlob("No SVG loaded.", paint);
    canvas->DrawTextBlob(blob.get(), 24.f, 48.f, paint);
  }
}

- (std::shared_ptr<skity::Image>)makeGPUImageWithWidth:(float)width
                                                height:(float)height {
  if (!_gpuContext) {
    return nullptr;
  }

  skity::GPURenderTargetDescriptor desc;
  desc.width = width;
  desc.height = height;
  desc.sample_count = 4;
  auto render_target = _gpuContext->CreateRenderTarget(desc);
  if (!render_target) {
    return nullptr;
  }

  auto* canvas = render_target->GetCanvas();
  if (!canvas) {
    return nullptr;
  }
  canvas->DrawColor(skity::Color_WHITE);
  [self drawContentOnCanvas:canvas width:width height:height];

  return _gpuContext->MakeSnapshot(std::move(render_target));
}

- (std::shared_ptr<skity::Image>)makeSoftwareImageWithWidth:(float)width
                                                     height:(float)height {
  if (!_gpuContext) {
    return nullptr;
  }

  auto bitmap = std::make_unique<skity::Bitmap>(
      width, height, skity::AlphaType::kPremul_AlphaType);
  auto canvas = skity::Canvas::MakeSoftwareCanvas(bitmap.get());
  if (!canvas) {
    return nullptr;
  }
  canvas->DrawColor(skity::Color_WHITE);
  [self drawContentOnCanvas:canvas.get() width:width height:height];

  auto pixmap = bitmap->GetPixmap();
  if (!pixmap) {
    return nullptr;
  }

  auto texture = _gpuContext->CreateTexture(
      skity::Texture::FormatFromColorType(pixmap->GetColorType()),
      pixmap->Width(), pixmap->Height(), pixmap->GetAlphaType());
  if (!texture) {
    return nullptr;
  }
  texture->DeferredUploadImage(std::move(pixmap));

  return skity::Image::MakeHWImage(texture);
}

- (void)render {
  if (!_gpuSurface) {
    [self recreateSurface];
  }
  if (!_gpuSurface) {
    return;
  }

  auto* canvas = _gpuSurface->LockCanvas();
  if (!canvas) {
    return;
  }

  const float width = static_cast<float>(NSWidth(self.bounds));
  const float height = static_cast<float>(NSHeight(self.bounds));
  canvas->DrawColor(skity::Color_WHITE);

  std::shared_ptr<skity::Image> image =
      self.useSoftwareCanvas
          ? [self makeSoftwareImageWithWidth:width height:height]
          : [self makeGPUImageWithWidth:width height:height];
  if (image) {
    skity::SamplingOptions options{};
    options.filter = skity::FilterMode::kLinear;
    canvas->DrawImage(image, skity::Rect::MakeXYWH(0.f, 0.f, width, height),
                      options, nullptr);
  } else {
    [self drawContentOnCanvas:canvas width:width height:height];
  }

  canvas->Flush();
  _gpuSurface->Flush();
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  [self recreateSurface];
  [self render];
}

- (void)layout {
  [super layout];
  [self recreateSurface];
  [self render];
}

- (void)setFrameSize:(NSSize)newSize {
  [super setFrameSize:newSize];
  [self recreateSurface];
  [self render];
}

@end
