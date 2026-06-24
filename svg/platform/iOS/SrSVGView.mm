// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/iOS/SrSVGView.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include "element/SrSVGTypes.h"
#include "platform/iOS/SrIOSCanvas.h"
#include "renderer/SrSVGAnimatedRenderer.h"

using serval::svg::ios::SrIOSCanvas;
using serval::svg::parser::SrSVGDiagnostic;
using serval::svg::renderer::SrSVGAnimatedRenderer;

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

static bool SrSVGSetRendererContent(SrSVGAnimatedRenderer& renderer,
                                    NSString* svgDoc,
                                    std::vector<SrSVGDiagnostic>* diagnostics) {
  if (svgDoc.length == 0) {
    renderer.SetDOM(nullptr);
    return false;
  }
  std::string svgCppString([svgDoc UTF8String]);
  return renderer.SetContent(svgCppString.c_str(), svgCppString.length(),
                             diagnostics);
}

static bool SrSVGSetRendererData(SrSVGAnimatedRenderer& renderer,
                                 NSData* svgDoc,
                                 std::vector<SrSVGDiagnostic>* diagnostics) {
  NSString* svgString = [[NSString alloc] initWithData:svgDoc
                                              encoding:NSUTF8StringEncoding];
  return SrSVGSetRendererContent(renderer, svgString, diagnostics);
}

@implementation SrSVGView {
  NSData* _svgDoc;
  SrSVGAnimatedRenderer _svgRenderer;
  CADisplayLink* _displayLink;
}

- (void)drawRect:(CGRect)rect {
  if (_svgRenderer.HasContent()) {
    CGContextRef context = UIGraphicsGetCurrentContext();
    SrIOSCanvas canvas(context);
    SrSVGBox viewPort{static_cast<float>(rect.origin.x),
                      static_cast<float>(rect.origin.y),
                      static_cast<float>(rect.size.width),
                      static_cast<float>(rect.size.height)};
    std::optional<uint32_t> default_color;
    if (self.color.length > 0) {
      uint32_t parsed_color = 0;
      if (parse_svg_color(self.color.UTF8String, &parsed_color)) {
        default_color = parsed_color;
      }
    }
    _svgRenderer.Render(&canvas, viewPort, default_color);
  }
}

- (SrSVGRenderResult*)parseContentWithResult:(NSString*)content {
  std::vector<SrSVGDiagnostic> diagnostics;
  SrSVGSetRendererContent(self->_svgRenderer, content, &diagnostics);
  [self resetAnimationClock];
  [self updateAnimationState];
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

- (void)didMoveToWindow {
  [super didMoveToWindow];
  [self updateAnimationState];
}

- (void)setHidden:(BOOL)hidden {
  [super setHidden:hidden];
  [self updateAnimationState];
}

- (instancetype)initWithData:(NSData*)data {
  self = [super init];
  if (self) {
    self->_svgDoc = data;
    std::vector<SrSVGDiagnostic> diagnostics;
    SrSVGSetRendererData(self->_svgRenderer, data, &diagnostics);
    [self resetAnimationClock];
    [self updateAnimationState];
  }
  return self;
}

- (instancetype)initWithString:(NSString*)svgDoc {
  self = [super init];
  if (self) {
    std::vector<SrSVGDiagnostic> diagnostics;
    SrSVGSetRendererContent(self->_svgRenderer, svgDoc, &diagnostics);
    [self resetAnimationClock];
    [self updateAnimationState];
  }
  return self;
}

- (void)dealloc {
  [self stopDisplayLink];
}

- (void)displayLinkDidTick:(CADisplayLink*)displayLink {
  const bool needsNextFrame =
      _svgRenderer.OnFrameTimeSeconds(displayLink.timestamp);
  [self setNeedsDisplay];
  if (!needsNextFrame) {
    [self stopDisplayLink];
  }
}

- (void)updateAnimationState {
  BOOL shouldAnimate =
      _svgRenderer.HasAnimations() && self.window != nil && !self.hidden;
  if (shouldAnimate) {
    [self startDisplayLink];
  } else {
    [self stopDisplayLink];
  }
}

- (void)startDisplayLink {
  if (_displayLink != nil) {
    return;
  }
  _svgRenderer.StartAnimationAtTimeSeconds(CACurrentMediaTime());
  if (!_svgRenderer.NeedsAnimationFrame()) {
    _svgRenderer.StopAnimation();
    return;
  }
  _displayLink =
      [CADisplayLink displayLinkWithTarget:self
                                  selector:@selector(displayLinkDidTick:)];
  [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                     forMode:NSRunLoopCommonModes];
}

- (void)stopDisplayLink {
  [_displayLink invalidate];
  _displayLink = nil;
  _svgRenderer.StopAnimation();
}

- (void)resetAnimationClock {
  _svgRenderer.ResetAnimationClock();
}

@end
