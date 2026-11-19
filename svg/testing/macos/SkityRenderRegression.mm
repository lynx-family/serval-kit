// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <skity/gpu/gpu_context_mtl.h>
#include <skity/skity.hpp>
#import "SVGMetalView.h"
#import "SVGPreviewImageLoader.h"
#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

@interface SVGMetalView (Regression)
- (void)drawContentOnCanvas:(skity::Canvas*)canvas
                      width:(float)w
                     height:(float)h;
@end

// Observe actual document invalidations without requiring a display link.
@interface RegressionView : SVGMetalView
@property(nonatomic) NSUInteger redraws;
@end
@implementation RegressionView
- (void)render {
  self.redraws++;
}
@end

namespace {
using serval::svg::parser::SrSVGDOM;
using serval::svg::skity::SrSkityCanvas;
using Pixel = std::array<uint8_t, 4>;
using Frame = std::vector<Pixel>;

void Check(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL %s\n", message);
    std::exit(1);
  }
}

NSString* Fixture(NSString* directory, NSString* name) {
  auto text = [NSString
      stringWithContentsOfFile:[directory stringByAppendingPathComponent:name]
                      encoding:NSUTF8StringEncoding
                         error:nil];
  Check(text.length > 0, name.UTF8String);
  return text;
}

Frame ReadFrame(skity::GPUContext* gpu, uint32_t width, uint32_t height,
                const std::function<void(skity::Canvas*)>& draw) {
  skity::GPURenderTargetDescriptor desc{};
  desc.width = width;
  desc.height = height;
  desc.sample_count = 4;
  auto target = gpu->CreateRenderTarget(desc);
  Check(target && target->GetCanvas(), "Metal render target");
  draw(target->GetCanvas());
  auto image = gpu->MakeSnapshot(std::move(target));
  auto map = image ? image->ReadPixels(gpu) : nullptr;
  Check(map && map->Addr() && map->Width() == width && map->Height() == height,
        "Metal pixel readback");
  bool bgra = map->GetColorType() == skity::ColorType::kBGRA;
  Check(bgra || map->GetColorType() == skity::ColorType::kRGBA, "pixel format");
  Frame result(width * height);
  for (size_t y = 0; y < height; ++y) {
    const auto* row =
        static_cast<const uint8_t*>(map->Addr()) + y * map->RowBytes();
    std::memcpy(result.data() + y * width, row, width * sizeof(Pixel));
  }
  if (bgra)
    for (auto& p : result)
      std::swap(p[0], p[2]);
  return result;
}

Frame RenderPreview(skity::GPUContext* gpu, RegressionView* view,
                    uint32_t width, uint32_t height) {
  return ReadFrame(gpu, width, height, [&](skity::Canvas* canvas) {
    [view drawContentOnCanvas:canvas width:width height:height];
  });
}

void SaveFrame(NSString* output, NSString* name, const Frame& pixels,
               size_t width) {
  if (!output)
    return;
  unsigned char* planes[5] = {
      reinterpret_cast<unsigned char*>(const_cast<Pixel*>(pixels.data()))};
  auto bitmap =
      [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:planes
                                              pixelsWide:width
                                              pixelsHigh:pixels.size() / width
                                           bitsPerSample:8
                                         samplesPerPixel:4
                                                hasAlpha:YES
                                                isPlanar:NO
                                          colorSpaceName:NSDeviceRGBColorSpace
                                             bytesPerRow:width * sizeof(Pixel)
                                            bitsPerPixel:32];
  NSData* png = [bitmap representationUsingType:NSBitmapImageFileTypePNG
                                     properties:@{}];
  Check([png writeToFile:[output stringByAppendingPathComponent:name]
              atomically:YES],
        "save PNG");
}

void WaitForImageRedraw(RegressionView* view, NSUInteger before) {
  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:10];
  while (view.redraws == before && deadline.timeIntervalSinceNow > 0)
    [NSRunLoop.currentRunLoop
        runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
  Check(view.redraws == before + 1, "one redraw after image download");
}

void CheckImages(skity::GPUContext* gpu, NSString* directory, NSString* remote,
                 NSString* output) {
  RegressionView* view =
      [[RegressionView alloc] initWithFrame:NSMakeRect(0, 0, 160, 160)];
  auto frame = [&] {
    return RenderPreview(gpu, view, 160, 160);
  };
  NSString* valid = Fixture(directory, @"image.svg");
  NSString* bad = Fixture(directory, @"invalid-image-png.svg");
  auto uri = [](NSString* svg) {
    NSXMLDocument* document = [[NSXMLDocument alloc] initWithXMLString:svg
                                                               options:0
                                                                 error:nil];
    return [[[document.rootElement
                 elementsForLocalName:@"image"
                                  URI:@"http://www.w3.org/2000/svg"]
                 .firstObject attributeForName:@"href"] stringValue];
  };
  auto svg = [](NSString* url, NSString* attributes) {
    return [NSString
        stringWithFormat:
            @"<svg width='160' height='160'><image href='%@' %@/></svg>", url,
            attributes];
  };
  [view setSVGContent:valid];
  auto bundled = frame();
  Check(bundled[30 * 160 + 10] == Pixel{20, 130, 200, 255},
        "bundled image fixture");
  NSString* box = @"x='20' y='20' width='100' height='100'";
  [view setSVGContent:svg(uri(valid), box)];
  const Frame expected = frame();
  for (size_t y = 0; y < 160; ++y)
    for (size_t x = 0; x < 160; ++x)
      Check((expected[y * 160 + x][3] != 0) ==
                (x >= 20 && x < 120 && y >= 20 && y < 120),
            "image bounds must be exactly (20,20) 100x100");
  Check(expected[35 * 160 + 35] == Pixel{20, 130, 200, 255} &&
            expected[35 * 160 + 90] == Pixel{240, 150, 45, 255} &&
            expected[90 * 160 + 35] == Pixel{42, 170, 96, 255} &&
            expected[90 * 160 + 90] == Pixel{135, 70, 185, 255},
        "orientation and color");

  SaveFrame(output, @"image-embedded.png", expected, 160);
  skity::Bitmap bitmap(160, 160, skity::AlphaType::kPremul_AlphaType);
  auto software = skity::Canvas::MakeSoftwareCanvas(&bitmap);
  Check(bool(software), "software canvas");
  [view drawContentOnCanvas:software.get() width:160 height:160];
  software->Flush();
  Check(bitmap.GetPixel(35, 35) == skity::ColorSetARGB(255, 20, 130, 200) &&
            bitmap.GetPixel(90, 90) == skity::ColorSetARGB(255, 135, 70, 185),
        "same cached image on software canvas");

  for (NSString* mode in @[@"xMidYMid meet", @"xMidYMid slice", @"none"]) {
    NSString* attrs =
        [NSString stringWithFormat:@"x='20' y='30' width='100' height='50' "
                                   @"opacity='0.5' preserveAspectRatio='%@'",
                                   mode];
    [view setSVGContent:svg(uri(valid), attrs)];
    auto pixels = frame();
    bool meet = [mode hasSuffix:@"meet"];
    for (size_t y = 0; y < 160; ++y)
      for (size_t x = 0; x < 160; ++x) {
        uint8_t alpha = pixels[y * 160 + x][3];
        bool inside =
            y >= 30 && y < 80 && x >= (meet ? 45 : 20) && x < (meet ? 95 : 120);
        Check(inside ? (alpha >= 126 && alpha <= 128) : alpha == 0,
              mode.UTF8String);
      }
  }

  SVGPreviewImageLoader* loader =
      [[SVGPreviewImageLoader alloc] initWithRedraw:^{}];
  auto image = [loader imageForHref:std::string(uri(valid).UTF8String)];
  Check(bool(image), "image for geometry checks");
  auto rejected = ReadFrame(gpu, 16, 16, [&](skity::Canvas* canvas) {
    int lookups = 0;
    SrSkityCanvas svg(canvas, [&](const std::string&) {
      ++lookups;
      return image;
    });
    const SrSVGPreserveAspectRatio meet{SR_SVG_AlIGN_MID, SR_SVG_AlIGN_MID,
                                        SR_SVG_SCALE_MEET};
    const float large = std::numeric_limits<float>::max();
    svg.DrawImage("", 0, 0, 16, 16, meet, 1);
    svg.DrawImage("image", 0, 0, 0, 16, meet, 1);
    svg.DrawImage("image", 0, std::numeric_limits<float>::quiet_NaN(), 16, 16,
                  meet, 1);
    svg.DrawImage("image", large, 0, large, 16, meet, 1);
    Check(lookups == 0, "invalid image bounds or empty href trigger no lookup");
    // The viewport is finite, but centering this 50x50 image overflows.
    svg.DrawImage("image", 0, 0, large, 16, meet, 1);
    Check(lookups == 1, "valid viewport proceeds to image transform");
  });
  Check(std::all_of(rejected.begin(), rejected.end(),
                    [](Pixel p) { return p[3] == 0; }),
        "invalid image transform leaves the canvas empty");
  Check(![loader imageForHref:std::string(uri(bad).UTF8String)],
        "damaged PNG decode");
  [loader invalidate];
  [view setSVGContent:bad];
  auto broken = frame();
  Check(std::any_of(broken.begin(), broken.end(),
                    [](Pixel p) { return p[0] && !p[1] && !p[2]; }),
        "sibling red outline survives failed PNG");
  Check(broken[30 * 160 + 15][3] == 0, "damaged PNG interior stays empty");

  [view setSVGContent:svg(remote, box)];
  NSUInteger before = view.redraws;
  auto loading = frame();
  Check(std::all_of(loading.begin(), loading.end(),
                    [](Pixel p) { return p[3] == 0; }),
        "pending image");
  frame();  // A second frame while downloading must share the request.
  WaitForImageRedraw(view, before);
  auto downloaded = frame();
  Check(downloaded == expected && frame() == expected,
        "download redraw and cached pixels match embedded PNG");
  SaveFrame(output, @"image-remote.png", downloaded, 160);
  std::puts("PNG, remote redraw, placement, orientation, opacity, "
            "meet/slice/none and failure passed");
}

void CheckPatternSamples(skity::GPUContext* gpu, NSString* directory,
                         NSString* manifestPath, NSString* output) {
  NSData* data = [NSData dataWithContentsOfFile:manifestPath];
  Check(data != nil, "pattern reference manifest");
  NSDictionary* manifest = [NSJSONSerialization JSONObjectWithData:data
                                                           options:0
                                                             error:nil];
  NSArray* cases = manifest[@"cases"];
  Check([manifest[@"schema_version"] isEqual:@1] && cases.count > 0,
        "pattern reference cases");
  for (NSDictionary* item in cases) {
    NSString* file = item[@"file"];
    uint32_t width = [item[@"width"] unsignedIntValue];
    uint32_t height = [item[@"height"] unsignedIntValue];
    RegressionView* view =
        [[RegressionView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
    [view setSVGContent:Fixture(directory, file)];
    auto frame = RenderPreview(gpu, view, width, height);
    SaveFrame(output,
              [file.stringByDeletingPathExtension
                  stringByAppendingPathExtension:@"png"],
              frame, width);
    NSArray* samples = item[@"samples"];
    Check(samples.count > 0, file.UTF8String);
    for (NSDictionary* sample in samples) {
      uint32_t x = [sample[@"x"] unsignedIntValue];
      uint32_t y = [sample[@"y"] unsignedIntValue];
      NSArray* rgba = sample[@"rgba"];
      Check(x < width && y < height && rgba.count == 4, file.UTF8String);
      const Pixel& pixel = frame[y * width + x];
      for (int c = 0; c < 4; ++c) {
        // Metal readback is premultiplied. Chrome's samples are straight RGBA.
        int expected = [rgba[c] intValue];
        if (c < 3)
          expected = (expected * [rgba[3] intValue] + 127) / 255;
        if (std::abs(pixel[c] - expected) > 2) {
          std::fprintf(stderr,
                       "FAIL %s (%u,%u) channel %d: got %u, expected %d\n",
                       file.UTF8String, x, y, c, pixel[c], expected);
          std::exit(1);
        }
      }
    }
  }
  std::printf(
      "%lu pattern and control fixtures passed Chrome reference samples\n",
      (unsigned long)cases.count);

  RegressionView* view =
      [[RegressionView alloc] initWithFrame:NSMakeRect(0, 0, 160, 160)];
  [view setSVGContent:Fixture(directory, @"pattern-image-stroke.svg")];
  auto stroke = RenderPreview(gpu, view, 160, 160);
  Check(stroke[22 * 160 + 40] == Pixel{0, 255, 0, 255} &&
            stroke[22 * 160 + 56] == Pixel{255, 255, 0, 255} &&
            stroke[34 * 160 + 40] == Pixel{255, 0, 0, 255} &&
            stroke[34 * 160 + 72] == Pixel{255, 0, 0, 255} &&
            stroke[80 * 160 + 80][3] == 0 && stroke[5 * 160 + 5][3] == 0,
        "pattern image repeats within the stroke only");
  SaveFrame(output, @"pattern-image-stroke.png", stroke, 160);
}

void CheckRemotePatterns(skity::GPUContext* gpu, NSString* directory,
                         NSString* remote, NSString* output) {
  for (NSString* name in @[
         @"pattern-image-remote-whitelist",
         @"pattern-image-remote-non-whitelist"
       ]) {
    NSString* file = [name stringByAppendingPathExtension:@"svg"];
    NSXMLDocument* document =
        [[NSXMLDocument alloc] initWithXMLString:Fixture(directory, file)
                                         options:0
                                           error:nil];
    NSArray* refs = [document
        nodesForXPath:@"//*[local-name()='pattern']/*[local-name()='image']/"
                      @"@*[local-name()='href']"
                error:nil];
    NSXMLNode* href = refs.firstObject;
    Check(refs.count == 1 && [href.stringValue hasPrefix:@"https://"],
          file.UTF8String);
    // Keep the bundled pattern and its circular clip; serve a known PNG on loopback.
    href.stringValue = [remote stringByAppendingFormat:@"?%@", name];
    RegressionView* view =
        [[RegressionView alloc] initWithFrame:NSMakeRect(0, 0, 200, 200)];
    [view setSVGContent:document.XMLString];
    NSUInteger before = view.redraws;
    auto pending = RenderPreview(gpu, view, 200, 200);
    Check(pending[60 * 200 + 60][3] == 0, "pending remote pattern");
    WaitForImageRedraw(view, before);
    auto loaded = RenderPreview(gpu, view, 200, 200);
    Check(loaded[60 * 200 + 60] == Pixel{20, 130, 200, 255} &&
              loaded[60 * 200 + 140] == Pixel{240, 150, 45, 255} &&
              loaded[140 * 200 + 60] == Pixel{42, 170, 96, 255} &&
              loaded[140 * 200 + 140] == Pixel{135, 70, 185, 255} &&
              loaded[10 * 200 + 10][3] == 0,
          file.UTF8String);
    SaveFrame(output, [name stringByAppendingPathExtension:@"png"], loaded,
              200);
  }
}

void CheckPathsAndXML(skity::GPUContext* gpu, NSString* directory,
                      NSString* output) {
  auto domFor = [&](NSString* name) {
    std::string xml(Fixture(directory, name).UTF8String);
    return SrSVGDOM::make(xml.data(), xml.size(), nullptr);
  };
  auto dom = domFor(@"smil-path-morph.svg");
  Check(dom && dom->HasAnimations(), "SMIL DOM");
  auto at = [&](double seconds) {
    return ReadFrame(gpu, 320, 180, [&](skity::Canvas* canvas) {
      SrSkityCanvas svg(canvas, {});
      dom->RenderAtTime(&svg, {0, 0, 320, 180}, seconds);
    });
  };
  auto first = at(0), middle = at(1), last = at(2);
  SaveFrame(output, @"smil-0s.png", first, 320);
  SaveFrame(output, @"smil-1s.png", middle, 320);
  SaveFrame(output, @"smil-2s.png", last, 320);
  Check(first == last && first != middle, "SMIL path loop");
  Check(first[100 * 320 + 115] != middle[100 * 320 + 115],
        "triangle interpolation");
  Check(first[65 * 320 + 226] != middle[65 * 320 + 226], "cubic interpolation");

  for (NSString* name in @[
         @"invalid-attr-gt-mismatched-close-tag.svg",
         @"invalid-leading-text-root.svg", @"invalid-mismatched-close-tag.svg",
         @"invalid-multiple-root-tags.svg",
         @"invalid-text-mismatched-close-tag.svg",
         @"invalid-unclosed-root-tag.svg"
       ]) {
    Check(!domFor(name), name.UTF8String);
  }
  auto comment = domFor(@"invalid-comment-gt-before-close.svg");
  Check(bool(comment), "greater-than inside XML comment");
  auto pixels = ReadFrame(gpu, 120, 120, [&](skity::Canvas* canvas) {
    SrSkityCanvas svg(canvas, {});
    comment->Render(&svg);
  });
  Check(pixels[60 * 120 + 60] == Pixel{79, 107, 255, 255},
        "valid comment fixture renders");
  std::puts("SMIL path frames and all seven XML fixtures passed");
}
}  // namespace

int main(int argc, char** argv) {
  @autoreleasepool {
    Check(argc == 4 || argc == 5, "expected fixtures, image URL, pattern "
                                  "manifest and optional output directory");
    NSString* output = argc == 5 ? @(argv[4]) : nil;
    [NSApplication sharedApplication];
    auto device = MTLCreateSystemDefaultDevice();
    Check(device, "Metal device");
    auto gpu = skity::MTLContextCreate(device, [device newCommandQueue]);
    Check(bool(gpu), "Metal context");
    std::printf("Metal device: %s\n", device.name.UTF8String);
    CheckImages(gpu.get(), @(argv[1]), @(argv[2]), output);
    CheckPatternSamples(gpu.get(), @(argv[1]), @(argv[3]), output);
    CheckRemotePatterns(gpu.get(), @(argv[1]), @(argv[2]), output);
    CheckPathsAndXML(gpu.get(), @(argv[1]), output);
  }
}
