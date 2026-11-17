// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#import "SvgImageLoader.h"

// Exercise the demo-local view through its existing initializer.
@interface SvgImagePreviewView : UIView
- (instancetype)initWithFrame:(CGRect)frame
                      content:(NSString*)content
                        color:(NSString*)color
                  imageLoader:(SvgImageLoader*)imageLoader
                     fileName:(NSString*)fileName;
@end

static NSMutableDictionary<NSString*, NSNumber*>* requestCounts;
static NSData* responseData;

@interface ImageLoaderProtocol : NSURLProtocol
@end

@implementation ImageLoaderProtocol
+ (BOOL)canInitWithRequest:(NSURLRequest*)request {
  return [request.URL.host isEqualToString:@"image-regression.test"];
}
+ (NSURLRequest*)canonicalRequestForRequest:(NSURLRequest*)request {
  return request;
}
- (void)startLoading {
  @synchronized(ImageLoaderProtocol.class) {
    NSString* key = self.request.URL.absoluteString;
    requestCounts[key] = @([requestCounts[key] unsignedIntegerValue] + 1);
  }
  NSHTTPURLResponse* response =
      [[NSHTTPURLResponse alloc] initWithURL:self.request.URL
                                  statusCode:200
                                 HTTPVersion:@"HTTP/1.1"
                                headerFields:@{@"Content-Type": @"image/png"}];
  [self.client URLProtocol:self
        didReceiveResponse:response
        cacheStoragePolicy:NSURLCacheStorageNotAllowed];
  [self.client URLProtocol:self didLoadData:responseData];
  [self.client URLProtocolDidFinishLoading:self];
}
- (void)stopLoading {
}
@end

static NSUInteger RequestCount(NSString* url) {
  @synchronized(ImageLoaderProtocol.class) {
    return [requestCounts[url] unsignedIntegerValue];
  }
}

@interface ImageLoaderTrackingView : UIView
@property(nonatomic, copy) void (^onDisplay)(void);
@end

@implementation ImageLoaderTrackingView
- (void)setNeedsDisplay {
  [super setNeedsDisplay];
  if (self.onDisplay) {
    self.onDisplay();
  }
}
@end

@interface ImageLoaderTrackingPreview : SvgImagePreviewView
@property(nonatomic, copy) void (^onDisplay)(void);
@end

@implementation ImageLoaderTrackingPreview
- (void)setNeedsDisplay {
  [super setNeedsDisplay];
  if (self.onDisplay) {
    self.onDisplay();
  }
}
@end

static NSUInteger RedPixelCount(UIView* view) {
  UIGraphicsBeginImageContextWithOptions(view.bounds.size, NO, 1);
  [view drawRect:view.bounds];
  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  size_t width = CGImageGetWidth(image.CGImage);
  size_t height = CGImageGetHeight(image.CGImage);
  NSMutableData* data = [NSMutableData dataWithLength:width * height * 4];
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      data.mutableBytes, width, height, 8, width * 4, space,
      (CGBitmapInfo)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), image.CGImage);
  const uint8_t* bytes = data.bytes;
  NSUInteger count = 0;
  for (NSUInteger i = 0; i < width * height; ++i) {
    if (bytes[4 * i] > 180 && bytes[4 * i + 1] < 40 && bytes[4 * i + 2] < 40 &&
        bytes[4 * i + 3] > 180) {
      ++count;
    }
  }
  CGContextRelease(context);
  CGColorSpaceRelease(space);
  return count;
}

@interface ImageLoaderTests : XCTestCase
@end

@implementation ImageLoaderTests

- (void)setUp {
  [super setUp];
  requestCounts = [NSMutableDictionary dictionary];
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(16, 16), NO, 1);
  [[UIColor redColor] setFill];
  UIRectFill(CGRectMake(0, 0, 16, 16));
  responseData =
      UIImagePNGRepresentation(UIGraphicsGetImageFromCurrentImageContext());
  UIGraphicsEndImageContext();
}

- (SvgImageLoader*)loader {
  SvgImageLoader* loader = [[SvgImageLoader alloc] init];
  [[loader valueForKey:@"session"] invalidateAndCancel];
  NSURLSessionConfiguration* configuration =
      [NSURLSessionConfiguration ephemeralSessionConfiguration];
  configuration.protocolClasses = @[ImageLoaderProtocol.class];
  [loader setValue:[NSURLSession sessionWithConfiguration:configuration]
            forKey:@"session"];
  return loader;
}

- (void)testSharedRequestsCoalesceAndNotifyBothViewsOnMainThread {
  SvgImageLoader* loader = [self loader];
  NSString* url = @"https://image-regression.test/shared.png";
  ImageLoaderTrackingView* first = [[ImageLoaderTrackingView alloc] init];
  ImageLoaderTrackingView* second = [[ImageLoaderTrackingView alloc] init];
  XCTestExpectation* ready =
      [self expectationWithDescription:@"Both views redraw"];
  ready.expectedFulfillmentCount = 2;
  first.onDisplay = ^{
    XCTAssertTrue(NSThread.isMainThread);
    [ready fulfill];
  };
  second.onDisplay = first.onDisplay;
  XCTAssertNil([loader imageForHref:url view:first]);
  XCTAssertNil([loader imageForHref:url view:first]);
  XCTAssertNil([loader imageForHref:url view:second]);
  [self waitForExpectations:@[ready] timeout:5];
  XCTAssertEqual(RequestCount(url), 1u);
  XCTAssertNotNil([loader imageForHref:url view:first]);
  XCTAssertEqual([loader imageForHref:url view:first],
                 [loader imageForHref:url view:second]);
  XCTAssertEqual(RequestCount(url), 1u);
}

- (void)testPendingRequestDoesNotRetainItsView {
  SvgImageLoader* loader = [self loader];
  NSString* url = @"https://image-regression.test/weak-view.png";
  __weak UIView* weakView;
  @autoreleasepool {
    UIView* view = [[UIView alloc] init];
    weakView = view;
    XCTAssertNil([loader imageForHref:url view:view]);
  }
  XCTAssertNil(weakView);

  ImageLoaderTrackingView* replacement = [[ImageLoaderTrackingView alloc] init];
  XCTestExpectation* ready =
      [self expectationWithDescription:@"Replacement view redraws"];
  replacement.onDisplay = ^{
    XCTAssertTrue(NSThread.isMainThread);
    [ready fulfill];
  };
  XCTAssertNil([loader imageForHref:url view:replacement]);
  [self waitForExpectations:@[ready] timeout:5];
  XCTAssertEqual(RequestCount(url), 1u);
  XCTAssertNotNil([loader imageForHref:url view:replacement]);
}

- (void)testActualPreviewRepaintsAfterRemoteImageCompletes {
  SvgImageLoader* loader = [self loader];
  NSString* url = @"https://image-regression.test/preview.png";
  NSString* content = [NSString
      stringWithFormat:
          @"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'>"
           "<image width='32' height='32' href='%@'/></svg>",
          url];
  ImageLoaderTrackingPreview* preview =
      [[ImageLoaderTrackingPreview alloc] initWithFrame:CGRectMake(0, 0, 32, 32)
                                                content:content
                                                  color:nil
                                            imageLoader:loader
                                               fileName:@"loader-preview.svg"];
  XCTestExpectation* ready =
      [self expectationWithDescription:@"Preview redraws"];
  preview.onDisplay = ^{
    XCTAssertTrue(NSThread.isMainThread);
    [ready fulfill];
  };
  XCTAssertEqual(RedPixelCount(preview), 0u);
  [self waitForExpectations:@[ready] timeout:5];
  XCTAssertGreaterThan(RedPixelCount(preview), 900u);
  XCTAssertEqual(RequestCount(url), 1u);
}

@end
