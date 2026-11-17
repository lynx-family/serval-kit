// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"
#include "skity/skity.hpp"

static std::shared_ptr<skity::Image> DecodeDataImage(const std::string& href) {
  NSString* text = [NSString stringWithUTF8String:href.c_str()];
  NSRange marker = [text rangeOfString:@";base64,"];
  if (![text hasPrefix:@"data:image/"] || marker.location == NSNotFound)
    return nullptr;
  NSData* data = [[NSData alloc]
      initWithBase64EncodedString:[text substringFromIndex:NSMaxRange(marker)]
                          options:NSDataBase64DecodingIgnoreUnknownCharacters];
  if (!data)
    return nullptr;
  CGImageSourceRef source =
      CGImageSourceCreateWithData((__bridge CFDataRef)data, nullptr);
  if (!source)
    return nullptr;
  CGImageRef cg = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (!cg)
    return nullptr;
  uint32_t width = (uint32_t)CGImageGetWidth(cg),
           height = (uint32_t)CGImageGetHeight(cg);
  auto pixmap = std::make_shared<skity::Pixmap>(
      width, height, skity::AlphaType::kPremul_AlphaType,
      skity::ColorType::kRGBA);
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixmap->WritableAddr(), width, height, 8, pixmap->RowBytes(), space,
      (CGBitmapInfo)(kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big));
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cg);
  CGContextRelease(context);
  CGColorSpaceRelease(space);
  CGImageRelease(cg);
  return skity::Image::MakeImage(pixmap);
}

static void WritePNG(const skity::Bitmap& bitmap, NSString* path) {
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      bitmap.GetPixelAddr(), bitmap.Width(), bitmap.Height(), 8,
      bitmap.RowBytes(), space,
      (CGBitmapInfo)(kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big));
  CGImageRef cg = CGBitmapContextCreateImage(context);
  CGImageDestinationRef destination = CGImageDestinationCreateWithURL(
      (__bridge CFURLRef)[NSURL fileURLWithPath:path], CFSTR("public.png"), 1,
      nullptr);
  CGImageDestinationAddImage(destination, cg, nullptr);
  CGImageDestinationFinalize(destination);
  CFRelease(destination);
  CGImageRelease(cg);
  CGContextRelease(context);
  CGColorSpaceRelease(space);
}

int main(int argc, const char** argv) {
  @autoreleasepool {
    if (argc != 4) {
      fprintf(stderr, "runner manifest test_cases output_directory\n");
      return 2;
    }
    NSString* manifestPath = [NSString stringWithUTF8String:argv[1]];
    NSString* fixtures = [NSString stringWithUTF8String:argv[2]];
    NSString* output = [NSString stringWithUTF8String:argv[3]];
    [[NSFileManager defaultManager] createDirectoryAtPath:output
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];
    NSData* manifestData = [NSData dataWithContentsOfFile:manifestPath];
    NSDictionary* manifest =
        manifestData ? [NSJSONSerialization JSONObjectWithData:manifestData
                                                       options:0
                                                         error:nil]
                     : nil;
    if (![manifest isKindOfClass:NSDictionary.class] ||
        [manifest[@"schema_version"] intValue] != 1 ||
        ![manifest[@"cases"] isKindOfClass:NSArray.class] ||
        [manifest[@"cases"] count] == 0) {
      fprintf(stderr, "Invalid or empty manifest %s\n",
              manifestPath.UTF8String);
      return 2;
    }
    NSMutableArray* reports = [NSMutableArray array];
    int failures = 0;
    for (NSDictionary* item in manifest[@"cases"]) {
      NSString* file = item[@"file"];
      NSData* svg = [NSData
          dataWithContentsOfFile:[fixtures
                                     stringByAppendingPathComponent:file]];
      if (!svg) {
        fprintf(stderr, "Missing fixture %s\n", file.UTF8String);
        return 2;
      }
      std::vector<serval::svg::parser::SrSVGDiagnostic> diagnostics;
      auto dom = serval::svg::parser::SrSVGDOM::make((const char*)svg.bytes,
                                                     svg.length, &diagnostics);
      if (!dom) {
        fprintf(stderr, "Invalid fixture %s\n", file.UTF8String);
        return 2;
      }
      uint32_t width = [item[@"width"] unsignedIntValue],
               height = [item[@"height"] unsignedIntValue];
      if (!width || !height ||
          ![item[@"samples"] isKindOfClass:NSArray.class] ||
          [item[@"samples"] count] == 0) {
        fprintf(stderr, "Invalid dimensions or samples for %s\n",
                file.UTF8String);
        return 2;
      }
      skity::Bitmap bitmap(width, height, skity::AlphaType::kPremul_AlphaType,
                           skity::ColorType::kRGBA);
      auto canvas = skity::Canvas::MakeSoftwareCanvas(&bitmap);
      canvas->DrawColor(0, skity::BlendMode::kSrc);
      std::unordered_map<std::string, std::shared_ptr<skity::Image>> imageCache;
      serval::svg::skity::SrSkityCanvas backend(
          canvas.get(), [&](std::string href) {
            auto found = imageCache.find(href);
            if (found != imageCache.end())
              return found->second;
            auto image = DecodeDataImage(href);
            imageCache[href] = image;
            return image;
          });
      dom->Render(&backend, SrSVGBox{0, 0, (float)width, (float)height});
      canvas->Flush();
      NSMutableArray* samples = [NSMutableArray array];
      bool passed = true;
      for (NSDictionary* sample in item[@"samples"]) {
        if (![sample[@"rgba"] isKindOfClass:NSArray.class] ||
            [sample[@"rgba"] count] != 4) {
          fprintf(stderr, "Reference pixels are not ready\n");
          return 2;
        }
        uint32_t x = [sample[@"x"] unsignedIntValue],
                 y = [sample[@"y"] unsignedIntValue];
        if (x >= width || y >= height) {
          fprintf(stderr, "Probe outside fixture %s\n", file.UTF8String);
          return 2;
        }
        const uint8_t* pixel = bitmap.GetPixmap()->Addr8(x, y);
        auto straight = [&](int channel) {
          return pixel[3]
                     ? std::min(255, (pixel[channel] * 255 + pixel[3] / 2) /
                                         pixel[3])
                     : 0;
        };
        NSArray* actual =
            @[@(straight(0)), @(straight(1)), @(straight(2)), @(pixel[3])];
        NSArray* expected = sample[@"rgba"];
        bool matched = true;
        for (int c = 0; c < 4; ++c)
          matched = matched && std::abs([actual[c] intValue] -
                                        [expected[c] intValue]) <= 2;
        passed = passed && matched;
        [samples addObject:@{
          @"x": @(x),
          @"y": @(y),
          @"expected": expected,
          @"actual": actual,
          @"passed": @(matched)
        }];
      }
      if (!passed)
        ++failures;
      [reports addObject:@{
        @"file": file,
        @"passed": @(passed),
        @"samples": samples
      }];
      NSString* png = [output
          stringByAppendingPathComponent:[[file stringByDeletingPathExtension]
                                             stringByAppendingString:@".png"]];
      WritePNG(bitmap, png);
      printf("%s %s\n", passed ? "PASS" : "FAIL", file.UTF8String);
    }
    NSDictionary* report = @{
      @"platform": @"skity-software-macos",
      @"reference_generator": manifest[@"generator"] ?: @{},
      @"cases": reports,
      @"failures": @(failures)
    };
    NSData* json =
        [NSJSONSerialization dataWithJSONObject:report
                                        options:NSJSONWritingPrettyPrinted
                                          error:nil];
    [json writeToFile:[output stringByAppendingPathComponent:@"results.json"]
           atomically:YES];
    return failures ? 1 : 0;
  }
}
