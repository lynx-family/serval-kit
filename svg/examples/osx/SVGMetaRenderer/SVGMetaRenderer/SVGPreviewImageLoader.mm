// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SVGPreviewImageLoader.h"
#import <ImageIO/ImageIO.h>

#include <libkern/OSByteOrder.h>

#include <zlib.h>
#include <cstring>
#include <unordered_map>

#include "parser/SrSVGTraversalState.h"
#include "skity/graphic/image.hpp"

namespace {
using serval::svg::parser::SrSVGTraversalState;
constexpr NSUInteger kMaxDownloadBytes = 16 * 1024 * 1024;

// ImageIO can report a complete image even when the PNG's IDAT is corrupt.
bool HasIntactPNGChunks(NSData* data) {
  const auto* bytes = static_cast<const uint8_t*>(data.bytes);
  size_t remaining = data.length;
  if (remaining < 8 || memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) != 0)
    return false;
  bytes += 8;
  remaining -= 8;
  while (remaining >= 12) {
    const uint32_t length = OSReadBigInt32(bytes, 0);
    if (length > remaining - 12 ||
        crc32(0, bytes + 4, length + 4) != OSReadBigInt32(bytes, 8 + length))
      return false;
    if (memcmp(bytes + 4, "IEND", 4) == 0)
      return length == 0;
    bytes += 12 + length;
    remaining -= 12 + length;
  }
  return false;
}

std::shared_ptr<skity::Image> DecodeImage(NSData* data, bool require_png,
                                          size_t pixel_budget) {
  if (!data.length || data.length > kMaxDownloadBytes)
    return nullptr;
  CGImageSourceRef source =
      CGImageSourceCreateWithData((__bridge CFDataRef)data, nullptr);
  if (!source)
    return nullptr;
  NSString* type = (__bridge NSString*)CGImageSourceGetType(source);
  bool png = [type isEqualToString:@"public.png"];
  NSDictionary* properties =
      CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(source, 0, nullptr));
  size_t width = [properties[(__bridge NSString*)kCGImagePropertyPixelWidth]
      unsignedIntegerValue];
  size_t height = [properties[(__bridge NSString*)kCGImagePropertyPixelHeight]
      unsignedIntegerValue];
  const size_t pixels = SrSVGTraversalState::OffscreenPixels(width, height);
  CGImageRef decoded = nullptr;
  if (pixels && pixels <= pixel_budget && (!require_png || png) &&
      (!png || HasIntactPNGChunks(data))) {
    decoded = CGImageSourceCreateImageAtIndex(
        source, 0,
        (__bridge CFDictionaryRef)
            @{(__bridge NSString*)kCGImageSourceShouldCacheImmediately: @YES});
  }
  CFRelease(source);
  if (!decoded)
    return nullptr;

  auto pixmap = std::make_shared<skity::Pixmap>(
      width, height, skity::AlphaType::kPremul_AlphaType);
  CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGContextRef context =
      pixmap->WritableAddr() && space
          ? CGBitmapContextCreate(
                pixmap->WritableAddr(), width, height, 8, pixmap->RowBytes(),
                space, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast)
          : nullptr;
  if (space)
    CGColorSpaceRelease(space);
  if (!context) {
    CGImageRelease(decoded);
    return nullptr;
  }
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), decoded);
  CGContextRelease(context);
  CGImageRelease(decoded);
  return skity::Image::MakeImage(std::move(pixmap));
}
}  // namespace

@interface SVGPreviewImageLoader () <NSURLSessionDownloadDelegate>
@end

@implementation SVGPreviewImageLoader {
  std::unordered_map<std::string, std::shared_ptr<skity::Image>> _images;
  size_t _remainingPixels;
  NSURLSession* _session;
  dispatch_block_t _redraw;
}

- (instancetype)initWithRedraw:(dispatch_block_t)redraw {
  if ((self = [super init])) {
    _redraw = [redraw copy];
    _remainingPixels = SrSVGTraversalState::kMaxOffscreenPixels;
  }
  return self;
}

- (void)invalidate {
  _redraw = nil;
  [_session invalidateAndCancel];
  _session = nil;
  _images.clear();
}

- (std::shared_ptr<skity::Image>)imageForHref:(const std::string&)href {
  // A null entry represents either an outstanding request or a failed image.
  auto [it, first] = _images.try_emplace(href);
  if (!first || !_remainingPixels)
    return it->second;
  NSString* key = [[NSString alloc] initWithBytes:href.data()
                                           length:href.size()
                                         encoding:NSUTF8StringEncoding];
  if (!key)
    return nullptr;
  NSString* prefix = @"data:image/png;base64,";
  if ([key rangeOfString:prefix
                 options:NSAnchoredSearch | NSCaseInsensitiveSearch]
          .location == 0) {
    NSString* payload = [key substringFromIndex:prefix.length];
    NSData* data = [[NSData alloc]
        initWithBase64EncodedString:payload
                            options:
                                NSDataBase64DecodingIgnoreUnknownCharacters];
    auto image = DecodeImage(data, true, _remainingPixels);
    if (image)
      _remainingPixels -= image->Width() * image->Height();
    return it->second = std::move(image);
  }

  NSURL* url = [NSURL URLWithString:key];
  NSString* scheme = url.scheme.lowercaseString;
  if (!url.host.length ||
      !([scheme isEqualToString:@"https"] || [scheme isEqualToString:@"http"]))
    return nullptr;
  if (!_session) {
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    queue.maxConcurrentOperationCount = 1;
    _session = [NSURLSession
        sessionWithConfiguration:NSURLSessionConfiguration
                                     .ephemeralSessionConfiguration
                        delegate:self
                   delegateQueue:queue];
  }
  NSURLSessionDownloadTask* task = [_session downloadTaskWithURL:url];
  task.taskDescription = key;
  [task resume];
  return nullptr;
}

- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionDownloadTask*)task
                 didWriteData:(int64_t)bytesWritten
            totalBytesWritten:(int64_t)written
    totalBytesExpectedToWrite:(int64_t)expected {
  if (written > (int64_t)kMaxDownloadBytes ||
      expected > (int64_t)kMaxDownloadBytes)
    [task cancel];
}

- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionDownloadTask*)task
    didFinishDownloadingToURL:(NSURL*)location {
  NSHTTPURLResponse* response = (NSHTTPURLResponse*)task.response;
  NSNumber* fileSize = nil;
  if (![response isKindOfClass:NSHTTPURLResponse.class] ||
      response.statusCode < 200 || response.statusCode >= 300 ||
      ![location getResourceValue:&fileSize
                           forKey:NSURLFileSizeKey
                            error:nil] ||
      fileSize.unsignedLongLongValue > kMaxDownloadBytes)
    return;
  NSData* data = [NSData dataWithContentsOfURL:location
                                       options:NSDataReadingMappedIfSafe
                                         error:nil];
  auto image =
      DecodeImage(data, false, SrSVGTraversalState::kMaxOffscreenPixels);
  if (!image)
    return;
  NSString* key = task.taskDescription;
  // Serial delegate queue. Hand off one decoded image before decoding the next.
  dispatch_sync(dispatch_get_main_queue(), ^{
    if (self->_session != session)
      return;
    size_t pixels = image->Width() * image->Height();
    if (pixels <= self->_remainingPixels) {
      auto entry = self->_images.find(key.UTF8String);
      if (entry == self->_images.end())
        return;
      entry->second = image;
      self->_remainingPixels -= pixels;
      self->_redraw();
    }
  });
}
@end
