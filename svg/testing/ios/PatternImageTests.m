// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <ServalSVG/SrSVG.h>
#import <XCTest/XCTest.h>
#include <math.h>

static UIImage* DecodeDataImage(NSString* href) {
  if (![href hasPrefix:@"data:image/"]) {
    return nil;
  }
  NSRange separator = [href rangeOfString:@";base64,"];
  if (separator.location == NSNotFound) {
    return nil;
  }
  NSData* data = [[NSData alloc]
      initWithBase64EncodedString:[href
                                      substringFromIndex:NSMaxRange(separator)]
                          options:NSDataBase64DecodingIgnoreUnknownCharacters];
  return data.length > 0 ? [UIImage imageWithData:data] : nil;
}

static NSData* RGBAImageBytes(UIImage* image) {
  CGImageRef cgImage = image.CGImage;
  if (!cgImage) {
    return nil;
  }
  size_t width = CGImageGetWidth(cgImage);
  size_t height = CGImageGetHeight(cgImage);
  NSMutableData* bytes = [NSMutableData dataWithLength:width * height * 4];
  CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGContextRef context = CGBitmapContextCreate(
      bytes.mutableBytes, width, height, 8, width * 4, space,
      (CGBitmapInfo)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGContextSetBlendMode(context, kCGBlendModeCopy);
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
  CGContextRelease(context);
  CGColorSpaceRelease(space);
  return bytes;
}

@interface PatternImageTests : XCTestCase
@end

@implementation PatternImageTests

- (void)testPatternPercentagesMatchChromeReferenceSamples {
  NSBundle* bundle = [NSBundle bundleForClass:self.class];
  NSString* manifestPath = [bundle pathForResource:@"pattern_percent_cases"
                                            ofType:@"json"];
  XCTAssertNotNil(manifestPath);
  if (!manifestPath) {
    return;
  }
  NSError* error = nil;
  NSDictionary* manifest = [NSJSONSerialization
      JSONObjectWithData:[NSData dataWithContentsOfFile:manifestPath]
                 options:0
                   error:&error];
  XCTAssertNil(error);
  XCTAssertEqualObjects(manifest[@"schema_version"], @1);
  NSArray<NSDictionary*>* cases = manifest[@"cases"];
  XCTAssertGreaterThan(cases.count, 0u);
  NSLog(@"Pattern reference generator: %@", manifest[@"generator"]);

  NSUInteger checkedSamples = 0;
  for (NSDictionary* testCase in cases) {
    @autoreleasepool {
      NSString* fileName = testCase[@"file"];
      NSString* fixturePath = [bundle pathForResource:fileName.lastPathComponent
                                               ofType:nil];
      XCTAssertNotNil(fixturePath, @"Fixture %@", fileName);
      if (!fixturePath) {
        continue;
      }
      NSData* svgData = [NSData dataWithContentsOfFile:fixturePath];
      XCTAssertGreaterThan(svgData.length, 0u, @"Fixture %@", fileName);
      NSUInteger width = [testCase[@"width"] unsignedIntegerValue];
      NSUInteger height = [testCase[@"height"] unsignedIntegerValue];
      XCTAssertGreaterThan(width, 0u);
      XCTAssertGreaterThan(height, 0u);
      if (svgData.length == 0 || width == 0 || height == 0) {
        continue;
      }

      NSMutableDictionary<NSString*, UIImage*>* images =
          [NSMutableDictionary dictionary];
      SrSVG* renderer = [[SrSVG alloc] init];
      SrSVGRenderResult* result = nil;
      UIImage* image =
          [renderer getSrSvgDrawImageWithData:svgData
                                      andSize:CGSizeMake(width, height)
                                     andColor:nil
                                       atTime:0
                                  andCallback:^UIImage*(NSString* href) {
                                    if (href.length == 0) {
                                      return nil;
                                    }
                                    UIImage* cached = images[href];
                                    if (!cached) {
                                      cached = DecodeDataImage(href);
                                      if (cached) {
                                        images[href] = cached;
                                      }
                                    }
                                    return cached;
                                  }
                                       result:&result];
      XCTAssertFalse(result.hasError, @"%@: %@", fileName, result.errorMessage);
      XCTAssertNotNil(image, @"Fixture %@", fileName);
      if (!image) {
        continue;
      }
      XCTAssertEqual(CGImageGetWidth(image.CGImage), width);
      XCTAssertEqual(CGImageGetHeight(image.CGImage), height);
      XCTAttachment* attachment =
          [XCTAttachment attachmentWithData:UIImagePNGRepresentation(image)
                      uniformTypeIdentifier:@"public.png"];
      attachment.name = [fileName stringByReplacingOccurrencesOfString:@".svg"
                                                            withString:@".png"];
      attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
      [self addAttachment:attachment];

      NSData* rgba = RGBAImageBytes(image);
      const uint8_t* pixels = rgba.bytes;
      NSArray<NSDictionary*>* samples = testCase[@"samples"];
      XCTAssertGreaterThan(samples.count, 0u, @"Fixture %@", fileName);
      for (NSDictionary* sample in samples) {
        NSUInteger x = [sample[@"x"] unsignedIntegerValue];
        NSUInteger y = [sample[@"y"] unsignedIntegerValue];
        XCTAssertLessThan(x, width);
        XCTAssertLessThan(y, height);
        if (x >= width || y >= height || !pixels) {
          continue;
        }
        NSArray<NSNumber*>* expected = sample[@"rgba"];
        XCTAssertEqual(expected.count, 4u);
        const uint8_t* pixel = pixels + 4 * (y * width + x);
        // Chrome's ImageData is unpremultiplied RGBA; Core Graphics stores
        // premultiplied channels. Samples avoid rasterization boundaries.
        for (NSUInteger channel = 0; channel < 4; ++channel) {
          NSInteger actual = pixel[channel];
          if (channel < 3 && pixel[3] > 0 && pixel[3] < 255) {
            actual = lround(actual * 255.0 / pixel[3]);
          }
          XCTAssertEqualWithAccuracy(actual, expected[channel].integerValue, 3,
                                     @"%@ pixel (%lu,%lu) channel %lu",
                                     fileName, (unsigned long)x,
                                     (unsigned long)y, (unsigned long)channel);
        }
        ++checkedSamples;
      }
      NSLog(@"Chrome reference fixture %@: %lu samples", fileName,
            (unsigned long)samples.count);
    }
  }
  NSLog(@"Compared %lu pattern fixtures and %lu RGBA samples against Chrome",
        (unsigned long)cases.count, (unsigned long)checkedSamples);
}

@end
