// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrSVGView.h"

#import <Foundation/Foundation.h>
#include <memory>
#include <sstream>
#include <vector>

#include "element/SrSVGTypes.h"
#include "parser/SrSVGDOM.h"
#include "platform/iOS/SrIOSCanvas.h"

using serval::svg::ios::SrIOSCanvas;
using serval::svg::parser::SrSVGDiagnostic;
using serval::svg::parser::SrSVGDOM;

@implementation SrSVGRenderResult {
  BOOL _error;
  NSString* _errorMessage;
}

@synthesize error = _error;
@synthesize errorMessage = _errorMessage;

- (instancetype)initWithError:(BOOL)error errorMessage:(NSString*)errorMessage {
  self = [super init];
  if (self) {
    _error = error;
    _errorMessage = [errorMessage copy];
  }
  return self;
}

@end

static SrSVGRenderResult* SrSVGMakeRenderResult(
    const std::vector<SrSVGDiagnostic>& diagnostics) {
  NSString* errorMessage =
      diagnostics.empty()
          ? nil
          : [NSString stringWithUTF8String:diagnostics.front().message.c_str()];
  return [[SrSVGRenderResult alloc] initWithError:!diagnostics.empty()
                                     errorMessage:errorMessage];
}

static bool SrSVGParseSVGStringDoc(std::unique_ptr<SrSVGDOM>& svgDom,
                                   NSString* svgDoc,
                                   std::vector<SrSVGDiagnostic>* diagnostics) {
  std::string svgCppString([svgDoc UTF8String]);
  svgDom =
      SrSVGDOM::make(svgCppString.c_str(), svgCppString.length(), diagnostics);
  return svgDom != nullptr;
}

static bool SrSVGParseSVGDataDoc(std::unique_ptr<SrSVGDOM>& svgDom,
                                 NSData* svgDoc,
                                 std::vector<SrSVGDiagnostic>* diagnostics) {
  NSString* svgString = [[NSString alloc] initWithData:svgDoc
                                              encoding:NSUTF8StringEncoding];
  return SrSVGParseSVGStringDoc(svgDom, svgString, diagnostics);
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
    _svgDom.get()->Render(&canvas, viewPort);
  }
}

- (SrSVGRenderResult*)parseContentWithResult:(NSString*)content {
  std::vector<SrSVGDiagnostic> diagnostics;
  SrSVGParseSVGStringDoc(self->_svgDom, content, &diagnostics);
  [self setNeedsDisplay];
  return SrSVGMakeRenderResult(diagnostics);
}

- (void)setColor:(NSString*)color {
  NSString* normalized = nil;
  if (color.length > 0) {
    normalized = [color
        stringByTrimmingCharactersInSet:[NSCharacterSet
                                            whitespaceAndNewlineCharacterSet]];
  }
  if ((_color == nil && normalized == nil) ||
      [_color isEqualToString:normalized]) {
    return;
  }
  _color = [normalized copy];
  [self setNeedsDisplay];
}

- (instancetype)initWithData:(NSData*)data {
  self = [super init];
  if (self) {
    self->_svgDoc = data;
    std::vector<SrSVGDiagnostic> diagnostics;
    SrSVGParseSVGDataDoc(self->_svgDom, data, &diagnostics);
  }
  return self;
}

- (instancetype)initWithString:(NSString*)svgDoc {
  self = [super init];
  if (self) {
    std::vector<SrSVGDiagnostic> diagnostics;
    SrSVGParseSVGStringDoc(self->_svgDom, svgDoc, &diagnostics);
  }
  return self;
}

@end
