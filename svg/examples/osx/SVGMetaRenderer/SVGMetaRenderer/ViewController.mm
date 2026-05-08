// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ViewController.h"
#import "SVGMetalView.h"

@implementation ViewController

- (void)loadView {
    [super loadView];
    self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)];
    [self.view setWantsLayer:YES];
    [self.view.layer setBackgroundColor:[[NSColor whiteColor] CGColor]];
    self.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Create Metal view
    NSRect metalFrame = NSMakeRect(20, 20, 760, 560);
    SVGMetalView *metalView = [[SVGMetalView alloc] initWithFrame:metalFrame];
    metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable | NSViewMinXMargin | NSViewMinYMargin;
    [self.view addSubview:metalView];
    self.metalView = metalView;
    
    // Load initial SVG
    [self loadSVGFile:@"basic_shapes.svg"];
}

- (void)loadSVGFile:(NSString *)fileName {
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    NSString *svgPath = [resourcePath stringByAppendingPathComponent:
                         [@"svg" stringByAppendingPathComponent:fileName]];
    
    // Try loading from bundle first
    if ([[NSFileManager defaultManager] fileExistsAtPath:svgPath]) {
        [(SVGMetalView *)self.metalView loadSVGFromFile:svgPath];
        return;
    }
    
    // Fallback: try to load from development path
    NSString *devPath = [[NSString stringWithUTF8String:__FILE__] stringByDeletingLastPathComponent];
    devPath = [devPath stringByAppendingPathComponent:@"svg"];
    devPath = [devPath stringByAppendingPathComponent:fileName];
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:devPath]) {
        [(SVGMetalView *)self.metalView loadSVGFromFile:devPath];
    }
}

@end
