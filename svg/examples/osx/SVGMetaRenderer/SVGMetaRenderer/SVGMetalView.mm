// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "SVGMetalView.h"

#include <memory>
#include <string>

#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"

#include <skity/skity.hpp>
#include <skity/gpu/gpu_context_mtl.h>
#include <skity/gpu/gpu_surface.hpp>

using serval::svg::parser::SrSVGDOM;
using serval::svg::skity::SrSkityCanvas;

@interface SVGMetalView () {
    std::unique_ptr<SrSVGDOM> _svgDom;
    std::unique_ptr<skity::GPUContext> _gpuContext;
    std::unique_ptr<skity::GPUSurface> _gpuSurface;
    skity::Canvas* _canvas;
    CVDisplayLinkRef _displayLink;
}

@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, assign) NSInteger vertexCount;

@end

@implementation SVGMetalView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (CAMetalLayer *)metalLayer {
    return (CAMetalLayer *)self.layer;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (void)commonInit {
    // Create and set the metal layer
    CAMetalLayer *metalLayer = [CAMetalLayer new];
    [self setLayer:metalLayer];
    self.wantsLayer = YES;
    
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.opaque = NO;
    metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
    
    self.device = metalLayer.device;
    self.commandQueue = [self.device newCommandQueue];
    
    [self setupSkity];
    [self setupCADisplayLink];
}

- (void)setupSkity {
    // Create Skity GPU context
    _gpuContext = skity::MTLContextCreate(self.device, self.commandQueue);
    
    // Create GPU surface
    CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
    skity::GPUSurfaceDescriptorMTL desc{};
    desc.backend = skity::GPUBackendType::kMetal;
    desc.width = metalLayer.frame.size.width;
    desc.height = metalLayer.frame.size.height;
    desc.content_scale = metalLayer.contentsScale;
    desc.sample_count = 4;
    desc.surface_type = skity::MTLSurfaceType::kLayer;
    desc.layer = metalLayer;
    
    _gpuSurface = _gpuContext->CreateSurface(&desc);
}

- (void)setupCADisplayLink {
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    
    if (_displayLink) {
        CVDisplayLinkSetOutputCallback(_displayLink, DisplayLinkCallback, (__bridge void *)self);
        CVDisplayLinkStart(_displayLink);
    }
}

- (void)dealloc {
    if (_displayLink) {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
    }
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                      const CVTimeStamp *now,
                                      const CVTimeStamp *outputTime,
                                      CVOptionFlags flagsIn,
                                      CVOptionFlags *flagsOut,
                                      void *displayLinkContext) {
    @autoreleasepool {
        SVGMetalView *view = (__bridge SVGMetalView *)displayLinkContext;
        // Only render when main thread is free
        if ([NSThread isMainThread]) {
            [view render];
        } else {
            // Use async dispatch but avoid frequent scheduling
            static BOOL isRendering = NO;
            if (!isRendering) {
                isRendering = YES;
                dispatch_async(dispatch_get_main_queue(), ^{
                    [view render];
                    isRendering = NO;
                });
            }
        }
    }
    return kCVReturnSuccess;
}

- (void)setSVGContent:(NSString *)content {
    if (content == nil) {
        _svgDom.reset();
        return;
    }
    
    std::string svgContent([content UTF8String]);
    _svgDom = SrSVGDOM::make(svgContent.c_str(), svgContent.length());
}

- (void)loadSVGFromFile:(NSString *)path {
    NSError *error = nil;
    NSString *content = [NSString stringWithContentsOfFile:path
                                                  encoding:NSUTF8StringEncoding
                                                     error:&error];
    if (error) {
        NSLog(@"Failed to load SVG file: %@", error);
        return;
    }
    
    [self setSVGContent:content];
}

- (void)render {
    if (!_gpuSurface) {
        return;
    }
    
    // Lock canvas
    _canvas = _gpuSurface->LockCanvas();
    if (!_canvas) {
        return;
    }
    
    // Clear with white background
    _canvas->DrawColor(skity::Color_WHITE);
    
    // Render SVG if available
    if (_svgDom) {
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
        SrSkityCanvas srCanvas(_canvas, [](std::string url) -> std::shared_ptr<skity::Image> {
            // Image callback - implement image loading here if needed
            return nullptr;
        });
        
        SrSVGBox viewPort{
            0.f,
            0.f,
            static_cast<float>(metalLayer.frame.size.width),
            static_cast<float>(metalLayer.frame.size.height)
        };
        
        _svgDom->Render(&srCanvas, viewPort);
    } else {
        // Draw fallback text if no SVG loaded
        skity::Paint paint;
        paint.SetTextSize(24.f);
        paint.SetFillColor(0, 0, 0, 1);
        paint.SetAntiAlias(true);
        
        skity::TextBlobBuilder builder;
        auto typeface = skity::Typeface::GetDefaultTypeface();
        paint.SetTypeface(typeface);
        
        auto blob = builder.BuildTextBlob("No SVG loaded.", paint);
        _canvas->DrawTextBlob(blob.get(), 50.f, 100.f, paint);
    }
    
    // Flush and unlock
    _canvas->Flush();
    _gpuSurface->Flush();
    _canvas = nullptr;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    [self setNeedsDisplay:YES];
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    
    if (_gpuContext) {
        // Recreate surface with new size
        CAMetalLayer *metalLayer = (CAMetalLayer *)self.layer;
        skity::GPUSurfaceDescriptorMTL desc{};
        desc.backend = skity::GPUBackendType::kMetal;
        desc.width = newSize.width;
        desc.height = newSize.height;
        desc.content_scale = metalLayer.contentsScale;
        desc.sample_count = 4;
        desc.surface_type = skity::MTLSurfaceType::kLayer;
        desc.layer = metalLayer;
        
        _gpuSurface = _gpuContext->CreateSurface(&desc);
    }
}

@end
