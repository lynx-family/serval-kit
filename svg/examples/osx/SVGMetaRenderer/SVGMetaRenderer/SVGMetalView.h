// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface SVGMetalView : NSView

@property(nonatomic, readonly) CAMetalLayer* metalLayer;
@property(nonatomic, strong) id<MTLDevice> device;
@property(nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property(nonatomic, copy) NSString* color;

- (void)setSVGContent:(NSString*)content;
- (void)loadSVGFromFile:(NSString*)path;
- (void)render;

@end
