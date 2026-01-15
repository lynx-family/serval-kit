// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrSVG.h"

#import <Foundation/Foundation.h>
#include <memory>
#include <sstream>

#include "parser/SrSVGDOM.h"
#include "platform/iOS/SrIOSCanvas.h"

using serval::svg::ios::SrIOSCanvas;
using serval::svg::parser::SrSVGDOM;

static bool SrSVGParseSVGStringDoc(std::unique_ptr<SrSVGDOM>& svgDom,
                                   NSString* svgDoc) {
  std::string svgCppString([svgDoc UTF8String]);
  svgDom = SrSVGDOM::make(svgCppString.c_str(), svgCppString.length());
  return svgDom != nullptr;
}

static bool SrSVGParseSVGDataDoc(std::unique_ptr<SrSVGDOM>& svgDom,
                                 NSData* svgDoc) {
  NSString* svgString = [[NSString alloc] initWithData:svgDoc
                                              encoding:NSUTF8StringEncoding];
  return SrSVGParseSVGStringDoc(svgDom, svgString);
}

@implementation SrSVG {
  NSData* _svgDoc;
  std::unique_ptr<SrSVGDOM> _svgDom;
}

- (instancetype)initWithData:(NSData*)data {
  self = [super init];
  if (self) {
    self->_svgDoc = data;
    SrSVGParseSVGDataDoc(self->_svgDom, data);
  }
  return self;
}

- (instancetype)initWithString:(NSString*)svgDoc {
  self = [super init];
  if (self) {
    SrSVGParseSVGStringDoc(self->_svgDom, svgDoc);
  }
  return self;
}

- (UIImage*)getSrSvgDrawImageWithData:(NSData*)data
                              andSize:(CGSize)size
                          andCallback:(SrSvgImageCallback)imageCb {
  NSString* dataString = [[NSString alloc] initWithData:data
                                               encoding:NSUTF8StringEncoding];
  if (dataString == nil) {
    return nil;
  }
  CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef cgContext =
      CGBitmapContextCreate(NULL, size.width, size.height, 8, 0, cgColorSpace,
                            kCGImageAlphaPremultipliedLast);
  CGContextSetInterpolationQuality(cgContext, kCGInterpolationMedium);
  CGContextSetAllowsAntialiasing(cgContext, YES);
  CGContextSetShouldAntialias(cgContext, YES);
  // Translate and flip the coordinate system, moving the drawing origin from the bottom-left to the top-left, while changing the Y-axis from incrementing upwards to incrementing downwards.
  CGContextTranslateCTM(cgContext, 0, size.height);
  CGContextScaleCTM(cgContext, 1, -1);
  SrSVGParseSVGStringDoc(_svgDom, dataString);
  if (_svgDom) {
    SrIOSCanvas canvas(cgContext, imageCb);
    SrSVGBox viewPort{0.f, 0.f, static_cast<float>(size.width),
                      static_cast<float>(size.height)};
    _svgDom.get()->Render(&canvas, viewPort);
  }
  CGImageRef cgImage = CGBitmapContextCreateImage(cgContext);
  UIImage* renderImage = [[UIImage alloc] initWithCGImage:cgImage];
  CGImageRelease(cgImage);
  CGContextRelease(cgContext);
  CGColorSpaceRelease(cgColorSpace);
  return renderImage;
}

@end
