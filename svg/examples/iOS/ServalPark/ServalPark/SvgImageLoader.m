// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SvgImageLoader.h"

@interface SvgImageLoader ()

@property(nonatomic, strong) NSMutableDictionary<NSString*, UIImage*>* cache;
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, NSHashTable<UIView*>*>* pending;
@property(nonatomic, strong) NSMutableSet<NSString*>* failed;
@property(nonatomic, strong) NSURLSession* session;

@end

@implementation SvgImageLoader

- (instancetype)init {
  self = [super init];
  if (self) {
    _cache = [NSMutableDictionary dictionary];
    _pending = [NSMutableDictionary dictionary];
    _failed = [NSMutableSet set];
    NSURLSessionConfiguration* configuration =
        [NSURLSessionConfiguration defaultSessionConfiguration];
    configuration.timeoutIntervalForRequest = 10.0;
    configuration.timeoutIntervalForResource = 30.0;
    _session = [NSURLSession sessionWithConfiguration:configuration];
  }
  return self;
}

- (void)dealloc {
  [_session invalidateAndCancel];
}

- (UIImage*)imageForHref:(NSString*)href view:(UIView*)view {
  NSAssert([NSThread isMainThread],
           @"SVG image callbacks run on the main thread.");
  if (href.length == 0 || [self.failed containsObject:href]) {
    return nil;
  }
  UIImage* cached = self.cache[href];
  if (cached) {
    return cached;
  }

  if ([href hasPrefix:@"data:image/"]) {
    NSRange separator = [href rangeOfString:@";base64,"];
    NSData* data = nil;
    if (separator.location != NSNotFound) {
      NSString* base64 = [href substringFromIndex:NSMaxRange(separator)];
      data = [[NSData alloc]
          initWithBase64EncodedString:base64
                              options:
                                  NSDataBase64DecodingIgnoreUnknownCharacters];
    }
    UIImage* image = data.length > 0 ? [UIImage imageWithData:data] : nil;
    if (image) {
      self.cache[href] = image;
    } else {
      [self.failed addObject:href];
      NSLog(@"Failed to decode embedded SVG image.");
    }
    return image;
  }

  NSURL* url = [NSURL URLWithString:href];
  NSString* scheme = url.scheme.lowercaseString;
  if (url.host.length == 0 || (![scheme isEqualToString:@"http"] &&
                               ![scheme isEqualToString:@"https"])) {
    [self.failed addObject:href];
    NSLog(@"Unsupported SVG image URL: %@", href);
    return nil;
  }

  NSHashTable<UIView*>* listeners = self.pending[href];
  if (listeners) {
    [listeners addObject:view];
    return nil;
  }
  listeners = [NSHashTable weakObjectsHashTable];
  [listeners addObject:view];
  NSString* key = [href copy];
  self.pending[key] = listeners;

  __weak SvgImageLoader* weakSelf = self;
  NSURLSessionDataTask* task = [self.session
        dataTaskWithURL:url
      completionHandler:^(NSData* data, NSURLResponse* response,
                          NSError* error) {
        // NSURLSession invokes this completion off the main thread. Only the
        // decoded image and readiness notification cross back to the draw loop.
        UIImage* image = nil;
        NSString* errorMessage = error.localizedDescription;
        if (!error && [response isKindOfClass:[NSHTTPURLResponse class]]) {
          NSInteger status = ((NSHTTPURLResponse*)response).statusCode;
          if (status == 200) {
            image = [UIImage imageWithData:data];
            if (!image) {
              errorMessage = @"Image decoding failed.";
            }
          } else {
            errorMessage =
                [NSString stringWithFormat:@"HTTP %ld", (long)status];
          }
        }
        dispatch_async(dispatch_get_main_queue(), ^{
          SvgImageLoader* loader = weakSelf;
          if (loader) {
            [loader completeHref:key image:image errorMessage:errorMessage];
          }
        });
      }];
  [task resume];
  return nil;
}

- (void)completeHref:(NSString*)href
               image:(UIImage*)image
        errorMessage:(NSString*)errorMessage {
  NSHashTable<UIView*>* listeners = self.pending[href];
  [self.pending removeObjectForKey:href];
  if (!image) {
    [self.failed addObject:href];
    NSLog(@"Failed to load SVG image: %@ (%@)", href,
          errorMessage ?: @"No image data.");
    return;
  }
  self.cache[href] = image;
  for (UIView* view in listeners) {
    [view setNeedsDisplay];
  }
}

- (void)clearCallbacks {
  NSAssert([NSThread isMainThread],
           @"SVG image callbacks run on the main thread.");
  for (NSHashTable<UIView*>* listeners in self.pending.allValues) {
    [listeners removeAllObjects];
  }
  // Keep in-flight requests and cached images across category changes.
  [self.failed removeAllObjects];
}

@end
