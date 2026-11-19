// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "parser/SrSVGDOM.h"
#include "platform/iOS/SrIOSCanvas.h"
#import "platform/iOS/SrSVG.h"
#import "platform/iOS/SrSVGView.h"

static const CGSize kSize = {320, 180};

static NSString* Wrap(NSString* contents) {
  return [NSString
      stringWithFormat:
          @"<svg width='320' height='180' viewBox='0 0 320 180'>%@</svg>",
          contents];
}

static UIImage* Render(NSString* svg, double seconds) {
  NSData* data = [svg dataUsingEncoding:NSUTF8StringEncoding];
  SrSVG* renderer = [[SrSVG alloc] initWithData:data];
  return [renderer getSrSvgDrawImageWithData:data
                                     andSize:kSize
                                    andColor:nil
                                      atTime:seconds
                                 andCallback:nil
                                      result:nil];
}

static NSData* Pixels(UIImage* image) {
  if (!image.CGImage)
    return nil;
  NSMutableData* pixels = [NSMutableData dataWithLength:320 * 180 * 4];
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixels.mutableBytes, 320, 180, 8, 320 * 4, space,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  if (context)
    CGContextDrawImage(context, CGRectMake(0, 0, 320, 180), image.CGImage);
  if (context)
    CGContextRelease(context);
  CGColorSpaceRelease(space);
  return context ? pixels : nil;
}

static UIImage* RenderView(NSString* svg) {
  SrSVGView* view = [[SrSVGView alloc] initWithString:svg];
  view.frame = CGRectMake(0, 0, 320, 180);
  UIGraphicsBeginImageContextWithOptions(kSize, NO, 1);
  [view drawRect:view.bounds];
  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return image;
}

@interface RenderRegressionTests : XCTestCase
@end

@implementation RenderRegressionTests

- (void)testPathAnimationMatchesExpectedGeometry {
  NSString* animated =
      Wrap(@"<path fill='none' stroke='#d03070' stroke-width='4' "
            "d='M20 40 L80 40 C100 20 120 20 140 40 Q160 60 180 40 A20 10 0 0 "
            "1 220 40'>"
            "<animate attributeName='d' dur='2s' fill='freeze' "
            "from='M20 40 L80 40 C100 20 120 20 140 40 Q160 60 180 40 A20 10 0 "
            "0 1 220 40' "
            "to='M20 80 L80 80 C100 60 120 60 140 80 Q160 100 180 80 A30 20 0 "
            "0 1 240 80'/>"
            "</path>");
  NSData* first = nil;
  for (NSNumber* time in @[@0, @0.5, @1, @1.5, @2]) {
    double p = time.doubleValue / 2;
    NSString* path = [NSString
        stringWithFormat:@"<path fill='none' stroke='#d03070' stroke-width='4' "
                          "d='M20 %g L80 %g C100 %g 120 %g 140 %g Q160 %g 180 "
                          "%g A%g %g 0 0 1 %g %g'/>",
                         40 + 40 * p, 40 + 40 * p, 20 + 40 * p, 20 + 40 * p,
                         40 + 40 * p, 60 + 40 * p, 40 + 40 * p, 20 + 10 * p,
                         10 + 10 * p, 220 + 20 * p, 40 + 40 * p];
    NSData* actual = Pixels(Render(animated, time.doubleValue));
    NSData* expected = Pixels(Render(Wrap(path), 0));
    XCTAssertEqualObjects(actual, expected, @"Path geometry t=%@", time);
    if (!first)
      first = actual;
    else
      XCTAssertNotEqualObjects(first, actual, @"Animation did not change");
  }
  XCTAssertEqualObjects(Pixels(RenderView(animated)), first);
}

- (void)testPersistentDOMRestoresPath {
  NSString* svg =
      Wrap(@"<path fill='none' stroke='red' stroke-width='5' d='M20 40L80 40'>"
            "<animate attributeName='d' from='M20 40L80 40' to='M20 80L160 80' "
            "dur='2s'/></path>");
  const char* text = svg.UTF8String;
  auto dom = serval::svg::parser::SrSVGDOM::make(text, strlen(text), nullptr);
  XCTAssertTrue(dom != nullptr);
  auto draw = [&](double time, bool animated) {
    UIGraphicsBeginImageContextWithOptions(kSize, NO, 1);
    {
      serval::svg::ios::SrIOSCanvas canvas(UIGraphicsGetCurrentContext());
      SrSVGBox viewport{0, 0, 320, 180};
      if (animated)
        dom->RenderAtTime(&canvas, viewport, time);
      else
        dom->Render(&canvas, viewport);
    }
    UIImage* result = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return Pixels(result);
  };
  NSData* original = draw(0, false);
  NSData* mid = draw(1, true);
  XCTAssertNotEqualObjects(original, mid);
  XCTAssertEqualObjects(original, draw(0, false));
  XCTAssertEqualObjects(mid, draw(1, true));
}

- (void)testMalformedInputsDoNotCrash {
  NSArray* invalid = @[
    @"<path d='M0 0 A1 1 0'/>", @"<path d='M0 0 A1 1 0 0'/>",
    @"<path d='M0 0 A1 1 0 2 1 10 10'/>", @"<path d='M0 0 L10'/>",
    @"<path d='M0 0 L1e999 10'/>", @"<polygon points='0 0 10'/>",
    @"<polygon points='0 0 nan 10'/>", @"<polyline points='0 0 10junk 20'/>",
    @"<rect width='1e999' height='20'/>", @"<rect width='nan' height='20'/>",
    @"<rect width='40' height='40' transform='translate()'/>",
    @"<rect width='40' height='40' transform='scale()'/>",
    @"<rect width='40' height='40' transform='rotate(10,20)'/>",
    @"<rect width='40' height='40' transform='matrix(1 0)'/>",
    @"<rect width='40' height='40' transform='scale(1e999)'/>",
    @"<path stroke='red' d='M0 0L10 10'><animate attributeName='d' "
     "from='M0 0 A1 1 0' to='M0 0 A2 2 0' dur='1s'/></path>",
    @"<polygon points='0 0 40 40'><animate attributeName='points' "
     "values='0 0;10junk 20' dur='1s'/></polygon>"
  ];
  for (NSString* content in invalid) {
    @autoreleasepool {
      NSString* svg = Wrap(content);
      for (NSNumber* time in @[@0, @0.5, @1]) {
        XCTAssertNotNil(Render(svg, time.doubleValue), @"%@ t=%@", content,
                        time);
      }
      XCTAssertNotNil(RenderView(svg), @"%@", content);
    }
  }
  for (NSString* value in @[@"0 0 inf 20", @"0 0 10", @"0 0 10 20junk"]) {
    NSString* svg = [NSString
        stringWithFormat:
            @"<svg viewBox='%@'><path fill='red' d='M0 0L10 0L0 10Z'/></svg>",
            value];
    XCTAssertNotNil(Render(svg, .5));
  }
}

- (void)testExistingIllegalParsingFixtures {
  NSString* cases = [[NSBundle bundleForClass:self.class] resourcePath];
  NSUInteger count = 0;
  for (NSString* filename in
       [[NSFileManager defaultManager] contentsOfDirectoryAtPath:cases
                                                           error:nil]) {
    if (![filename hasPrefix:@"invalid-"])
      continue;
    @autoreleasepool {
      NSString* svg = [NSString
          stringWithContentsOfFile:[cases
                                       stringByAppendingPathComponent:filename]
                          encoding:NSUTF8StringEncoding
                             error:nil];
      XCTAssertNotNil(Render(svg, .5), @"%@", filename);
      ++count;
    }
  }
  XCTAssertGreaterThan(count, 0u);
  NSLog(@"SVG_RENDER_INVALID_FIXTURES=%lu", (unsigned long)count);
}

- (void)testConcurrentPathBitmapRendering {
  NSString* svg =
      Wrap(@"<path fill='none' stroke='blue' stroke-width='5' d='M20 20L80 80'>"
            "<animate attributeName='d' from='M20 20L80 80' to='M20 80L160 20' "
            "dur='2s'/></path>");
  NSData* data = [svg dataUsingEncoding:NSUTF8StringEncoding];
  SrSVG* renderer = [[SrSVG alloc] initWithData:data];
  NSData* expected = Pixels(Render(svg, .75));
  dispatch_apply(40, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0),
                 ^(size_t i) {
                   @autoreleasepool {
                     UIImage* image = [renderer getSrSvgDrawImageWithData:data
                                                                  andSize:kSize
                                                                 andColor:nil
                                                                   atTime:.75
                                                              andCallback:nil
                                                                   result:nil];
                     XCTAssertEqualObjects(Pixels(image), expected,
                                           @"Concurrent frame %zu", i);
                   }
                 });
}

- (void)testZeroRadiusArcMatchesLine {
  UIImage* actual =
      Render(Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                   "d='M20 40 A0 20 0 0 1 100 80'/>"),
             0);
  UIImage* expected =
      Render(Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                   "d='M20 40 L100 80'/>"),
             0);
  XCTAssertEqualObjects(Pixels(actual), Pixels(expected));
  UIImage* negative =
      Render(Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                   "d='M20 40 A-30 -20 0 0 1 100 80'/>"),
             0);
  UIImage* positive =
      Render(Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                   "d='M20 40 A30 20 0 0 1 100 80'/>"),
             0);
  XCTAssertEqualObjects(Pixels(negative), Pixels(positive));
}

- (void)testArcEndpointAndRadiusBoundaries {
  auto pathPixels = [](NSString* path) {
    NSString* content = [NSString
        stringWithFormat:@"<path fill='none' stroke='red' stroke-width='4' "
                          "stroke-linecap='round' d='%@'/>",
                         path];
    return Pixels(Render(Wrap(content), 0));
  };
  NSData* semicircle = pathPixels(@"M20 40 A10 10 0 0 1 40 40");
  XCTAssertEqual(semicircle.length, 320 * 180 * 4u);
  XCTAssertEqualObjects(pathPixels(@"M20 40 A1e-20 1e-20 0 0 1 40 40"),
                        semicircle);
  XCTAssertNotEqualObjects(semicircle, pathPixels(@"M20 40 L40 40"));
  // A 10/9 radius correction makes both choices of arc size a half ellipse.
  NSData* corrected = pathPixels(@"M220 160 A50 33.333333333 0 1 1 300 120");
  XCTAssertEqualObjects(pathPixels(@"M220 160 A45 30 0 1 1 300 120"),
                        corrected);
  XCTAssertEqualObjects(pathPixels(@"M220 160 A45 30 0 0 1 300 120"),
                        corrected);

  NSData* blank = Pixels(Render(Wrap(@""), 0));
  XCTAssertEqual(blank.length, 320 * 180 * 4u);
  XCTAssertEqualObjects(pathPixels(@"M60 40 A0 10 0 1 1 60 40"), blank);
  XCTAssertNotEqualObjects(pathPixels(@"M160 130 A50 50 0 1 1 160.02 130"),
                           blank);
}

- (void)testLongDashArrayMatchesRepeatedPattern {
  NSMutableString* dash = [NSMutableString string];
  for (size_t i = 0; i < 2048; ++i)
    [dash appendString:@"4 4 "];
  NSString* actual = Wrap([NSString
      stringWithFormat:@"<path fill='none' stroke='red' stroke-width='4' "
                       @"stroke-dasharray='%@' d='M20 40 L300 140'/>",
                       dash]);
  NSString* expected = Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                            @"stroke-dasharray='4 4' d='M20 40 L300 140'/>");
  XCTAssertEqualObjects(Pixels(Render(actual, 0)), Pixels(Render(expected, 0)));
}

- (void)testDashArrayNoneAnimationClearsAndRestores {
  NSString* dashed = Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                          @"stroke-dasharray='8 4' d='M20 40 L300 140'/>");
  NSString* solid = Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                         @"stroke-dasharray='none' d='M20 40 L300 140'/>");
  NSData* dashedPixels = Pixels(Render(dashed, 0));
  NSData* solidPixels = Pixels(Render(solid, 0));
  XCTAssertEqual(dashedPixels.length, 320 * 180 * 4u);
  XCTAssertEqual(solidPixels.length, 320 * 180 * 4u);
  XCTAssertNotEqualObjects(dashedPixels, solidPixels);
  NSString* set = Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                       @"stroke-dasharray='8 4' d='M20 40 L300 140'><set "
                       @"attributeName='stroke-dasharray' to='none'/></path>");
  XCTAssertEqualObjects(Pixels(Render(set, 0)), solidPixels);
  NSString* timedSet = Wrap(
      @"<path fill='none' stroke='red' stroke-width='4' stroke-dasharray='8 4' "
      @"d='M20 40 L300 140'><set attributeName='stroke-dasharray' to='none' "
      @"begin='.5s' dur='1s'/></path>");
  XCTAssertEqualObjects(Pixels(Render(timedSet, 0)), dashedPixels);
  XCTAssertEqualObjects(Pixels(Render(timedSet, 1)), solidPixels);
  XCTAssertEqualObjects(Pixels(Render(timedSet, 1.75)), dashedPixels);
  NSString* discrete = Wrap(
      @"<path fill='none' stroke='red' stroke-width='4' stroke-dasharray='8 4' "
      @"d='M20 40 L300 140'><animate attributeName='stroke-dasharray' "
      @"values='8 4;none;2 2' calcMode='discrete' dur='3s'/></path>");
  XCTAssertEqualObjects(Pixels(Render(discrete, .5)), dashedPixels);
  XCTAssertEqualObjects(Pixels(Render(discrete, 1.5)), solidPixels);
  NSString* shortDash = Wrap(@"<path fill='none' stroke='red' stroke-width='4' "
                             @"stroke-dasharray='2 2' d='M20 40 L300 140'/>");
  XCTAssertEqualObjects(Pixels(Render(discrete, 2.5)),
                        Pixels(Render(shortDash, 0)));
}

- (void)testDashOffsetLengthsAndAnimation {
  auto svg = [](NSString* attributes, NSString* animation) {
    return Wrap([NSString
        stringWithFormat:@"<path fill='none' stroke='red' stroke-width='4' "
                          "stroke-dasharray='8 4' d='M20 40 L300 140' %@>%@"
                          "</path>",
                         attributes, animation]);
  };
  NSData* zero = Pixels(Render(svg(@"stroke-dashoffset='0'", @""), 0));
  NSData* offset = Pixels(Render(svg(@"stroke-dashoffset='3'", @""), 0));
  XCTAssertEqual(offset.length, 320 * 180 * 4u);
  XCTAssertNotEqualObjects(offset, zero);
  XCTAssertEqualObjects(Pixels(Render(svg(@"stroke-dashoffset='3px'", @""), 0)),
                        offset);
  XCTAssertEqualObjects(
      Pixels(Render(svg(@"style='stroke-dashoffset: 3px'", @""), 0)), offset);
  XCTAssertEqualObjects(
      Pixels(Render(svg(@"stroke-dashoffset='3junk'", @""), 0)), zero);

  NSString* animated = svg(@"stroke-dashoffset='0px'",
                           @"<animate attributeName='stroke-dashoffset' "
                            "from='1px' to='5px' dur='2s'/>");
  XCTAssertEqualObjects(Pixels(Render(animated, 1)), offset);
  XCTAssertEqualObjects(Pixels(Render(animated, 2.5)), zero);
}

@end
