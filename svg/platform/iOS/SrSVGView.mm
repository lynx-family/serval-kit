// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrSVGView.h"

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

@implementation SrSVGView {
  NSData* _svgDoc;
  std::unique_ptr<SrSVGDOM> _svgDom;
}

- (void)drawRect:(CGRect)rect {
  if (_svgDom) {
    CGContextRef context = UIGraphicsGetCurrentContext();
    SrIOSCanvas canvas(context);
    SrSVGBox viewPort{static_cast<float>(rect.origin.x),
                      static_cast<float>(rect.origin.y),
                      static_cast<float>(rect.size.width),
                      static_cast<float>(rect.size.height)};
    _svgDom.get()->Render(&canvas, viewPort);
  }
}

- (void)parseContent:(NSString*)content {
  SrSVGParseSVGStringDoc(self->_svgDom, content);
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

@end
