// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "canvas/SrParagraph.h"
#include "element/SrSVGCircle.h"
#include "element/SrSVGG.h"
#include "element/SrSVGMarker.h"
#include "element/SrSVGPath.h"
#include "element/SrSVGPattern.h"
#include "element/SrSVGRect.h"
#include "element/SrSVGUse.h"
#include "gtest/gtest.h"
#include "parser/SrSVGDOM.h"
#include "parser/SrSVGTraversalState.h"

// These tests exercise shared vector rendering only.
namespace serval::svg::canvas {
std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas*) {
  return nullptr;
}
}  // namespace serval::svg::canvas

namespace serval::svg {
namespace {
using element::SrMarkerPositions;

std::vector<element::SrSVGMarkerPosition> Positions(const char* data) {
  auto* path = make_serval_path(data, nullptr);
  auto positions = SrMarkerPositions(*path);
  release_serval_path(path);
  return positions;
}
void ExpectAngle(float actual, float expected) {
  EXPECT_NEAR(std::remainder(actual - expected, 360.f), 0.f, 0.001f);
}

class TestPath : public canvas::Path {
 public:
  explicit TestPath(SrSVGBox bounds = {0, 0, 100, 100}) : bounds_(bounds) {}
  SrSVGBox GetBounds() const override { return bounds_; }
  void Transform(const float (&)[6]) override {}
  std::unique_ptr<canvas::Path> CreateTransformCopy(
      const float (&)[6]) const override {
    return std::make_unique<TestPath>(bounds_);
  }
  void AddPath(canvas::Path*) override {}
  void SetFillType(SrSVGFillRule) override {}

 private:
  SrSVGBox bounds_;
};
class TestPathFactory : public canvas::PathFactory {
 public:
  std::unique_ptr<canvas::Path> CreateCircle(float, float, float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateRect(float x, float y, float, float,
                                           float width, float height) override {
    return std::make_unique<TestPath>(SrSVGBox{x, y, width, height});
  }
  std::unique_ptr<canvas::Path> CreateLine(float, float, float,
                                           float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateEllipse(float, float, float,
                                              float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreatePolygon(float*, uint32_t) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreatePolyline(float*, uint32_t) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateMutable() override {
    return std::make_unique<TestPath>();
  }
  std::unique_ptr<canvas::Path> CreatePath(uint8_t*, uint64_t, float*,
                                           uint64_t) override {
    return CreateMutable();
  }
  void Op(canvas::Path*, canvas::Path*, canvas::OP) override {}
  std::unique_ptr<canvas::Path> CreateStrokePath(const canvas::Path*, float,
                                                 SrSVGStrokeCap,
                                                 SrSVGStrokeJoin,
                                                 float) override {
    return CreateMutable();
  }
};
// Implements only the backend operations; all primitives use shared defaults.
class DefaultPathFactory : public canvas::PathFactory {
 public:
  std::vector<uint8_t> ops;
  std::vector<float> args;
  size_t calls{0};
  std::unique_ptr<canvas::Path> CreateMutable() override {
    return std::make_unique<TestPath>();
  }
  std::unique_ptr<canvas::Path> CreatePath(uint8_t* commands, uint64_t count,
                                           float* values,
                                           uint64_t value_count) override {
    ++calls;
    ops.assign(commands, commands + count);
    args.assign(values, values + value_count);
    return CreateMutable();
  }
  void Op(canvas::Path*, canvas::Path*, canvas::OP) override {}
  std::unique_ptr<canvas::Path> CreateStrokePath(const canvas::Path*, float,
                                                 SrSVGStrokeCap,
                                                 SrSVGStrokeJoin,
                                                 float) override {
    return CreateMutable();
  }
  std::vector<element::SrSVGMarkerPosition> Positions() {
    SrPathData data{};
    data.ops = ops.data();
    data.n_ops = ops.size();
    data.args = args.data();
    data.n_args = args.size();
    return SrMarkerPositions(data);
  }
};

TEST(PathFactoryDefaults, LineAndPolylinePreserveVertices) {
  DefaultPathFactory factory;
  ASSERT_NE(factory.CreateLine(2, 3, 8, 9), nullptr);
  EXPECT_EQ(factory.ops, (std::vector<uint8_t>{SPO_MOVE_TO, SPO_LINE_TO}));
  EXPECT_EQ(factory.args, (std::vector<float>{2, 3, 8, 9}));
  float points[]{0, 0, 10, 0, 10, 10};
  ASSERT_NE(factory.CreatePolyline(points, 3), nullptr);
  EXPECT_EQ(factory.calls, 2u);
  auto positions = factory.Positions();
  ASSERT_EQ(positions.size(), 3u);
  EXPECT_FLOAT_EQ(positions[1].x, 10.f);
  EXPECT_FLOAT_EQ(positions[1].y, 0.f);
  ExpectAngle(positions[0].angle, 0);
  ExpectAngle(positions[1].angle, 45);
  ExpectAngle(positions[2].angle, 90);
}

TEST(PathFactoryDefaults, PolygonClosesAndKeepsSinglePoint) {
  DefaultPathFactory factory;
  float points[]{0, 0, 10, 0, 10, 10};
  ASSERT_NE(factory.CreatePolygon(points, 3), nullptr);
  EXPECT_EQ(factory.ops, (std::vector<uint8_t>{SPO_MOVE_TO, SPO_LINE_TO,
                                               SPO_LINE_TO, SPO_CLOSE}));
  auto positions = factory.Positions();
  ASSERT_EQ(positions.size(), 4u);
  EXPECT_FLOAT_EQ(positions.back().x, 0.f);
  EXPECT_FLOAT_EQ(positions.back().y, 0.f);
  ASSERT_NE(factory.CreatePolyline(points, 1), nullptr);
  EXPECT_EQ(factory.ops, (std::vector<uint8_t>{SPO_MOVE_TO}));
  ASSERT_EQ(factory.Positions().size(), 1u);
  ASSERT_NE(factory.CreatePolygon(points, 1), nullptr);
  EXPECT_EQ(factory.ops, (std::vector<uint8_t>{SPO_MOVE_TO, SPO_CLOSE}));
}

TEST(PathFactoryDefaults, EllipseAndCircleKeepQuarterArcTangents) {
  DefaultPathFactory factory;
  ASSERT_NE(factory.CreateEllipse(10, 20, 4, 2), nullptr);
  EXPECT_EQ(factory.ops,
            (std::vector<uint8_t>{SPO_MOVE_TO, SPO_ELLIPTICAL_ARC,
                                  SPO_ELLIPTICAL_ARC, SPO_ELLIPTICAL_ARC,
                                  SPO_ELLIPTICAL_ARC, SPO_CLOSE}));
  EXPECT_EQ(factory.args, (std::vector<float>{
                              14, 20, 14, 20, 4,  2, 0,  0, 1,  10, 22, 10, 22,
                              4,  2,  0,  0,  1,  6, 20, 6, 20, 4,  2,  0,  0,
                              1,  10, 18, 10, 18, 4, 2,  0, 0,  1,  14, 20}));
  auto positions = factory.Positions();
  // Close retains a separate vertex even when the last arc ends at the start.
  ASSERT_EQ(positions.size(), 6u);
  for (size_t i = 0; i < 5; ++i) {
    ExpectAngle(positions[i].angle, 90.f + 90.f * i);
  }
  ExpectAngle(positions.back().angle, 90);
  ASSERT_NE(factory.CreateCircle(10, 20, 4), nullptr);
  const auto circle_args = factory.args;
  ASSERT_NE(factory.CreateEllipse(10, 20, 4, 4), nullptr);
  EXPECT_EQ(factory.args, circle_args);
}

TEST(PathFactoryDefaults, RectPreservesCornersAndClampsRadii) {
  DefaultPathFactory factory;
  ASSERT_NE(factory.CreateRect(1, 2, 0, 0, 8, 6), nullptr);
  EXPECT_EQ(factory.ops,
            (std::vector<uint8_t>{SPO_MOVE_TO, SPO_LINE_TO, SPO_LINE_TO,
                                  SPO_LINE_TO, SPO_CLOSE}));
  EXPECT_EQ(factory.args, (std::vector<float>{1, 2, 9, 2, 9, 8, 1, 8}));
  ASSERT_NE(factory.CreateRect(1, 2, 100, 100, 8, 6), nullptr);
  const auto clamped_args = factory.args;
  ASSERT_NE(factory.CreateRect(1, 2, 4, 3, 8, 6), nullptr);
  EXPECT_EQ(factory.args, clamped_args);
  EXPECT_EQ(factory.ops,
            (std::vector<uint8_t>{SPO_MOVE_TO, SPO_LINE_TO, SPO_ELLIPTICAL_ARC,
                                  SPO_LINE_TO, SPO_ELLIPTICAL_ARC, SPO_LINE_TO,
                                  SPO_ELLIPTICAL_ARC, SPO_LINE_TO,
                                  SPO_ELLIPTICAL_ARC, SPO_CLOSE}));
  ASSERT_EQ(factory.Positions().size(), 10u);
  ASSERT_NE(factory.CreateRect(1, 2, -1, 3, 8, 6), nullptr);
  EXPECT_EQ(factory.args, (std::vector<float>{1, 2, 9, 2, 9, 8, 1, 8}));
}

TEST(PathFactoryDefaults, EmptyShapesDoNotCallBackend) {
  DefaultPathFactory factory;
  float points[]{1, 2};
  EXPECT_EQ(factory.CreatePolyline(nullptr, 1), nullptr);
  EXPECT_EQ(factory.CreatePolygon(nullptr, 1), nullptr);
  EXPECT_EQ(factory.CreatePolyline(points, 0), nullptr);
  EXPECT_EQ(factory.CreatePolygon(points, 0), nullptr);
  EXPECT_EQ(factory.CreateCircle(0, 0, 0), nullptr);
  EXPECT_EQ(factory.CreateEllipse(0, 0, -1, 2), nullptr);
  EXPECT_EQ(factory.CreateEllipse(0, 0, 2, 0), nullptr);
  EXPECT_EQ(factory.CreateRect(0, 0, 1, 1, 0, 10), nullptr);
  EXPECT_EQ(factory.CreateRect(0, 0, 1, 1, 10, -1), nullptr);
  EXPECT_EQ(factory.calls, 0u);
}

struct Draw {
  std::string id;
  std::array<float, 6> matrix;
  SrSVGPaintType fill_type;
  uint32_t color;
  float opacity;
  SrSVGPaintType stroke_type;
  uint32_t stroke_color;
  float stroke_width;
  float stroke_opacity;
  SrSVGStrokeCap stroke_cap;
  SrSVGStrokeJoin stroke_join;
  float stroke_miter_limit;
  float stroke_dash_offset;
  std::vector<float> stroke_dash_array;
};
class RecordingCanvas : public canvas::SrCanvas {
 public:
  std::vector<Draw> draws;
  std::vector<SrSVGBox> clips;
  std::vector<float> opacity_layers;
  std::vector<float> image_opacities;
  std::array<float, 6> matrix{1, 0, 0, 1, 0, 0};
  std::vector<std::array<float, 6>> stack;
  TestPathFactory factory;
  void Record(const char* id, const SrSVGRenderState& state) {
    auto type = state.fill ? state.fill->type : SERVAL_PAINT_COLOR;
    uint32_t color = state.fill && type == SERVAL_PAINT_COLOR
                         ? state.fill->content.color.color
                         : NSVG_RGBA(0, 0, 0, 255);
    const auto* stroke = state.stroke_state;
    std::vector<float> dash_array;
    if (stroke && stroke->dash_array && stroke->dash_array_length) {
      dash_array.assign(stroke->dash_array,
                        stroke->dash_array + stroke->dash_array_length);
    }
    draws.push_back(
        {id, matrix, type, color, state.fill_opacity,
         state.stroke ? state.stroke->type : SERVAL_PAINT_NONE,
         state.stroke && state.stroke->type == SERVAL_PAINT_COLOR
             ? state.stroke->content.color.color
             : 0,
         state.stroke_width, state.stroke_opacity,
         stroke ? stroke->stroke_line_cap : SR_SVG_STROKE_CAP_BUTT,
         stroke ? stroke->stroke_line_join : SR_SVG_STROKE_JOIN_MITER,
         stroke ? stroke->stroke_miter_limit : 4.f,
         stroke ? stroke->stroke_dash_offset : 0.f, std::move(dash_array)});
  }
  void SetViewBox(float, float, float, float) override {}
  void DrawRect(const char* id, float, float, float, float, float, float,
                const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawCircle(const char* id, float, float, float,
                  const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawPolygon(const char* id, float*, uint32_t,
                   const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawPolyline(const char* id, float*, uint32_t,
                    const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawLine(const char* id, float, float, float, float,
                const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawPath(const char* id, uint8_t*, uint32_t, float*, uint32_t,
                const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void DrawEllipse(const char* id, float, float, float, float,
                   const SrSVGRenderState& state) override {
    Record(id, state);
  }
  void UpdateLinearGradient(const char*, const float (&)[6], GradientSpread,
                            float, float, float, float,
                            const std::vector<SrStop>&,
                            SrSVGObjectBoundingBoxUnitType) override {}
  void UpdateRadialGradient(const char*, const float (&)[6], GradientSpread,
                            float, float, float, float, float,
                            const std::vector<SrStop>&,
                            SrSVGObjectBoundingBoxUnitType) override {}
  void DrawUse(const char*, float, float, float, float) override {}
  void DrawImage(const char*, float, float, float, float,
                 const SrSVGPreserveAspectRatio&, float opacity) override {
    image_opacities.push_back(opacity);
  }
  void Translate(float x, float y) override {
    float transform[6]{1, 0, 0, 1, x, y};
    Transform(transform);
  }
  void Transform(const float (&form)[6]) override {
    xform_multiply(matrix.data(), form);
  }
  bool GetTransform(float (&form)[6]) const override {
    std::copy(matrix.begin(), matrix.end(), form);
    return true;
  }
  void ClipPath(canvas::Path* path, SrSVGFillRule) override {
    clips.push_back(path->GetBounds());
  }
  void Save() override { stack.push_back(matrix); }
  void Restore() override {
    ASSERT_FALSE(stack.empty());
    matrix = stack.back();
    stack.pop_back();
  }
  void BeginOpacityLayer(const SrSVGBox*, float opacity) override {
    opacity_layers.push_back(opacity);
    Save();
  }
  void EndOpacityLayer() override { Restore(); }
  canvas::PathFactory* PathFactory() override { return &factory; }
};

std::unique_ptr<parser::SrSVGDOM> Parse(const std::string& content) {
  std::string svg = "<svg viewBox=\"0 0 200 100\">" + content + "</svg>";
  return parser::SrSVGDOM::make(svg.c_str(), svg.size(), nullptr);
}
const char* kMarker =
    "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\" "
    "orient=\"auto\"><path id=\"tip\" d=\"M0 0L3 1L0 2Z\"/></marker>";

TEST(MarkerGeometry, PolylineBisectorAndZeroSegments) {
  auto p = Positions("M10 20L30 20L30 20L30 40");
  ASSERT_EQ(p.size(), 4u);
  ExpectAngle(p[0].angle, 0);
  ExpectAngle(p[1].angle, 45);
  ExpectAngle(p[2].angle, 45);
  ExpectAngle(p[3].angle, 90);
  p = Positions("M1 2L1 2L1 2");
  ASSERT_EQ(p.size(), 3u);
  for (auto vertex : p) {
    ExpectAngle(vertex.angle, 0);
  }
}
TEST(MarkerGeometry, CubicAndQuadraticTangents) {
  auto p = Positions("M0 0 C0 0 0 10 10 10 Q20 10 20 20");
  ASSERT_EQ(p.size(), 3u);
  ExpectAngle(p[0].angle, 90);
  ExpectAngle(p[1].angle, 0);
  ExpectAngle(p[2].angle, 90);
  p = Positions("M0 0 C0 0 10 10 10 10");
  ExpectAngle(p[0].angle, 45);
  ExpectAngle(p[1].angle, 45);
}
TEST(MarkerGeometry, LongZeroLengthSubpathBorrowsDirectionsInLinearTime) {
  std::string data = "M0 0L10 0";
  for (size_t i = 0; i < 10000; ++i) {
    data += "L10 0";
  }
  data += "L10 10";
  auto p = Positions(data.c_str());
  ASSERT_EQ(p.size(), 10003u);
  ExpectAngle(p[5000].angle, 45);
}
TEST(MarkerGeometry, EllipticalArcTangentsAndDegeneracy) {
  auto p = Positions("M10 0 A10 10 0 0 1 0 10");
  ASSERT_EQ(p.size(), 2u);
  ExpectAngle(p[0].angle, 90);
  ExpectAngle(p[1].angle, 180);
  p = Positions("M10 0 A0 10 0 0 1 0 10");
  ExpectAngle(p[0].angle, 135);
  p = Positions("M10 0 A10 10 0 0 1 10 0 L20 0");
  ASSERT_EQ(p.size(), 2u);
}
TEST(MarkerGeometry, ClosedSubpathsRetainSeparateStartAndEndVertices) {
  auto p = Positions("M10 10h10v10z m20 0h10v10z m20 0h10v10z");
  ASSERT_EQ(p.size(), 12u);
  EXPECT_EQ(p[0].x, p[3].x);
  EXPECT_EQ(p[4].x, p[7].x);
  EXPECT_EQ(p[8].x, p[11].x);
  ExpectAngle(p[0].angle, -67.5);
  ExpectAngle(p[3].angle, -67.5);
}
TEST(MarkerGeometry, OpenSubpathsDoNotBorrowAcrossMoveto) {
  auto p = Positions("M0 0L10 0 M20 20L20 30 M50 50");
  ASSERT_EQ(p.size(), 5u);
  ExpectAngle(p[1].angle, 0);
  ExpectAngle(p[2].angle, 90);
  ExpectAngle(p.back().angle, 0);
}
TEST(MarkerGeometry, RejectsNonFiniteAndTruncatedData) {
  float args[]{0, 0, std::numeric_limits<float>::infinity(), 1};
  uint8_t ops[]{SPO_MOVE_TO, SPO_LINE_TO};
  SrPathData path{};
  path.ops = ops;
  path.n_ops = 2;
  path.args = args;
  path.n_args = 4;
  EXPECT_TRUE(SrMarkerPositions(path).empty());
  path.n_args = 1;
  EXPECT_TRUE(SrMarkerPositions(path).empty());
}
TEST(MarkerGeometry, BasicShapesUseEquivalentPaths) {
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  auto rect = std::unique_ptr<element::SrSVGRect>(element::SrSVGRect::Make());
  rect->ParseAndSetAttribute("width", "40");
  rect->ParseAndSetAttribute("height", "20");
  auto p = SrMarkerPositions(*rect, context);
  ASSERT_EQ(p.size(), 5u);
  EXPECT_EQ(p.front().x, p.back().x);
  rect->ParseAndSetAttribute("rx", "5");
  p = SrMarkerPositions(*rect, context);
  ASSERT_GE(p.size(), 9u);
  EXPECT_EQ(p.front().x, 5);
}
TEST(MarkerGeometry, PathUsesCurrentDataWithoutApplyingShapeTransform) {
  auto shape = std::unique_ptr<element::SrSVGPath>(element::SrSVGPath::Make());
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  EXPECT_TRUE(SrMarkerPositions(*shape, context).empty());
  shape->ParseAndSetAttribute("d", "M10 20L30 20");
  shape->ParseAndSetAttribute("transform", "translate(40,50) scale(3)");
  auto positions = SrMarkerPositions(*shape, context);
  ASSERT_EQ(positions.size(), 2u);
  EXPECT_FLOAT_EQ(positions.front().x, 10);
  EXPECT_FLOAT_EQ(positions.back().x, 30);
  auto* animated = make_serval_path("M1 2L3 4", nullptr);
  ASSERT_TRUE(animated);
  ASSERT_TRUE(shape->SetAnimatedPathData(animated));
  release_serval_path(animated);
  positions = SrMarkerPositions(*shape, context);
  ASSERT_EQ(positions.size(), 2u);
  EXPECT_FLOAT_EQ(positions.front().x, 1);
  EXPECT_FLOAT_EQ(positions.back().y, 4);
  ExpectAngle(positions.back().angle, 45);
  shape->ParseAndSetAttribute("d", "");
  EXPECT_TRUE(SrMarkerPositions(*shape, context).empty());
}
TEST(MarkerRendering, DefinitionNeverRendersDirectly) {
  auto dom = Parse(kMarker);
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  EXPECT_TRUE(canvas.draws.empty());
}
TEST(MarkerRendering, MarkerDefinitionDoesNotPaintPatternTiles) {
  class PatternCanvas final : public RecordingCanvas {
   public:
    void SetRenderContext(const SrSVGRenderContext* context) override {
      ids = context ? static_cast<element::IDMapper*>(context->id_mapper)
                    : nullptr;
    }
    element::IDMapper* ids{nullptr};
  } canvas;
  auto dom =
      Parse(std::string("<pattern id=\"tile\" width=\"20\" height=\"20\">") +
            kMarker +
            "<rect id=\"tile-content\" width=\"10\" height=\"10\"/>"
            "</pattern><path d=\"M10 10L20 10\" marker-end=\"url(#m)\"/>");
  ASSERT_TRUE(dom);
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws.back().id, "tip");

  ASSERT_NE(canvas.ids, nullptr);
  auto tile = canvas.ids->find("tile");
  ASSERT_NE(tile, canvas.ids->end());
  ASSERT_EQ(tile->second->Tag(), element::SrSVGTag::kPattern);
  auto* pattern = static_cast<element::SrSVGPattern*>(tile->second);
  canvas.draws.clear();
  SrSVGRenderContext context{};
  context.id_mapper = canvas.ids;
  pattern->RenderContent(&canvas, context);
  ASSERT_EQ(canvas.draws.size(), 1u);
  EXPECT_EQ(canvas.draws[0].id, "tile-content");
}

TEST(MarkerRendering, UseDoesNotDrawMarkerDefinition) {
  auto dom = Parse(std::string("<defs>") + kMarker + "</defs>" +
                   "<use href=\"#m\" x=\"30\"/>"
                   "<path id=\"source\" d=\"M60 10L70 10\" "
                   "marker-end=\"url(#m)\"/>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws[0].id, "source");
  EXPECT_EQ(canvas.draws[1].id, "tip");
  EXPECT_FLOAT_EQ(canvas.draws[1].matrix[4], 70.f);
}

TEST(MarkerRendering, UseDoesNotExtractMarkerDefinitionGeometry) {
  auto rect = std::unique_ptr<element::SrSVGRect>(element::SrSVGRect::Make());
  ASSERT_TRUE(rect->ParseAndSetAttribute("width", "10"));
  ASSERT_TRUE(rect->ParseAndSetAttribute("height", "10"));
  auto marker =
      std::unique_ptr<element::SrSVGMarker>(element::SrSVGMarker::Make());
  marker->AppendChild(rect.get());
  auto use = std::unique_ptr<element::SrSVGUse>(element::SrSVGUse::Make());
  element::IDMapper ids{{"m", marker.get()}, {"rect", rect.get()}};
  parser::SrSVGTraversalState traversal;
  SrSVGRenderContext context{};
  context.id_mapper = &ids;
  context.traversal_state = &traversal;
  TestPathFactory factory;
  ASSERT_TRUE(use->ParseAndSetAttribute("href", "#m"));
  EXPECT_EQ(use->AsPath(&factory, &context), nullptr);
  ASSERT_TRUE(use->ParseAndSetAttribute("href", "#rect"));
  EXPECT_NE(use->AsPath(&factory, &context), nullptr);
}

TEST(MarkerRendering, InheritanceShorthandNoneAndInlineStyle) {
  auto dom =
      Parse(std::string(kMarker) +
            "<g marker=\"url('#m')\"><polyline id=\"a\" points=\"10,10 20,10 "
            "20,20\"/>"
            "<path id=\"b\" d=\"M40 10L50 10\" marker=\"none\"/>"
            "<path id=\"c\" d=\"M60 10L70 10\" style=\"marker-end: url(#m);"
            "marker-start:none;marker-mid:none\"/></g>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 7u);
  EXPECT_EQ(canvas.draws[0].id, "a");
  EXPECT_EQ(canvas.draws[4].id, "b");
  EXPECT_EQ(canvas.draws[5].id, "c");
  EXPECT_FLOAT_EQ(canvas.draws[6].matrix[4], 70);
  EXPECT_TRUE(canvas.stack.empty());
}
TEST(MarkerRendering, StartAndEndBelongToWholePath) {
  auto dom =
      Parse(std::string(kMarker) +
            "<path d=\"M10 10L20 10 M30 10L40 10\" marker=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 5u);
  for (size_t i = 1; i < canvas.draws.size(); ++i) {
    EXPECT_FLOAT_EQ(canvas.draws[i].matrix[4], i * 10);
  }
}
TEST(MarkerRendering, UseInheritsAllMarkerKindsThroughReferencedGroup) {
  auto dom =
      Parse(std::string(kMarker) +
            "<defs><g id=\"shape\"><path d=\"M10 10L20 10L20 20\"/></g></defs>"
            "<use href=\"#shape\" marker=\"url(#m)\"/>"
            "<g marker=\"url(#m)\"><use href=\"#shape\" x=\"40\"/></g>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 8u);
  EXPECT_EQ(canvas.draws[1].id, "tip");
  EXPECT_EQ(canvas.draws[2].id, "tip");
  EXPECT_EQ(canvas.draws[3].id, "tip");
  EXPECT_FLOAT_EQ(canvas.draws[5].matrix[4], 50);
  EXPECT_FLOAT_EQ(canvas.draws[6].matrix[4], 60);
  EXPECT_FLOAT_EQ(canvas.draws[7].matrix[5], 20);
  EXPECT_TRUE(canvas.stack.empty());
}
TEST(MarkerRendering, UseNoneAndReferencedDeclarationsTakePrecedence) {
  auto dom =
      Parse(std::string(kMarker) +
            "<defs><path id=\"plain\" d=\"M10 10L20 10\"/>"
            "<path id=\"disabled\" d=\"M10 10L20 10\" marker=\"none\"/>"
            "<path id=\"enabled\" d=\"M10 10L20 10\" marker-end=\"url(#m)\"/>"
            "</defs><g marker=\"url(#m)\">"
            "<use href=\"#plain\" marker=\"none\"/>"
            "<use href=\"#disabled\"/>"
            "<use href=\"#enabled\" marker=\"none\" x=\"40\"/></g>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 4u);
  EXPECT_EQ(canvas.draws[0].id, "plain");
  EXPECT_EQ(canvas.draws[1].id, "disabled");
  EXPECT_EQ(canvas.draws[2].id, "enabled");
  EXPECT_EQ(canvas.draws[3].id, "tip");
  EXPECT_FLOAT_EQ(canvas.draws[3].matrix[4], 60);
}
TEST(MarkerRendering, UseMarkerInstancesDoNotLeakAcrossSiblingsOrRenders) {
  auto dom = Parse(std::string(kMarker) +
                   "<defs><path id=\"shape\" d=\"M10 10L20 10\"/></defs>"
                   "<use href=\"#shape\" marker-end=\"url(#m)\"/>"
                   "<use href=\"#shape\"/>"
                   "<use href=\"#shape\" marker-end=\"url(#m)\" opacity=\"0\"/>"
                   "<use href=\"#shape\" x=\"40\"/>");
  ASSERT_TRUE(dom);
  for (int i = 0; i < 2; ++i) {
    RecordingCanvas canvas;
    dom->Render(&canvas);
    ASSERT_EQ(canvas.draws.size(), 4u);
    EXPECT_EQ(canvas.draws[1].id, "tip");
    EXPECT_EQ(canvas.draws[2].id, "shape");
    EXPECT_EQ(canvas.draws[3].id, "shape");
    EXPECT_TRUE(canvas.stack.empty());
  }
}
TEST(MarkerRendering, UseRestoresDefinitionMarkerInheritanceOnAllExits) {
  auto original = std::unique_ptr<element::SrSVGG>(element::SrSVGG::Make());
  original->ParseAndSetAttribute("fill", "blue");
  original->ParseAndSetAttribute("stroke", "red");
  original->ParseAndSetAttribute("clip-path", "url(#clip)");
  original->ParseAndSetAttribute("mask", "url(#mask)");
  original->ParseAndSetAttribute("color", "green");
  auto shape = std::unique_ptr<element::SrSVGPath>(element::SrSVGPath::Make());
  auto use = std::unique_ptr<element::SrSVGUse>(element::SrSVGUse::Make());
  ASSERT_TRUE(shape->ParseAndSetAttribute("d", "M10 10L20 10"));
  shape->inherit_fill_paint_ = original->fill_;
  shape->inherit_stroke_paint_ = original->stroke_;
  shape->inherit_clip_path_ = original->clip_path_;
  shape->inherit_mask_ = original->mask_;
  shape->inherit_color_ = original->color_;
  shape->inherit_stroke_width_ = SrSVGLength{9.f, SR_SVG_UNITS_PX};
  shape->inherit_fill_opacity_ = 0.25f;
  shape->inherit_stroke_opacity_ = 0.75f;
  shape->inherit_markers_ = {std::string("start"), std::string("mid"),
                             std::string("end")};
  const auto original_markers = shape->inherit_markers_;
  ASSERT_TRUE(use->ParseAndSetAttribute("href", "#shape"));
  use->ParseAndSetAttribute("fill", "green");
  use->ParseAndSetAttribute("stroke", "blue");
  use->ParseAndSetAttribute("clip-path", "url(#other-clip)");
  use->ParseAndSetAttribute("mask", "url(#other-mask)");
  use->ParseAndSetAttribute("color", "red");
  use->ParseAndSetAttribute("stroke-width", "3");
  use->ParseAndSetAttribute("fill-opacity", "0.5");
  use->ParseAndSetAttribute("stroke-opacity", "0.5");
  element::IDMapper ids{{"shape", shape.get()}};
  parser::SrSVGTraversalState traversal;
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  context.id_mapper = &ids;
  context.traversal_state = &traversal;
  for (const char* opacity : {"1", "0"}) {
    SCOPED_TRACE(opacity);
    use->ParseAndSetAttribute("opacity", opacity);
    use->ParseAndSetAttribute("marker", "none");
    ASSERT_EQ(use->markers_[0], std::optional<std::string>(""));
    RecordingCanvas canvas;
    use->Render(&canvas, context);
    EXPECT_EQ(shape->inherit_markers_, original_markers);
    EXPECT_EQ(shape->inherit_fill_paint_, original->fill_);
    EXPECT_EQ(shape->inherit_stroke_paint_, original->stroke_);
    EXPECT_EQ(shape->inherit_clip_path_, original->clip_path_);
    EXPECT_EQ(shape->inherit_mask_, original->mask_);
    ASSERT_TRUE(shape->inherit_color_);
    EXPECT_EQ(shape->inherit_color_->color, original->color_->color);
    ASSERT_TRUE(shape->inherit_stroke_width_);
    EXPECT_FLOAT_EQ(shape->inherit_stroke_width_->value, 9);
    EXPECT_EQ(shape->inherit_stroke_width_->unit, SR_SVG_UNITS_PX);
    EXPECT_EQ(shape->inherit_fill_opacity_, std::optional<float>(0.25f));
    EXPECT_EQ(shape->inherit_stroke_opacity_, std::optional<float>(0.75f));
    EXPECT_TRUE(canvas.stack.empty());
    EXPECT_EQ(traversal.use_depth, 0u);
  }
}
TEST(MarkerRendering, NestedUsePreservesPaintAndOpacityPrecedence) {
  auto dom = Parse(
      "<defs><path id=\"plain\" d=\"M10 10L20 10\"/>"
      "<path id=\"local\" d=\"M10 10L20 10\" fill=\"white\" stroke=\"blue\" "
      "stroke-width=\"2\" fill-opacity=\"0.75\" stroke-opacity=\"0.4\"/>"
      "<use id=\"alias\" href=\"#plain\"/></defs>"
      "<g fill=\"red\" stroke=\"green\" stroke-width=\"7\" color=\"blue\" "
      "fill-opacity=\"0.25\" stroke-opacity=\"0.5\">"
      "<use href=\"#alias\"/>"
      "<use href=\"#alias\" fill=\"currentColor\" stroke=\"none\" "
      "stroke-width=\"3\"/>"
      "<use href=\"#local\" fill=\"red\" stroke=\"green\" "
      "stroke-width=\"9\"/></g><use href=\"#alias\"/>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 4u);
  EXPECT_EQ(canvas.draws[0].color, NSVG_RGBA(255, 0, 0, 255));
  EXPECT_EQ(canvas.draws[0].stroke_color, NSVG_RGBA(0, 128, 0, 255));
  EXPECT_FLOAT_EQ(canvas.draws[0].stroke_width, 7);
  EXPECT_FLOAT_EQ(canvas.draws[0].opacity, 0.25f);
  EXPECT_FLOAT_EQ(canvas.draws[0].stroke_opacity, 0.5f);
  EXPECT_EQ(canvas.draws[1].color, NSVG_RGBA(0, 0, 255, 255));
  EXPECT_EQ(canvas.draws[1].stroke_type, SERVAL_PAINT_NONE);
  EXPECT_FLOAT_EQ(canvas.draws[1].stroke_width, 3);
  EXPECT_EQ(canvas.draws[2].color, NSVG_RGBA(255, 255, 255, 255));
  EXPECT_EQ(canvas.draws[2].stroke_color, NSVG_RGBA(0, 0, 255, 255));
  EXPECT_FLOAT_EQ(canvas.draws[2].stroke_width, 2);
  EXPECT_FLOAT_EQ(canvas.draws[2].opacity, 0.75f);
  EXPECT_FLOAT_EQ(canvas.draws[2].stroke_opacity, 0.4f);
  EXPECT_EQ(canvas.draws[3].color, NSVG_RGBA(0, 0, 0, 255));
  EXPECT_EQ(canvas.draws[3].stroke_type, SERVAL_PAINT_NONE);
  EXPECT_FLOAT_EQ(canvas.draws[3].stroke_width, 1);
  EXPECT_FLOAT_EQ(canvas.draws[3].opacity, 1);
  EXPECT_TRUE(canvas.stack.empty());
}
TEST(MarkerRendering, UseStrokeOverridesRestoreBetweenInstances) {
  auto dom = Parse(
      "<defs><path id=\"p\" d=\"M10 10L20 10\"/></defs>"
      "<use href=\"#p\" stroke-linecap=\"round\" stroke-linejoin=\"bevel\" "
      "stroke-miterlimit=\"8\" stroke-dashoffset=\"3\" stroke-dasharray=\"4 "
      "2\"/>"
      "<use href=\"#p\"/>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws[0].stroke_cap, SR_SVG_STROKE_CAP_ROUND);
  EXPECT_EQ(canvas.draws[0].stroke_join, SR_SVG_STROKE_JOIN_BEVEL);
  EXPECT_FLOAT_EQ(canvas.draws[0].stroke_miter_limit, 8);
  EXPECT_FLOAT_EQ(canvas.draws[0].stroke_dash_offset, 3);
  EXPECT_EQ(canvas.draws[0].stroke_dash_array, (std::vector<float>{4, 2}));
  EXPECT_EQ(canvas.draws[1].stroke_cap, SR_SVG_STROKE_CAP_BUTT);
  EXPECT_EQ(canvas.draws[1].stroke_join, SR_SVG_STROKE_JOIN_MITER);
  EXPECT_FLOAT_EQ(canvas.draws[1].stroke_miter_limit, 4);
  EXPECT_FLOAT_EQ(canvas.draws[1].stroke_dash_offset, 0);
  EXPECT_TRUE(canvas.draws[1].stroke_dash_array.empty());
}
TEST(MarkerRendering,
     ContainerChildPathRestoresInheritanceOnSuccessAndFailure) {
  auto root = std::unique_ptr<element::SrSVGG>(element::SrSVGG::Make());
  auto child = std::unique_ptr<element::SrSVGG>(element::SrSVGG::Make());
  auto fallback = std::unique_ptr<element::SrSVGG>(element::SrSVGG::Make());
  auto path = std::unique_ptr<element::SrSVGPath>(element::SrSVGPath::Make());
  root->ParseAndSetAttribute("fill", "red");
  fallback->ParseAndSetAttribute("fill", "blue");
  child->inherit_fill_paint_ = fallback->fill_;
  path->ParseAndSetAttribute("d", "M10 10L20 10");
  root->AppendChild(child.get());
  child->AppendChild(path.get());
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  RecordingCanvas canvas;
  EXPECT_FALSE(root->RenderChildPathAt(&canvas, context, {0, 0, 0}));
  EXPECT_EQ(child->inherit_fill_paint_, fallback->fill_);
  EXPECT_EQ(path->inherit_fill_paint_, nullptr);
  EXPECT_TRUE(canvas.draws.empty());
  EXPECT_TRUE(root->RenderChildPathAt(&canvas, context, {0, 0}));
  EXPECT_EQ(child->inherit_fill_paint_, fallback->fill_);
  EXPECT_EQ(path->inherit_fill_paint_, nullptr);
  child->Render(&canvas, context);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws[0].color, NSVG_RGBA(255, 0, 0, 255));
  EXPECT_EQ(canvas.draws[1].color, NSVG_RGBA(0, 0, 255, 255));
}
TEST(MarkerRendering, MarkerKindsResolveIndependentlyForEachShape) {
  auto dom = Parse(
      "<marker id=\"red\" markerUnits=\"userSpaceOnUse\">"
      "<path id=\"red-tip\" fill=\"red\" d=\"M0 0L3 0\"/></marker>"
      "<marker id=\"blue\" markerUnits=\"userSpaceOnUse\">"
      "<path id=\"blue-tip\" fill=\"blue\" d=\"M0 0L3 0\"/></marker>"
      "<g marker=\"url(#red)\">"
      "<path id=\"a\" d=\"M10 10L20 10L30 10\" marker-mid=\"url(#blue)\"/>"
      "<path id=\"b\" d=\"M40 10L50 10L60 10\" marker-start=\"none\" "
      "marker-mid=\"url(#missing)\" marker-end=\"url(#blue)\"/></g>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 6u);
  EXPECT_EQ(canvas.draws[1].id, "red-tip");
  EXPECT_EQ(canvas.draws[2].id, "blue-tip");
  EXPECT_EQ(canvas.draws[3].id, "red-tip");
  EXPECT_EQ(canvas.draws[4].id, "b");
  EXPECT_EQ(canvas.draws[5].id, "blue-tip");
  EXPECT_FLOAT_EQ(canvas.draws[5].matrix[4], 60);
  EXPECT_TRUE(canvas.stack.empty());
}
TEST(MarkerRendering, UnresolvedMarkersPreserveWholeShapeOpacity) {
  auto dom = Parse(
      "<path id=\"wrong\" d=\"M0 0L1 0\"/>"
      "<path d=\"M10 10L20 10\" opacity=\"0.5\" marker-end=\"url(#missing)\"/>"
      "<path d=\"M30 10L40 10\" opacity=\"0.5\" marker-end=\"url(#wrong)\"/>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 3u);
  EXPECT_EQ(canvas.opacity_layers, (std::vector<float>{0.5f, 0.5f}));
  EXPECT_FLOAT_EQ(canvas.draws[1].opacity, 1);
  EXPECT_FLOAT_EQ(canvas.draws[2].opacity, 1);
  EXPECT_TRUE(canvas.stack.empty());
}
TEST(MarkerRendering, ContextStrokeDoesNotInheritOrdinaryFillOrStroke) {
  auto dom = Parse(
      "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\">"
      "<path id=\"context\" d=\"M0 0L3 0\" fill=\"context-stroke\"/>"
      "<path id=\"ordinary\" d=\"M0 0L3 0\"/></marker>"
      "<g fill=\"none\" stroke=\"red\" stroke-width=\"10\">"
      "<path d=\"M10 10L20 10\" marker-end=\"url(#m)\"/>"
      "<path d=\"M30 10L40 10\" stroke=\"blue\" marker-end=\"url(#m)\"/></g>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 6u);
  EXPECT_EQ(canvas.draws[1].color, NSVG_RGBA(255, 0, 0, 255));
  EXPECT_EQ(canvas.draws[2].color, NSVG_RGBA(0, 0, 0, 255));
  EXPECT_EQ(canvas.draws[4].color, NSVG_RGBA(0, 0, 255, 255));
  EXPECT_EQ(canvas.draws[5].color, NSVG_RGBA(0, 0, 0, 255));
}
TEST(MarkerRendering, ViewBoxReferencePointStrokeUnitsAndClipping) {
  auto dom = Parse(
      "<marker id=\"m\" viewBox=\"0 0 10 10\" markerWidth=\"4\" "
      "markerHeight=\"3\" refX=\"1\" refY=\"5\">"
      "<path d=\"M0 0L10 5L0 10Z\"/></marker>"
      "<path d=\"M10 20L30 20\" stroke-width=\"10\" marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  const auto& matrix = canvas.draws[1].matrix;
  EXPECT_NEAR(matrix[0], 3, 0.001f);
  EXPECT_NEAR(matrix[4] + matrix[0], 30, 0.001f);
  EXPECT_NEAR(matrix[5] + matrix[3] * 5, 20, 0.001f);
  ASSERT_EQ(canvas.clips.size(), 1u);
  EXPECT_FLOAT_EQ(canvas.clips[0].width, 4);
  EXPECT_FLOAT_EQ(canvas.clips[0].height, 3);
}
TEST(MarkerRendering, ReferenceKeywordsIncludeNonzeroViewBoxOrigin) {
  auto dom = Parse(
      "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" markerWidth=\"20\" "
      "markerHeight=\"10\" viewBox=\"10 20 20 10\" refX=\"right\" "
      "refY=\"center\"><path d=\"M10 20L30 25\"/></marker>"
      "<path d=\"M10 20L80 40\" marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_NEAR(canvas.draws[1].matrix[4] + 30, 80, 0.001);
  EXPECT_NEAR(canvas.draws[1].matrix[5] + 25, 40, 0.001);
}
TEST(MarkerRendering, PercentageAndKeywordReferencePointsAgree) {
  for (const char* ref : {"50%", "center"}) {
    SCOPED_TRACE(ref);
    auto dom = Parse(
        std::string(
            "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" "
            "markerWidth=\"40\" markerHeight=\"40\" viewBox=\"10 20 20 10\" "
            "refX=\"") +
        ref + "\" refY=\"" + ref +
        "\"><path d=\"M10 20L30 25\"/></marker>"
        "<path d=\"M10 20L80 40\" marker-end=\"url(#m)\"/>");
    ASSERT_TRUE(dom);
    RecordingCanvas canvas;
    dom->Render(&canvas);
    ASSERT_EQ(canvas.draws.size(), 2u);
    const auto& matrix = canvas.draws[1].matrix;
    EXPECT_NEAR(matrix[0] * 20 + matrix[4], 80, 0.001);
    EXPECT_NEAR(matrix[3] * 25 + matrix[5], 40, 0.001);
  }
}

TEST(MarkerRendering, ContextCurrentColorUsesReferencingElementColor) {
  auto dom = Parse(
      "<marker id=\"m\" color=\"blue\" markerUnits=\"userSpaceOnUse\">"
      "<path d=\"M0 0L3 0\" fill=\"context-stroke\"/></marker>"
      "<path d=\"M10 10L20 10\" color=\"red\" stroke=\"currentColor\" "
      "marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws[1].color, NSVG_RGBA(255, 0, 0, 255));
}
TEST(MarkerRendering, UnsupportedContextPaintServerDoesNotUseMarkerBounds) {
  auto dom = Parse(
      "<linearGradient id=\"g\"><stop stop-color=\"red\"/>"
      "<stop offset=\"1\" stop-color=\"blue\"/></linearGradient>"
      "<marker id=\"m\"><path d=\"M0 0L3 0\" fill=\"context-stroke\"/></marker>"
      "<path d=\"M10 10L20 10\" stroke=\"url(#g)\" marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_EQ(canvas.draws[1].fill_type, SERVAL_PAINT_NONE);
}
TEST(MarkerRendering, AutoStartReverseAndAngleUnits) {
  auto dom = Parse(
      "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\" "
      "orient=\"auto-start-reverse\"><path d=\"M0 0L3 0\"/></marker>"
      "<path d=\"M10 20L30 20\" marker=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 3u);
  EXPECT_NEAR(canvas.draws[1].matrix[0], -1, 0.001);
  EXPECT_NEAR(canvas.draws[2].matrix[0], 1, 0.001);
  auto marker =
      std::unique_ptr<element::SrSVGMarker>(element::SrSVGMarker::Make());
  EXPECT_TRUE(marker->ParseAndSetAttribute("orient", "0.25turn"));
  EXPECT_TRUE(marker->ParseAndSetAttribute("orient", "1.5rad"));
  EXPECT_TRUE(marker->ParseAndSetAttribute("orient", "100grad"));
  EXPECT_FALSE(marker->ParseAndSetAttribute("orient", "NaN"));
  EXPECT_FALSE(marker->ParseAndSetAttribute("orient", "45oops"));
}
TEST(MarkerRendering, InvalidReferencesAndEmptyViewportDrawNothing) {
  auto dom = Parse(
      "<marker id=\"zero\" markerWidth=\"0\"><path d=\"M0 0L3 0\"/></marker>"
      "<marker id=\"negative\" markerHeight=\"-3\"><path d=\"M0 0L3 "
      "0\"/></marker>"
      "<path id=\"wrong\" d=\"M0 0L3 0\"/>"
      "<path d=\"M10 20L30 20\" marker-start=\"url(#missing)\" "
      "marker-mid=\"url(#wrong)\" marker-end=\"url(#zero)\"/>"
      "<path d=\"M10 20L30 20\" marker-end=\"url(#negative)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  EXPECT_EQ(canvas.draws.size(), 3u);
}
TEST(MarkerRendering, AngleUnitsRenderFiniteExpectedRotations) {
  struct Case {
    const char* orient;
    double degrees;
  };
  constexpr double pi = 3.14159265358979323846;
  const Case cases[] = {
      {"1e308turn", 0},
      {"-1e308turn", 0},
      {"1e308rad", std::fmod(1e308, 2 * pi) * (180 / pi)},
      {"-1e308rad", std::fmod(-1e308, 2 * pi) * (180 / pi)},
      {"1e308grad", std::fmod(1e308, 400) * 0.9},
      {"1e308deg", std::fmod(1e308, 360)},
      {"1e308", std::fmod(1e308, 360)},
      {"-0.25turn", -90},
      {"-1.5707963267948966rad", -90},
      {"-100grad", -90},
      {"-450deg", -90},
  };
  for (const auto& test : cases) {
    SCOPED_TRACE(test.orient);
    auto dom = Parse(
        "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\" "
        "orient=\"" +
        std::string(test.orient) +
        "\"><path id=\"tip\" d=\"M0 0L3 0\"/></marker>"
        "<path d=\"M10 10L20 10\" marker-end=\"url(#m)\"/>");
    ASSERT_TRUE(dom);
    RecordingCanvas canvas;
    dom->Render(&canvas);
    ASSERT_EQ(canvas.draws.size(), 2u);
    const auto& matrix = canvas.draws[1].matrix;
    for (float value : matrix) {
      EXPECT_TRUE(std::isfinite(value));
    }
    EXPECT_NEAR(matrix[0], std::cos(test.degrees * pi / 180), 0.001);
    EXPECT_NEAR(matrix[1], std::sin(test.degrees * pi / 180), 0.001);
    EXPECT_FLOAT_EQ(matrix[4], 20);
    EXPECT_FLOAT_EQ(matrix[5], 10);
  }
}
TEST(MarkerRendering, InvalidOrientationPreservesPreviousRenderedValue) {
  auto marker =
      std::unique_ptr<element::SrSVGMarker>(element::SrSVGMarker::Make());
  auto tip = std::unique_ptr<element::SrSVGPath>(element::SrSVGPath::Make());
  auto shape = std::unique_ptr<element::SrSVGPath>(element::SrSVGPath::Make());
  ASSERT_TRUE(tip->ParseAndSetAttribute("d", "M0 0L3 0"));
  marker->AppendChild(tip.get());
  ASSERT_TRUE(marker->ParseAndSetAttribute("markerUnits", "userSpaceOnUse"));
  ASSERT_TRUE(shape->ParseAndSetAttribute("d", "M10 10L20 10"));
  shape->ParseAndSetAttribute("marker-start", "url(#m)");
  ASSERT_EQ(shape->markers_[0], std::optional<std::string>("m"));
  element::IDMapper ids{{"m", marker.get()}};
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  context.id_mapper = &ids;
  SrSVGRenderState state{};
  for (const char* valid : {"-100grad", "auto-start-reverse"}) {
    ASSERT_TRUE(marker->ParseAndSetAttribute("orient", valid));
    RecordingCanvas baseline;
    element::SrSVGMarker::RenderMarkers(
        &baseline, context, *shape, state,
        element::SrSVGMarker::ResolveMarkers(*shape, context));
    ASSERT_EQ(baseline.draws.size(), 1u);
    for (const char* invalid : {"NaN", "Infinity", "1e309turn", "45oops"}) {
      SCOPED_TRACE(invalid);
      EXPECT_FALSE(marker->ParseAndSetAttribute("orient", invalid));
      RecordingCanvas canvas;
      element::SrSVGMarker::RenderMarkers(
          &canvas, context, *shape, state,
          element::SrSVGMarker::ResolveMarkers(*shape, context));
      ASSERT_EQ(canvas.draws.size(), 1u);
      EXPECT_EQ(canvas.draws[0].matrix, baseline.draws[0].matrix);
    }
  }
}
TEST(MarkerRendering, RecursiveMarkerReferencesTerminate) {
  auto dom = Parse(
      "<marker id=\"a\"><path d=\"M0 0L1 0\" marker-end=\"url(#b)\"/></marker>"
      "<marker id=\"b\"><path d=\"M0 0L1 0\" marker-end=\"url(#a)\"/></marker>"
      "<path d=\"M10 20L30 20\" marker-end=\"url(#a)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  EXPECT_EQ(canvas.draws.size(), 3u);
  EXPECT_TRUE(canvas.stack.empty());
}

TEST(MarkerRendering, UseReentryKeepsContextPaintsAliveForAllMarkers) {
  for (const char* paint :
       {"fill=\"context-fill\"", "stroke=\"context-stroke\""}) {
    SCOPED_TRACE(paint);
    auto dom = Parse(
        std::string("<marker id=\"m\" overflow=\"visible\" "
                    "markerUnits=\"userSpaceOnUse\"><use href=\"#host\"/>"
                    "</marker><path id=\"host\" d=\"M10 10L20 10L30 10\" ") +
        paint +
        " marker-start=\"url(#m)\" marker-mid=\"url(#m)\" "
        "marker-end=\"url(#m)\"/>");
    ASSERT_TRUE(dom);
    for (int render = 0; render < 2; ++render) {
      RecordingCanvas canvas;
      dom->Render(&canvas);
      ASSERT_EQ(canvas.draws.size(), 4u);
      EXPECT_TRUE(canvas.stack.empty());
      for (size_t i = 1; i < canvas.draws.size(); ++i) {
        EXPECT_FLOAT_EQ(canvas.draws[i].matrix[4], 10.f * i);
        EXPECT_FLOAT_EQ(canvas.draws[i].matrix[5], 10.f);
        if (paint[0] == 'f') {
          EXPECT_EQ(canvas.draws[i].fill_type, SERVAL_PAINT_NONE);
        } else {
          EXPECT_EQ(canvas.draws[i].stroke_type, SERVAL_PAINT_NONE);
        }
      }
    }
  }
}

TEST(MarkerRendering,
     UseReentryDoesNotChangeContextCurrentColorBetweenMarkers) {
  auto dom = Parse(
      "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\" "
      "color=\"blue\"><use href=\"#host\"/>"
      "<path id=\"tip\" d=\"M0 0L3 1\" fill=\"context-fill\" "
      "stroke=\"context-stroke\"/></marker>"
      "<g color=\"red\"><path id=\"host\" d=\"M10 10L20 10\" "
      "fill=\"currentColor\" stroke=\"currentColor\" "
      "marker-start=\"url(#m)\" marker-end=\"url(#m)\"/></g>");
  ASSERT_TRUE(dom);
  for (int render = 0; render < 2; ++render) {
    RecordingCanvas canvas;
    dom->Render(&canvas);
    ASSERT_EQ(canvas.draws.size(), 5u);
    EXPECT_TRUE(canvas.stack.empty());
    EXPECT_EQ(canvas.draws[0].color, NSVG_RGBA(255, 0, 0, 255));
    for (size_t i : {1u, 3u}) {
      EXPECT_EQ(canvas.draws[i].color, NSVG_RGBA(0, 0, 255, 255));
    }
    for (size_t i : {2u, 4u}) {
      EXPECT_EQ(canvas.draws[i].id, "tip");
      EXPECT_EQ(canvas.draws[i].color, NSVG_RGBA(255, 0, 0, 255));
      EXPECT_EQ(canvas.draws[i].stroke_color, NSVG_RGBA(255, 0, 0, 255));
    }
  }
}

class ReentrantShape final : public element::SrSVGShape {
 public:
  ReentrantShape() : SrSVGShape(element::SrSVGTag::kRect) {}
  const SrSVGRenderState& state() const { return render_state_; }
  mutable size_t draws{0};

 protected:
  void onDraw(canvas::SrCanvas* canvas,
              SrSVGRenderContext& context) const override {
    if (++draws != 1) {
      return;
    }
    const SrSVGRenderState outer = render_state_;
    SrSVGPaint nested_fill{};
    nested_fill.type = SERVAL_PAINT_COLOR;
    nested_fill.content.color = {SERVAL_COLOR, NSVG_RGBA(0, 0, 255, 255)};
    SrSVGRenderContext nested_context = context;
    nested_context.context_fill = &nested_fill;
    const_cast<ReentrantShape*>(this)->Render(canvas, nested_context);
    EXPECT_EQ(render_state_.fill, outer.fill);
    EXPECT_EQ(render_state_.stroke_state, outer.stroke_state);
    ASSERT_NE(render_state_.fill, nullptr);
    EXPECT_EQ(render_state_.fill->content.color.color,
              NSVG_RGBA(255, 0, 0, 255));
  }
};

TEST(MarkerRendering, ShapeRestoresEnclosingAndInitialRenderState) {
  ReentrantShape shape;
  shape.ParseAndSetAttribute("fill", "context-fill");
  SrSVGPaint fill{};
  fill.type = SERVAL_PAINT_COLOR;
  fill.content.color = {SERVAL_COLOR, NSVG_RGBA(255, 0, 0, 255)};
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 200, 100};
  context.context_fill = &fill;
  RecordingCanvas canvas;
  shape.Render(&canvas, context);
  EXPECT_EQ(shape.draws, 2u);
  EXPECT_EQ(shape.state().fill, nullptr);
  EXPECT_EQ(shape.state().stroke, nullptr);
  EXPECT_EQ(shape.state().stroke_state, nullptr);
  EXPECT_TRUE(canvas.stack.empty());
}

TEST(MarkerRendering, NonScalingStrokeKeepsMarkerSizeAndPosition) {
  auto dom = Parse(
      "<marker id=\"m\" overflow=\"visible\"><path d=\"M0 0L1 0\"/></marker>"
      "<path transform=\"scale(3,2)\" d=\"M10 10L20 10\" stroke-width=\"4\" "
      "vector-effect=\"non-scaling-stroke\" marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_NEAR(canvas.draws[1].matrix[0], 4, 0.001);
  EXPECT_NEAR(canvas.draws[1].matrix[3], 4, 0.001);
  EXPECT_NEAR(canvas.draws[1].matrix[4], 60, 0.001);
  EXPECT_NEAR(canvas.draws[1].matrix[5], 20, 0.001);
}
TEST(MarkerRendering, ShapeOpacityCompositesStrokeAndMarkersTogether) {
  auto dom = Parse(
      std::string(kMarker) +
      "<path d=\"M10 10L20 10\" opacity=\"0.5\" marker-end=\"url(#m)\"/>");
  RecordingCanvas canvas;
  dom->Render(&canvas);
  ASSERT_EQ(canvas.opacity_layers.size(), 1u);
  EXPECT_FLOAT_EQ(canvas.opacity_layers[0], 0.5f);
  EXPECT_FLOAT_EQ(canvas.draws[0].opacity, 1.f);
  EXPECT_FLOAT_EQ(canvas.draws[1].opacity, 1.f);
}
TEST(MarkerRendering, LayerFailurePreservesOpacityForNestedMarkerContent) {
  class NoLayerCanvas final : public RecordingCanvas {
   public:
    bool TryBeginOpacityLayer(const SrSVGBox*, float) override { return false; }
  } canvas;
  auto dom = Parse(
      "<marker id=\"inner\">"
      "<path id=\"inner-tip\" d=\"M0 0L2 0L0 2Z\"/></marker>"
      "<marker id=\"outer\">"
      "<path id=\"outer-tip\" d=\"M0 0L2 0L0 2Z\" opacity=\"0.5\" "
      "fill-opacity=\"0.5\" marker-end=\"url(#inner)\"/>"
      "<image href=\"marker.png\" width=\"2\" height=\"2\"/></marker>"
      "<path id=\"source\" d=\"M10 10L20 10\" opacity=\"0.5\" "
      "marker-end=\"url(#outer)\"/>"
      "<path id=\"sibling\" d=\"M20 20L30 20\"/>");
  ASSERT_TRUE(dom);
  dom->Render(&canvas);
  ASSERT_EQ(canvas.draws.size(), 4u);
  EXPECT_EQ(canvas.draws[0].id, "source");
  EXPECT_FLOAT_EQ(canvas.draws[0].opacity, 0.5f);
  EXPECT_EQ(canvas.draws[1].id, "outer-tip");
  EXPECT_FLOAT_EQ(canvas.draws[1].opacity, 0.125f);
  EXPECT_EQ(canvas.draws[2].id, "inner-tip");
  EXPECT_FLOAT_EQ(canvas.draws[2].opacity, 0.25f);
  EXPECT_EQ(canvas.image_opacities, (std::vector<float>{0.5f}));
  EXPECT_EQ(canvas.draws[3].id, "sibling");
  EXPECT_FLOAT_EQ(canvas.draws[3].opacity, 1.f);
}

TEST(MarkerRendering, AnimatedMarkerAttributesRestoreBetweenRenders) {
  auto dom = Parse(
      "<marker id=\"m\" markerUnits=\"userSpaceOnUse\" overflow=\"visible\">"
      "<animate attributeName=\"markerWidth\" values=\"3;6\" dur=\"1s\" "
      "fill=\"freeze\"/><path d=\"M0 0L3 0\"/></marker>"
      "<path d=\"M10 10L20 10\"><set attributeName=\"marker-end\" "
      "to=\"url(#m)\" begin=\"1s\" dur=\"1s\"/></path>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->RenderAtTime(&canvas, 1.5);
  ASSERT_EQ(canvas.draws.size(), 2u);
  canvas.draws.clear();
  dom->RenderAtTime(&canvas, 0);
  EXPECT_EQ(canvas.draws.size(), 1u);
}
TEST(MarkerRendering, AnimatedLonghandRestoresInlineShorthand) {
  auto dom = Parse(
      std::string(kMarker) +
      "<path d=\"M10 10L20 10\" style=\"marker: url(#m)\">"
      "<set attributeName=\"marker-end\" to=\"none\" begin=\"1s\" dur=\"1s\"/>"
      "</path>");
  ASSERT_TRUE(dom);
  RecordingCanvas canvas;
  dom->RenderAtTime(&canvas, 0);
  ASSERT_EQ(canvas.draws.size(), 3u);
  canvas.draws.clear();
  dom->RenderAtTime(&canvas, 1.5);
  ASSERT_EQ(canvas.draws.size(), 2u);
  EXPECT_FLOAT_EQ(canvas.draws[1].matrix[4], 10.f);
  canvas.draws.clear();
  dom->RenderAtTime(&canvas, 2.5);
  ASSERT_EQ(canvas.draws.size(), 3u);
  EXPECT_FLOAT_EQ(canvas.draws[2].matrix[4], 20.f);
}

TEST(MarkerRendering, MultipleMarkerEffectsRestoreOriginalLonghands) {
  auto dom = Parse(
      std::string(kMarker) +
      "<path d=\"M10 10L20 10\" marker=\"url(#m)\" marker-end=\"none\">"
      "<set attributeName=\"marker\" to=\"url(#m)\" begin=\"1s\" dur=\"1s\"/>"
      "<set attributeName=\"marker-start\" to=\"none\" begin=\"1s\" "
      "dur=\"1s\"/>"
      "</path>");
  ASSERT_TRUE(dom);
  const double times[]{0, 1.5, 2.5};
  const float marker_x[]{10.f, 20.f, 10.f};
  for (size_t i = 0; i < 3; ++i) {
    SCOPED_TRACE(times[i]);
    RecordingCanvas canvas;
    dom->RenderAtTime(&canvas, times[i]);
    ASSERT_EQ(canvas.draws.size(), 2u);
    EXPECT_FLOAT_EQ(canvas.draws[1].matrix[4], marker_x[i]);
  }
}

TEST(MarkerRendering, DiscreteToAnimationUsesParsedAndInheritedMarkerBase) {
  const std::string animate =
      "<animate attributeName=\"marker-end\" to=\"none\" "
      "calcMode=\"discrete\" begin=\"1s\" dur=\"2s\"/>";
  auto dom = Parse(
      std::string(kMarker) +
      "<path d=\"M10 10L20 10\" marker-end=\"url(#m)\">" + animate + "</path>" +
      "<path d=\"M30 10L40 10\" "
      "style=\"marker:url(#m);marker-start:none;marker-mid:none\">" +
      animate + "</path><g marker-end=\"url(#m)\"><path d=\"M50 10L60 10\">" +
      animate + "</path></g><defs><path id=\"instance\" d=\"M70 10L80 10\">" +
      animate +
      "</path></defs><use href=\"#instance\" marker-end=\"url(#m)\"/>");
  ASSERT_TRUE(dom);
  for (double seconds : {0.5, 1.25, 2.25, 3.5}) {
    SCOPED_TRACE(seconds);
    RecordingCanvas canvas;
    dom->RenderAtTime(&canvas, seconds);
    std::vector<float> tips;
    for (const auto& draw : canvas.draws) {
      if (draw.id == "tip") {
        tips.push_back(draw.matrix[4]);
      }
    }
    const std::vector<float> active_tips{20.f, 40.f, 60.f, 80.f};
    EXPECT_EQ(tips, seconds == 2.25 ? std::vector<float>{} : active_tips);
  }
}

TEST(MarkerRendering, DiscreteShorthandAnimationUsesEachLonghandBase) {
  auto dom = Parse(
      std::string(kMarker) +
      "<path d=\"M10 10L20 10\" marker-start=\"url(#m)\" marker-end=\"none\">"
      "<animate attributeName=\"marker\" to=\"url(#m)\" calcMode=\"discrete\" "
      "begin=\"1s\" dur=\"2s\"/></path>");
  ASSERT_TRUE(dom);
  for (double seconds : {0.5, 1.25, 2.25, 3.5}) {
    SCOPED_TRACE(seconds);
    RecordingCanvas canvas;
    dom->RenderAtTime(&canvas, seconds);
    ASSERT_EQ(canvas.draws.size(), seconds == 2.25 ? 3u : 2u);
    EXPECT_FLOAT_EQ(canvas.draws[1].matrix[4], 10.f);
    if (seconds == 2.25) {
      EXPECT_FLOAT_EQ(canvas.draws[2].matrix[4], 20.f);
    }
  }
}

TEST(MarkerRendering, AnimatedPathMarkersFollowCurrentGeometry) {
  auto dom =
      Parse(std::string(kMarker) +
            "<path d=\"M10 10L20 10\" marker-end=\"url(#m)\">"
            "<animate attributeName=\"d\" values=\"M10 10L20 10;M10 10L80 10\" "
            "dur=\"1s\" fill=\"freeze\"/></path>");
  ASSERT_TRUE(dom);
  const double times[]{0.5, 1, 0};
  const float ends[]{50, 80, 20};
  for (size_t i = 0; i < 3; ++i) {
    RecordingCanvas canvas;
    dom->RenderAtTime(&canvas, times[i]);
    ASSERT_EQ(canvas.draws.size(), 2u);
    EXPECT_EQ(canvas.draws[1].id, "tip");
    EXPECT_FLOAT_EQ(canvas.draws[1].matrix[4], ends[i]);
    EXPECT_TRUE(canvas.stack.empty());
  }
}
}  // namespace
}  // namespace serval::svg
