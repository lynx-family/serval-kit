// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrSVG.h"
#include "parser/SrSVGTraversalState.h"

#import <Foundation/Foundation.h>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/iOS/SrIOSCanvas.h"

using serval::svg::ios::SrIOSCanvas;
using serval::svg::parser::SrSVGDiagnostic;
using serval::svg::parser::SrSVGDOM;

static std::unique_ptr<SrSVGDOM> SrSVGParseSVGStringDoc(
    NSString* svgDoc, std::vector<SrSVGDiagnostic>* diagnostics) {
  const char* contents = [svgDoc UTF8String];
  if (!contents) {
    return nullptr;
  }
  return SrSVGDOM::make(contents, strlen(contents), diagnostics);
}

static SrSVGRenderResult* SrSVGMakeRenderResult(
    const std::vector<SrSVGDiagnostic>& diagnostics) {
  NSString* errorMessage =
      diagnostics.empty()
          ? nil
          : [NSString stringWithUTF8String:diagnostics.front().message.c_str()];
  return [[SrSVGRenderResult alloc] initWithError:!diagnostics.empty()
                                     errorMessage:errorMessage];
}

static void SrSVGApplyDefaultColor(SrSVGDOM& svgDom, NSString* color) {
  NSString* normalized = nil;
  if (color.length > 0) {
    normalized = [color
        stringByTrimmingCharactersInSet:[NSCharacterSet
                                            whitespaceAndNewlineCharacterSet]];
  }
  if (normalized.length > 0) {
    uint32_t default_color = 0;
    if (parse_svg_color(normalized.UTF8String, &default_color)) {
      svgDom.SetDefaultColor(default_color);
      return;
    }
  }
  svgDom.ResetDefaultColor();
}

@interface SrSVG ()
@property(atomic, assign) BOOL parsedDocumentHasAnimations;

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(NSString*)color
                         renderAtTime:(BOOL)renderAtTime
                              seconds:(NSTimeInterval)seconds
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result;
@end

@implementation SrSVG {
  NSData* _svgDoc;
}

- (instancetype)initWithData:(NSData*)data {
  self = [super init];
  if (self) {
    self->_svgDoc = data;
    NSString* svgDoc = [[NSString alloc] initWithData:data
                                             encoding:NSUTF8StringEncoding];
    auto svgDom = SrSVGParseSVGStringDoc(svgDoc, nullptr);
    self.parsedDocumentHasAnimations = svgDom && svgDom->HasAnimations();
  }
  return self;
}

- (instancetype)initWithString:(NSString*)svgDoc {
  self = [super init];
  if (self) {
    auto svgDom = SrSVGParseSVGStringDoc(svgDoc, nullptr);
    self.parsedDocumentHasAnimations = svgDom && svgDom->HasAnimations();
  }
  return self;
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                          andCallback:(SrSvgImageCallback)imageCb {
  return [self getSrSvgDrawImageWithData:data
                                 andSize:size
                                andColor:nil
                             andCallback:imageCb
                                  result:nil];
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(NSString*)color
                          andCallback:(SrSvgImageCallback)imageCb {
  return [self getSrSvgDrawImageWithData:data
                                 andSize:size
                                andColor:color
                             andCallback:imageCb
                                  result:nil];
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(NSString*)color
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result {
  return [self getSrSvgDrawImageWithData:data
                                 andSize:size
                                andColor:color
                            renderAtTime:NO
                                 seconds:0
                             andCallback:imageCb
                                  result:result];
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(NSString*)color
                               atTime:(NSTimeInterval)seconds
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result {
  return [self getSrSvgDrawImageWithData:data
                                 andSize:size
                                andColor:color
                            renderAtTime:YES
                                 seconds:seconds
                             andCallback:imageCb
                                  result:result];
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                             andColor:(NSString*)color
                         renderAtTime:(BOOL)renderAtTime
                              seconds:(NSTimeInterval)seconds
                          andCallback:(SrSvgImageCallback)imageCb
                               result:(SrSVGRenderResult* _Nullable* _Nullable)
                                          result {
  NSString* dataString = [[NSString alloc] initWithData:data
                                               encoding:NSUTF8StringEncoding];
  if (dataString == nil) {
    if (result) {
      *result = [[SrSVGRenderResult alloc] initWithError:NO errorMessage:nil];
    }
    return nil;
  }
  if (size.width < 1 || size.height < 1 ||
      !serval::svg::parser::SrSVGTraversalState::OffscreenPixels(size.width,
                                                                 size.height))
    return nil;
  CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
  if (!cgColorSpace)
    return nil;
  CGContextRef cgContext =
      CGBitmapContextCreate(NULL, size.width, size.height, 8, 0, cgColorSpace,
                            kCGImageAlphaPremultipliedLast);
  if (!cgContext) {
    CGColorSpaceRelease(cgColorSpace);
    return nil;
  }
  CGContextSetInterpolationQuality(cgContext, kCGInterpolationMedium);
  CGContextSetAllowsAntialiasing(cgContext, YES);
  CGContextSetShouldAntialias(cgContext, YES);
  // Translate and flip the coordinate system, moving the drawing origin from the bottom-left to the top-left, while changing the Y-axis from incrementing upwards to incrementing downwards.
  CGContextTranslateCTM(cgContext, 0, size.height);
  CGContextScaleCTM(cgContext, 1, -1);
  std::vector<SrSVGDiagnostic> diagnostics;
  // Keep this draw's DOM alive across concurrent or reentrant draws.
  auto svgDom = SrSVGParseSVGStringDoc(dataString, &diagnostics);
  self.parsedDocumentHasAnimations = svgDom && svgDom->HasAnimations();
  if (svgDom) {
    SrSVGApplyDefaultColor(*svgDom, color);
    SrIOSCanvas canvas(cgContext, imageCb);
    SrSVGBox viewPort{0.f, 0.f, static_cast<float>(size.width),
                      static_cast<float>(size.height)};
    if (renderAtTime) {
      svgDom->RenderAtTime(&canvas, viewPort, seconds);
    } else {
      svgDom->Render(&canvas, viewPort);
    }
  }
  CGImageRef cgImage = CGBitmapContextCreateImage(cgContext);
  UIImage* renderImage =
      cgImage ? [[UIImage alloc] initWithCGImage:cgImage] : nil;
  if (cgImage)
    CGImageRelease(cgImage);
  CGContextRelease(cgContext);
  CGColorSpaceRelease(cgColorSpace);
  if (result) {
    *result = SrSVGMakeRenderResult(diagnostics);
  }
  return renderImage;
}

- (BOOL)hasAnimations {
  return self.parsedDocumentHasAnimations;
}

@end
