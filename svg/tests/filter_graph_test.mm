// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Metal/Metal.h>
#include <gtest/gtest.h>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>
#include "parser/SrSVGDOM.h"
#include "platform/skity/SrSkityCanvas.h"
#include "platform/skity/SrSkityFilterGraph.h"
#include "skity/gpu/gpu_context_mtl.h"
#include "skity/graphic/bitmap.hpp"
#include "skity/recorder/picture_recorder.hpp"
#include "skity/skity.hpp"

namespace {
std::vector<uint8_t> Render(const std::string& svg, bool record = false,
                            uint32_t content_scale = 1) {
  @autoreleasepool {
    auto dom =
        serval::svg::parser::SrSVGDOM::make(svg.c_str(), svg.size(), nullptr);
    if (!dom)
      return {};
    constexpr uint32_t size = 128;
    const uint32_t physical_size = size * content_scale;
    std::shared_ptr<skity::Pixmap> pixels;
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
      return {};
    id<MTLCommandQueue> queue = [device newCommandQueue];
    auto context = skity::MTLContextCreate(device, queue);
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:physical_size
                                    height:physical_size
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModeShared;
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    skity::GPUSurfaceDescriptorMTL surface_descriptor;
    surface_descriptor.backend = skity::GPUBackendType::kMetal;
    surface_descriptor.width = size;
    surface_descriptor.height = size;
    surface_descriptor.sample_count = 4;
    surface_descriptor.content_scale = content_scale;
    surface_descriptor.texture = texture;
    surface_descriptor.surface_type = skity::MTLSurfaceType::kTexture;
    auto surface = context->CreateSurface(&surface_descriptor);
    if (!surface)
      return {};
    auto* canvas = surface->LockCanvas();
    canvas->DrawColor(0, skity::BlendMode::kSrc);
    if (record) {
      skity::PictureRecorder recorder;
      recorder.BeginRecording(skity::Rect::MakeWH(size, size));
      serval::svg::skity::SrSkityCanvas adapter(recorder.GetRecordingCanvas(),
                                                {}, surface->ContentScale());
      dom->Render(&adapter, {0, 0, size, size});
      auto display_list = recorder.FinishRecording();
      if (!display_list)
        return {};
      display_list->Draw(canvas);
    } else {
      serval::svg::skity::SrSkityCanvas adapter(canvas, {},
                                                surface->ContentScale());
      dom->Render(&adapter, {0, 0, size, size});
    }
    canvas->Flush();
    surface->Flush();
    id<MTLCommandBuffer> completed = [queue commandBuffer];
    [completed commit];
    [completed waitUntilCompleted];
    skity::Bitmap readback(physical_size, physical_size,
                           skity::AlphaType::kPremul_AlphaType);
    [texture getBytes:readback.GetPixelAddr()
          bytesPerRow:physical_size * 4
           fromRegion:MTLRegionMake2D(0, 0, physical_size, physical_size)
          mipmapLevel:0];
    pixels = readback.GetPixmap();

    if (!pixels)
      return {};
    std::vector<uint8_t> bytes(size * size * 4);
    for (uint32_t y = 0; y < size; ++y) {
      for (uint32_t x = 0; x < size; ++x) {
        memcpy(bytes.data() + (y * size + x) * 4,
               static_cast<const uint8_t*>(pixels->Addr()) +
                   y * content_scale * pixels->RowBytes() +
                   x * content_scale * 4,
               4);
      }
    }
    return bytes;
  }
}
std::string Svg(const std::string& primitives,
                const std::string& body =
                    "<rect x='40' y='40' width='32' height='32' "
                    "fill='blue' filter='url(#f)'/>") {
  return "<svg xmlns='http://www.w3.org/2000/svg' width='128' height='128' "
         "viewBox='0 0 128 128'><defs><filter id='f' "
         "filterUnits='userSpaceOnUse' x='0' y='0' width='128' height='128' "
         "color-interpolation-filters='sRGB'>" +
         primitives + "</filter></defs>" + body + "</svg>";
}
std::vector<uint8_t> Pixel(const std::vector<uint8_t>& image, int x, int y) {
  if (image.size() != 128 * 128 * 4)
    return {};
  auto p = image.begin() + (y * 128 + x) * 4;
  return {p, p + 4};
}
TEST(SkityFilterGraph, FloodReplacesSource) {
  auto image = Render(Svg("<feFlood flood-color='red'/>"));
  EXPECT_EQ(Pixel(image, 50, 50), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 10, 10), (std::vector<uint8_t>{255, 0, 0, 255}));
}
TEST(SkityFilterGraph, TranslatedFloodFillsNegativeLocalSubregion) {
  const auto image =
      Render("<svg xmlns='http://www.w3.org/2000/svg' width='128' height='128'>"
             "<defs><filter id='f' filterUnits='userSpaceOnUse' x='-30' y='0' "
             "width='60' height='40' color-interpolation-filters='sRGB'>"
             "<feFlood x='-30' y='0' width='60' height='40' flood-color='red'/>"
             "</filter></defs><g transform='translate(50 20)'>"
             "<rect width='10' height='10' filter='url(#f)'/></g></svg>");
  EXPECT_EQ(Pixel(image, 25, 30), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 75, 30), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 15, 30), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, BlurSamplesInputOutsideOutputSubregion) {
  const auto reference =
      Render(Svg("<feGaussianBlur in='SourceAlpha' stdDeviation='4'/>"));
  const auto clipped =
      Render(Svg("<feGaussianBlur in='SourceAlpha' stdDeviation='4' "
                 "x='74' y='36' width='30' height='42'/>"));
  const auto pixel = Pixel(reference, 76, 56);
  ASSERT_EQ(pixel.size(), 4u);
  ASSERT_GT(pixel[3], 0);
  EXPECT_EQ(Pixel(clipped, 76, 56), pixel);
  EXPECT_EQ(Pixel(clipped, 70, 56), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, DownstreamClippingPreservesBlurInputHalo) {
  const std::string blur =
      "<feGaussianBlur in='SourceAlpha' stdDeviation='4' result='blur'/>";
  const auto reference = Render(Svg(blur));
  for (const bool record : {false, true}) {
    SCOPED_TRACE(record);
    const auto clipped =
        Render(Svg(blur + "<feBlend in='blur' in2='SourceAlpha' mode='normal' "
                          "x='74' y='36' width='30' height='42'/>"),
               record);
    EXPECT_EQ(Pixel(clipped, 76, 56), Pixel(reference, 76, 56));
    EXPECT_EQ(Pixel(clipped, 70, 56), (std::vector<uint8_t>{0, 0, 0, 0}));
  }
}
TEST(SkityFilterGraph, RetinaSurfaceSupportsDirectAndRecordedGraphs) {
  const auto svg =
      Svg("<feFlood flood-color='red' result='paint'/>"
          "<feComposite in='paint' in2='SourceAlpha' operator='in'/>");
  for (const bool record : {false, true}) {
    const auto image = Render(svg, record, 2);
    EXPECT_EQ(Pixel(image, 50, 50), (std::vector<uint8_t>{255, 0, 0, 255}));
    EXPECT_EQ(Pixel(image, 10, 10), (std::vector<uint8_t>{0, 0, 0, 0}));
  }
}
TEST(SkityFilterGraph, DisabledBlurPreservesRemainingFilterChain) {
  for (const std::string input : {"SourceGraphic", "SourceAlpha"}) {
    const auto primitives = "<feGaussianBlur in='" + input +
                            "' stdDeviation='DEVIATION'/><feOffset dx='20'/>";
    auto identity = primitives;
    identity.replace(identity.find("DEVIATION"), 9, "0");
    const auto expected = Render(Svg(identity));
    ASSERT_EQ(Pixel(expected, 80, 50).size(), 4u);
    EXPECT_EQ(Pixel(expected, 80, 50)[3], 255);
    for (const std::string deviation : {"-1", "-1 4", "4 -1"}) {
      SCOPED_TRACE(input + " " + deviation);
      auto disabled = primitives;
      disabled.replace(disabled.find("DEVIATION"), 9, deviation);
      EXPECT_EQ(Render(Svg(disabled)), expected);
    }
  }
}
TEST(SkityFilterGraph, SourceAlphaAndNamedInputs) {
  auto image =
      Render(Svg("<feFlood flood-color='red' result='paint'/><feComposite "
                 "in='paint' in2='SourceAlpha' operator='in'/>"));
  EXPECT_EQ(Pixel(image, 50, 50), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 10, 10), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, NormalBlendInputOrder) {
  auto image = Render(Svg(
      "<feFlood flood-color='red' flood-opacity='0.5' result='paint'/><feBlend "
      "in='paint' in2='SourceGraphic' mode='normal'/>"));
  auto p = Pixel(image, 50, 50);
  ASSERT_EQ(p.size(), 4u);
  EXPECT_NEAR(p[0], 128, 1);
  EXPECT_NEAR(p[2], 127, 1);
  EXPECT_EQ(p[3], 255);
}
TEST(SkityFilterGraph, CompositeOperatorsPreservePremultipliedAlpha) {
  struct Expected {
    const char* op;
    std::vector<uint8_t> pixel;
  };
  const std::vector<Expected> cases = {
      {"over", {128, 0, 32, 160}}, {"in", {32, 0, 0, 32}},
      {"out", {96, 0, 0, 96}},     {"atop", {32, 0, 32, 64}},
      {"xor", {96, 0, 32, 128}},
  };
  for (const auto& test : cases) {
    SCOPED_TRACE(test.op);
    auto image = Render(
        Svg("<feFlood flood-color='red' flood-opacity='.5' result='src'/>"
            "<feFlood flood-color='blue' flood-opacity='.25' result='dst'/>"
            "<feComposite in='src' in2='dst' operator='" +
            std::string(test.op) + "'/>"));
    const auto pixel = Pixel(image, 50, 50);
    ASSERT_EQ(pixel.size(), 4u);
    for (size_t channel = 0; channel < pixel.size(); ++channel) {
      EXPECT_NEAR(pixel[channel], test.pixel[channel], 1);
    }
  }
}
TEST(SkityFilterGraph, ReusedResultNamesKeepEarlierDependencies) {
  auto image = Render(Svg(
      "<feFlood flood-color='red' result='paint'/>"
      "<feComposite in='paint' in2='SourceAlpha' operator='in' result='shape'/>"
      "<feFlood flood-color='blue' result='paint'/>"
      "<feBlend in='shape' in2='paint' mode='normal'/>"));
  EXPECT_EQ(Pixel(image, 50, 50), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 10, 10), (std::vector<uint8_t>{0, 0, 255, 255}));
}
TEST(SkityFilterGraph, AlphaArithmeticUsesDifferenceNotPorterDuffOut) {
  auto image = Render(
      Svg("<feColorMatrix in='SourceAlpha' type='matrix' values='0 0 0 0 0 0 0 "
          "0 0 0 0 0 0 0 0 0 0 0 .25 0' result='a'/><feColorMatrix "
          "in='SourceAlpha' type='matrix' values='0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
          "0 0 0 0 .75 0' result='b'/><feComposite in='a' in2='b' "
          "operator='arithmetic' k2='-1' k3='1'/>"));
  auto p = Pixel(image, 50, 50);
  ASSERT_EQ(p.size(), 4u);
  EXPECT_NEAR(p[3], 128, 1);
  EXPECT_EQ(Pixel(image, 10, 10), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, NestedFiltersAndTransforms) {
  auto image =
      Render(Svg("<feFlood flood-color='red' result='a'/><feComposite in='a' "
                 "in2='SourceAlpha' operator='in'/>",
                 "<g transform='translate(10 8)'><g opacity='.5' "
                 "filter='url(#f)'><rect x='40' y='40' width='32' height='32' "
                 "fill='blue' filter='url(#f)'/></g></g>"));
  auto p = Pixel(image, 55, 55);
  ASSERT_EQ(p.size(), 4u);
  EXPECT_NEAR(p[0], 128, 1);
  EXPECT_NEAR(p[3], 128, 1);
  EXPECT_EQ(Pixel(image, 42, 42), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, RecordedPlaybackPreservesNestedTransforms) {
  const auto svg = Svg(
      "<feFlood flood-color='red' result='a'/>"
      "<feComposite in='a' in2='SourceAlpha' operator='in'/>",
      "<g transform='translate(10 8) scale(1.2)' opacity='.5' filter='url(#f)'>"
      "<rect x='30' y='30' width='32' height='32' fill='blue' "
      "stroke='black' stroke-width='2' vector-effect='non-scaling-stroke' "
      "filter='url(#f)'/></g>");
  const auto direct = Render(svg);
  ASSERT_EQ(direct.size(), 128 * 128 * 4u);
  EXPECT_EQ(Render(svg, true), direct);
  const auto pixel = Pixel(direct, 60, 60);
  ASSERT_EQ(pixel.size(), 4u);
  EXPECT_NEAR(pixel[0], 128, 1);
  EXPECT_NEAR(pixel[3], 128, 1);
}
using Graph = serval::svg::skity::SrSkityFilterGraph;
using Model = serval::svg::canvas::SrFilterModel;
using Primitive = serval::svg::canvas::SrFilterPrimitiveModel;
using Type = serval::svg::canvas::SrFilterPrimitiveType;
Model BlurModel(float sigma) {
  Model m;
  m.region = {0, 0, 128, 128};
  Primitive p;
  p.type = Type::kGaussianBlur;
  p.input = "SourceAlpha";
  p.result = "blur";
  p.subregion = m.region;
  p.std_deviation_x = sigma;
  p.std_deviation_y = sigma;
  m.primitives.push_back(p);
  return m;
}
TEST(SkityFilterGraph, RejectsHugeBlurWorkspace) {
  EXPECT_NE(Graph::Compile(BlurModel(2), skity::Matrix()), nullptr);
  EXPECT_EQ(Graph::Compile(BlurModel(100000), skity::Matrix()), nullptr);
  EXPECT_EQ(Graph::Compile(BlurModel(std::numeric_limits<float>::infinity()),
                           skity::Matrix()),
            nullptr);
}
TEST(SkityFilterGraph, BudgetsBasisLengthsInsteadOfMappedBoundingBox) {
  auto model = BlurModel(0);
  model.region = {0, 0, 1024, 1024};
  model.primitives.front().subregion = model.region;
  skity::Matrix nearly_parallel;
  nearly_parallel.SetScaleX(4);
  nearly_parallel.SetSkewX(4);
  nearly_parallel.SetScaleY(.1f);
  EXPECT_EQ(Graph::Compile(model, nearly_parallel), nullptr);
}
TEST(SkityFilterGraph, SurfaceContentScaleParticipatesInAdmission) {
  const auto one = Graph::Compile(BlurModel(0), skity::Matrix(), 1.f);
  const auto two = Graph::Compile(BlurModel(0), skity::Matrix(), 2.f);
  ASSERT_NE(one, nullptr);
  ASSERT_NE(two, nullptr);
  EXPECT_DOUBLE_EQ(two->cost().workspace_bytes,
                   one->cost().workspace_bytes * 4);
  auto model = BlurModel(0);
  model.region = {0, 0, 1024, 1024};
  model.primitives.front().subregion = model.region;
  EXPECT_NE(Graph::Compile(model, skity::Matrix(), 1.f), nullptr);
  EXPECT_EQ(Graph::Compile(model, skity::Matrix(), 2.f), nullptr);
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeWH(1024, 1024));
  serval::svg::skity::SrSkityCanvas retina(recorder.GetRecordingCanvas(), {},
                                           2.f);
  EXPECT_FALSE(retina.SupportsFilterModel(model));
  EXPECT_EQ(Graph::Compile(model, skity::Matrix(), 0.f), nullptr);
  EXPECT_EQ(Graph::Compile(model, skity::Matrix(),
                           std::numeric_limits<float>::infinity()),
            nullptr);
  skity::Matrix perspective;
  perspective.SetPersp0(.01f);
  EXPECT_EQ(Graph::Compile(model, perspective), nullptr);
}
TEST(SkityFilterGraph, LinearBlurAlsoHonorsWorkspaceLimit) {
  skity::Bitmap bitmap(128, 128, skity::AlphaType::kPremul_AlphaType);
  auto canvas = skity::Canvas::MakeSoftwareCanvas(&bitmap);
  serval::svg::skity::SrSkityCanvas adapter(canvas.get(), {});
  auto model = BlurModel(100000);
  model.primitives[0].input = "SourceGraphic";
  EXPECT_FALSE(adapter.SupportsFilterModel(model));
}
TEST(SkityFilterGraph, PrimitiveSubregionClipsFlood) {
  auto image = Render(
      Svg("<feFlood flood-color='red' x='45' y='45' width='10' height='10'/>"));
  EXPECT_EQ(Pixel(image, 50, 50), (std::vector<uint8_t>{255, 0, 0, 255}));
  EXPECT_EQ(Pixel(image, 40, 40), (std::vector<uint8_t>{0, 0, 0, 0}));
}
TEST(SkityFilterGraph, RejectsUnknownInputsAndUnboundedReplay) {
  auto m = BlurModel(1);
  m.primitives[0].input = "unknown";
  EXPECT_EQ(Graph::Compile(m, skity::Matrix()), nullptr);
  m = BlurModel(1);
  for (int i = 0; i < 20; ++i) {
    Primitive p;
    p.type = Type::kBlend;
    p.blend_mode = "normal";
    p.input = m.primitives.back().result;
    p.input2 = p.input;
    p.result = "r" + std::to_string(i);
    p.subregion = m.region;
    m.primitives.push_back(p);
  }
  EXPECT_EQ(Graph::Compile(m, skity::Matrix()), nullptr);
}
TEST(SkityFilterGraph, RejectsUnsupportedColoredArithmetic) {
  Model m;
  m.region = {0, 0, 128, 128};
  Primitive p;
  p.type = Type::kComposite;
  p.input = "SourceGraphic";
  p.input2 = "SourceAlpha";
  p.result = "r";
  p.subregion = m.region;
  p.composite_operator = "arithmetic";
  p.k2 = -1;
  p.k3 = 1;
  m.primitives.push_back(p);
  EXPECT_EQ(Graph::Compile(m, skity::Matrix()), nullptr);
}

Model ReplayModel() {
  Model model;
  model.region = {0, 0, 128, 128};
  std::string input = "SourceGraphic";
  for (int i = 0; i < 3; ++i) {
    Primitive primitive;
    primitive.type = Type::kBlend;
    primitive.blend_mode = "normal";
    primitive.input = input;
    primitive.input2 = input;
    primitive.result = "replay" + std::to_string(i);
    primitive.subregion = model.region;
    model.primitives.push_back(primitive);
    input = primitive.result;
  }
  return model;
}

TEST(SkityFilterGraph, NestedReplayBudgetRejectsAmplificationAndRecovers) {
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeWH(128, 128));
  serval::svg::skity::SrSkityCanvas adapter(recorder.GetRecordingCanvas(), {});
  const auto model = ReplayModel();
  ASSERT_TRUE(adapter.SupportsFilterModel(model));
  adapter.BeginFilterLayer(&model.region, model);
  EXPECT_FALSE(adapter.SupportsFilterModel(model));
  adapter.EndFilterLayer();
  EXPECT_TRUE(adapter.SupportsFilterModel(model));
}

TEST(SkityFilterGraph, LinearLayersParticipateInNestedBudget) {
  auto model = BlurModel(1);
  model.primitives.clear();
  std::string input = "SourceGraphic";
  for (int i = 0; i < 20; ++i) {
    Primitive primitive;
    primitive.type = Type::kOffset;
    primitive.input = input;
    primitive.result = "offset" + std::to_string(i);
    primitive.subregion = model.region;
    model.primitives.push_back(primitive);
    input = primitive.result;
  }
  ASSERT_TRUE(
      serval::svg::canvas::SrSupportsLinearSourceGraphicFilterModel(model));
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeWH(128, 128));
  serval::svg::skity::SrSkityCanvas adapter(recorder.GetRecordingCanvas(), {});
  ASSERT_TRUE(adapter.SupportsFilterModel(model));
  adapter.BeginFilterLayer(&model.region, model);
  EXPECT_FALSE(adapter.SupportsFilterModel(model));
  adapter.EndFilterLayer();
  EXPECT_TRUE(adapter.SupportsFilterModel(model));
}

TEST(SkityFilterGraph, FloodOnlyParentDoesNotEraseCaptureBudget) {
  Model flood;
  flood.region = {0, 0, 128, 128};
  Primitive primitive;
  primitive.type = Type::kFlood;
  primitive.result = "paint";
  primitive.subregion = flood.region;
  flood.primitives.push_back(primitive);
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeWH(128, 128));
  serval::svg::skity::SrSkityCanvas adapter(recorder.GetRecordingCanvas(), {});
  ASSERT_TRUE(adapter.SupportsFilterModel(flood));
  adapter.BeginFilterLayer(&flood.region, flood);
  const auto replay = ReplayModel();
  ASSERT_TRUE(adapter.SupportsFilterModel(replay));
  adapter.BeginFilterLayer(&replay.region, replay);
  EXPECT_FALSE(adapter.SupportsFilterModel(replay));
  adapter.EndFilterLayer();
  adapter.EndFilterLayer();
  EXPECT_TRUE(adapter.SupportsFilterModel(replay));
}

TEST(SkityFilterGraph, CombinedCostsRejectWorkspaceAndIntegerOverflow) {
  Graph::Cost outer;
  Graph::Cost inner;
  Graph::Cost combined;
  outer.workspace_bytes = 32.0 * 1024 * 1024;
  inner.workspace_bytes = 33.0 * 1024 * 1024;
  EXPECT_FALSE(Graph::CombineCosts(outer, inner, &combined));
  outer = {};
  inner = {};
  inner.source_replays = std::numeric_limits<size_t>::max();
  EXPECT_FALSE(Graph::CombineCosts(outer, inner, &combined));
  inner = {};
  inner.visits = std::numeric_limits<size_t>::max();
  EXPECT_FALSE(Graph::CombineCosts(outer, inner, &combined));
}

TEST(SkityFilterGraph, NestedSvgRecordingDoesNotMultiplySourceReplays) {
  std::string body;
  for (int i = 0; i < 4; ++i)
    body += "<g filter='url(#f)'>";
  body += "<rect x='40' y='40' width='32' height='32' fill='blue'/>";
  for (int i = 0; i < 4; ++i)
    body += "</g>";
  const auto svg =
      Svg("<feBlend in='SourceGraphic' in2='SourceGraphic' result='a'/>"
          "<feBlend in='a' in2='a' result='b'/><feBlend in='b' in2='b'/>",
          body);
  auto dom =
      serval::svg::parser::SrSVGDOM::make(svg.data(), svg.size(), nullptr);
  ASSERT_NE(dom, nullptr);
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeWH(128, 128));
  serval::svg::skity::SrSkityCanvas adapter(recorder.GetRecordingCanvas(), {});
  dom->Render(&adapter, {0, 0, 128, 128});
  auto recorded = recorder.FinishRecording();
  ASSERT_NE(recorded, nullptr);
  // The previous per-filter limit admitted this 4-layer input and recorded
  // 110,577 commands. Excessive inner filters now use the source fallback.
  EXPECT_LT(recorded->OpCount(), 4096u);
}
}  // namespace
int main(int argc, char** argv) {
  if (argc == 3 && argv[1][0] != '-') {
    std::ifstream file(argv[1]);
    std::string svg((std::istreambuf_iterator<char>(file)), {});
    auto pixels = Render(svg);
    if (pixels.empty())
      return 2;
    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    return out ? 0 : 3;
  }
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
