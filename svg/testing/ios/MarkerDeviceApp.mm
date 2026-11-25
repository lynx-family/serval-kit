// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#include <cmath>
#include "platform/iOS/SrSVG.h"

// Standalone physical-device regression app. Bundle marker_cases.json and its
// SVG fixtures, then compile against the current shared and iOS sources.
@interface MarkerDeviceApp : UIResponder <UIApplicationDelegate>
@end

@interface MarkerDeviceScene : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow* window;
@end

@implementation MarkerDeviceApp
@end

@implementation MarkerDeviceScene
- (void)scene:(UIScene*)scene
    willConnectToSession:(UISceneSession*)session
                 options:(UISceneConnectionOptions*)connectionOptions {
  self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
  UIViewController* controller = [[UIViewController alloc] init];
  self.window.rootViewController = controller;
  controller.view.backgroundColor = UIColor.whiteColor;
  [self.window makeKeyAndVisible];
  dispatch_async(dispatch_get_main_queue(),
                 ^{ [self runRegression:controller]; });
}

- (void)runRegression:(UIViewController*)controller {
  NSBundle* bundle = NSBundle.mainBundle;
  NSString* directory = NSSearchPathForDirectoriesInDomains(
                            NSDocumentDirectory, NSUserDomainMask, YES)
                            .firstObject;
  NSDictionary* manifest = [NSJSONSerialization
      JSONObjectWithData:
          [NSData dataWithContentsOfFile:[bundle pathForResource:@"marker_cases"
                                                          ofType:@"json"]]
                 options:0
                   error:nil];
  NSMutableArray* results = [NSMutableArray array];
  NSUInteger probesChecked = 0;
  NSUInteger failures = 0;
  NSUInteger failedCases = 0;
  UIStackView* stack = [[UIStackView alloc]
      initWithFrame:CGRectMake(24, 64, controller.view.bounds.size.width - 48,
                               720)];
  stack.axis = UILayoutConstraintAxisVertical;
  stack.spacing = 6;
  stack.distribution = UIStackViewDistributionFillEqually;
  [controller.view addSubview:stack];
  for (NSDictionary* testCase in manifest[@"cases"]) {
    NSString* name = testCase[@"file"];
    NSUInteger width = [testCase[@"width"] unsignedIntegerValue];
    NSUInteger height = [testCase[@"height"] unsignedIntegerValue];
    NSData* data = [NSData dataWithContentsOfFile:[bundle pathForResource:name
                                                                   ofType:nil]];
    SrSVGRenderResult* diagnostic = nil;
    UIImage* image = [[[SrSVG alloc] init]
        getSrSvgDrawImageWithData:data
                          andSize:CGSizeMake(width, height)
                         andColor:nil
                           atTime:0
                      andCallback:^UIImage*(NSString* href) { return nil; }
                           result:&diagnostic];
    BOOL valid = image.CGImage && !diagnostic.hasError &&
                 CGImageGetWidth(image.CGImage) == width &&
                 CGImageGetHeight(image.CGImage) == height;
    NSMutableData* bytes = [NSMutableData dataWithLength:width * height * 4];
    CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef context = CGBitmapContextCreate(
        bytes.mutableBytes, width, height, 8, width * 4, space,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    if (valid && context) {
      CGContextSetBlendMode(context, kCGBlendModeCopy);
      CGContextDrawImage(context, CGRectMake(0, 0, width, height),
                         image.CGImage);
    } else {
      valid = NO;
    }
    if (context)
      CGContextRelease(context);
    CGColorSpaceRelease(space);
    NSMutableArray* probes = [NSMutableArray array];
    BOOL passed = valid;
    const uint8_t* pixels = (const uint8_t*)bytes.bytes;
    for (NSDictionary* sample in testCase[@"samples"]) {
      NSUInteger x = [sample[@"x"] unsignedIntegerValue];
      NSUInteger y = [sample[@"y"] unsignedIntegerValue];
      BOOL matches = valid && x < width && y < height;
      NSMutableArray* actual = [NSMutableArray array];
      NSArray* expected = sample[@"rgba"];
      if (matches) {
        const uint8_t* pixel = pixels + 4 * (y * width + x);
        for (NSUInteger channel = 0; channel < 4; ++channel) {
          NSInteger value = pixel[channel];
          if (channel < 3 && pixel[3] > 0 && pixel[3] < 255) {
            value = lround(value * 255.0 / pixel[3]);
          }
          [actual addObject:@(value)];
          if (labs(value - [expected[channel] integerValue]) > 3)
            matches = NO;
        }
      }
      [probes addObject:@{
        @"x": @(x),
        @"y": @(y),
        @"expected": expected,
        @"actual": actual,
        @"passed": @(matches)
      }];
      ++probesChecked;
      if (!matches) {
        ++failures;
        passed = NO;
      }
    }
    if (!passed)
      ++failedCases;
    [results addObject:@{
      @"file": name,
      @"passed": @(passed),
      @"probes": probes,
      @"error": diagnostic.errorMessage ?: @""
    }];
    [UIImagePNGRepresentation(image)
        writeToFile:[directory
                        stringByAppendingPathComponent:
                            [name stringByReplacingOccurrencesOfString:@".svg"
                                                            withString:@".png"]]
         atomically:YES];
    UIImageView* imageView = [[UIImageView alloc] initWithImage:image];
    imageView.contentMode = UIViewContentModeScaleAspectFit;
    [stack addArrangedSubview:imageView];
  }
  BOOL passed = results.count > 0 && failures == 0;
  NSDictionary* report = @{
    @"schema_version": @1,
    @"generator": manifest[@"generator"] ?: @{},
    @"device": UIDevice.currentDevice.model,
    @"systemVersion": UIDevice.currentDevice.systemVersion,
    @"passed": @(passed),
    @"caseCount": @(results.count),
    @"probeCount": @(probesChecked),
    @"failedProbeCount": @(failures),
    @"failedCaseCount": @(failedCases),
    @"cases": results
  };
  [[NSJSONSerialization dataWithJSONObject:report
                                   options:NSJSONWritingPrettyPrinted
                                     error:nil]
      writeToFile:[directory stringByAppendingPathComponent:@"results.json"]
       atomically:YES];
  UILabel* status = [[UILabel alloc] initWithFrame:CGRectMake(24, 24, 350, 32)];
  status.text = [NSString
      stringWithFormat:@"%@: %lu cases / %lu probes (%lu failed)",
                       passed ? @"PASS" : @"FAIL", (unsigned long)results.count,
                       (unsigned long)probesChecked, (unsigned long)failures];
  status.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
  [controller.view addSubview:status];
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(),
      ^{
        UIGraphicsBeginImageContextWithOptions(controller.view.bounds.size, NO,
                                               1);
        [controller.view drawViewHierarchyInRect:controller.view.bounds
                              afterScreenUpdates:YES];
        UIImage* screenshot = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        [UIImagePNGRepresentation(screenshot)
            writeToFile:[directory
                            stringByAppendingPathComponent:@"app-view.png"]
             atomically:YES];
        NSLog(@"SVGMARKER_RESULT %@", report);
      });
}
@end

int main(int argc, char* argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil,
                             NSStringFromClass(MarkerDeviceApp.class));
  }
}
