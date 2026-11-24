// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <unordered_set>

#include "canvas/SrParagraph.h"
#include "element/SrSVGAnimation.h"
#include "element/SrSVGFilter.h"
#include "element/SrSVGFilterPrimitives.h"
#include "element/SrSVGG.h"
#include "element/SrSVGNode.h"
#include "element/SrSVGPath.h"
#include "element/SrSVGPolyLine.h"
#include "element/SrSVGPolygon.h"
#include "element/SrSVGRect.h"
#include "element/SrSVGSVG.h"
#include "element/SrSVGTypes.h"
#include "parser/SrDOM.h"
#include "parser/SrDOMParser.h"
#include "parser/SrSVGDOM.h"
#include "parser/SrSVGParse.h"
#include "parser/SrSVGParseLimits.h"
#include "parser/SrSVGTraversalState.h"
#include "utils/SrSVGLog.h"
#include "utils/SrSVGPatternUtils.h"

namespace serval {
namespace svg {
namespace parser {
// Original native construction entry point, also used by external callers.
element::SrSVGNodeBase* construct_svg_node(const SrDOM&,
                                           const element::SrSVGNodeBase*,
                                           const SrDOM::Node*,
                                           element::IDMapper*,
                                           std::list<element::SrSVGNodeBase*>&,
                                           const SrSVGDiagnosticSink*);
}  // namespace parser
}  // namespace svg
}  // namespace serval

namespace canvas = serval::svg::canvas;
namespace element = serval::svg::element;
namespace parser = serval::svg::parser;

namespace {
size_t allocation_count = 0;
size_t fail_at = 0;
bool fail_cpp = false;
size_t cpp_count = 0;
size_t cpp_fail_at = 0;
size_t cpp_live = 0;
std::unordered_set<void*> allocations;

bool FailAllocation() {
  return ++allocation_count == fail_at;
}

template <typename F>
void CheckAllocationFailures(F parse) {
  allocation_count = 0;
  fail_at = 0;
  assert(parse());
  const size_t count = allocation_count;
  assert(count > 1 && allocations.empty());
  for (size_t i = 1; i <= count; ++i) {
    allocation_count = 0;
    fail_at = i;
    assert(!parse());
    assert(allocations.empty());
  }
  fail_at = 0;
}

bool BuildXML(const std::string& text);

void TestCppAllocationFailures() {
  for (const std::string text :
       {"<svg id='x'><g><path d='M0 0 L1 1'/></g></svg>",
        "<svg><text>long text allocation during node construction</text>"
        "<path d='M0 0 L10'/><polygon points='0 0 10'/></svg>"}) {
    fail_cpp = true;
    cpp_count = 0;
    cpp_fail_at = 0;
    assert(BuildXML(text));
    fail_cpp = false;
    const size_t count = cpp_count;
    const size_t xml_live = cpp_live;
    for (size_t i = 1; i <= count; ++i) {
      cpp_count = 0;
      cpp_fail_at = i;
      fail_cpp = true;
      assert(!BuildXML(text));
      fail_cpp = false;
      assert(allocations.empty() && cpp_live == xml_live);
    }
    // Includes SVG node allocation, holder insertion, IDMapper, and DOM handoff.
    cpp_count = 0;
    cpp_fail_at = 0;
    fail_cpp = true;
    {
      std::vector<parser::SrSVGDiagnostic> diagnostics;
      auto dom = parser::SrSVGDOM::make(text.data(), text.size(), &diagnostics);
      assert(dom);
    }
    fail_cpp = false;
    const size_t svg_count = cpp_count;
    const size_t svg_live = cpp_live;
    for (size_t i = 1; i <= svg_count; ++i) {
      cpp_count = 0;
      cpp_fail_at = i;
      fail_cpp = true;
      {
        std::vector<parser::SrSVGDiagnostic> diagnostics;
        auto dom =
            parser::SrSVGDOM::make(text.data(), text.size(), &diagnostics);
        assert(!dom);
      }
      fail_cpp = false;
      assert(allocations.empty() && cpp_live == svg_live);
    }
  }
}

void TestThrowingDiagnostics() {
  const SrSVGDiagnosticSink sink{
      nullptr,
      [](void*, SrSVGDiagnosticCode, const char*, const char*, uint8_t) {
        throw std::bad_alloc();
      }};
  auto check = [&](element::SrSVGNodeBase* raw, const char* attribute,
                   const char* valid, const char* invalid) {
    std::unique_ptr<element::SrSVGNodeBase> node(raw);
    node->ParseAndSetAttribute(attribute, valid);
    node->SetDiagnosticSink(&sink);
    bool threw = false;
    try {
      node->ParseAndSetAttribute(attribute, invalid);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    assert(threw);
  };
  check(element::SrSVGPath::Make(), "d", "M0 0L1 1", "M0 0L10");
  check(element::SrSVGPolygon::Make(), "points", "0 0 1 1", "0 0 10");
  check(element::SrSVGPolyLine::Make(), "points", "0 0 1 1", "0 0 10");
  assert(allocations.empty());
}

void TestMalformedXMLOOM() {
  // Finish() and copying a long error noun allocate after parse() has returned.
  const std::string tag(128, 'g');
  for (const std::string& text : {"<" + tag + ">", "<svg></" + tag + ">"}) {
    allocation_count = 0;
    fail_cpp = true;
    cpp_count = 0;
    cpp_fail_at = 0;
    assert(!BuildXML(text));
    fail_cpp = false;
    const size_t count = cpp_count;
    const size_t live = cpp_live;
    for (size_t i = 1; i <= count; ++i) {
      cpp_count = 0;
      cpp_fail_at = i;
      fail_cpp = true;
      bool escaped = false;
      try {
        assert(!BuildXML(text));
      } catch (const std::bad_alloc&) {
        escaped = true;
      }
      fail_cpp = false;
      assert(!escaped && allocations.empty() && cpp_live == live);
    }
  }
}

void TestPaints() {
  auto* invalid = make_serval_paint("url() ignored");
  assert(invalid && invalid->type == SERVAL_PAINT_NONE);
  release_serval_paint(invalid);
  for (const char* value : {"none", "red", "url(#gradient)"}) {
    allocation_count = 0;
    fail_at = 0;
    auto* paint = make_serval_paint(value);
    assert(paint);
    const size_t count = allocation_count;
    release_serval_paint(paint);
    for (size_t i = 1; i <= count; ++i) {
      allocation_count = 0;
      fail_at = i;
      assert(!make_serval_paint(value));
      assert(allocations.empty());
    }
    fail_at = 0;
  }
}

// Exercise the real DOM-to-canvas path while checking inputs at its boundary.
struct TextRun {
  std::string text;
  SrTextStyle style;
};

class BoundaryCanvas final : public canvas::SrCanvas,
                             public canvas::PathFactory {
 public:
  class Path final : public canvas::Path {
   public:
    SrSVGBox GetBounds() const override { return {0, 0, 10, 10}; }
    void Transform(const float (&)[6]) override {}
    std::unique_ptr<canvas::Path> CreateTransformCopy(
        const float (&)[6]) const override {
      return std::make_unique<Path>();
    }
    void AddPath(canvas::Path*) override {}
    void SetFillType(SrSVGFillRule) override {}
  };
  canvas::PathFactory* PathFactory() override { return this; }
  std::unique_ptr<canvas::Path> CreateMutable() override {
    return std::make_unique<Path>();
  }
  std::unique_ptr<canvas::Path> CreateRect(float x, float y, float rx, float ry,
                                           float w, float h) override {
    path_rect = {x, y, rx, ry, w, h};
    ++rect_paths;
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateCircle(float, float, float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateEllipse(float, float, float,
                                              float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateLine(float, float, float,
                                           float) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreatePolygon(float*, uint32_t) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreatePolyline(float*, uint32_t) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreatePath(uint8_t*, uint64_t, float*,
                                           uint64_t) override {
    return CreateMutable();
  }
  std::unique_ptr<canvas::Path> CreateStrokePath(const canvas::Path*, float,
                                                 SrSVGStrokeCap,
                                                 SrSVGStrokeJoin,
                                                 float) override {
    return CreateMutable();
  }
  void Op(canvas::Path*, canvas::Path*, canvas::OP) override {}
  void Save() override { ++save_depth; }
  void Restore() override {
    assert(save_depth > 0);
    --save_depth;
  }
  void SetViewBox(float, float, float, float) override {}
  void Transform(const float (&matrix)[6]) override {
    for (float v : matrix)
      assert(std::isfinite(v));
  }
  void Translate(float x, float y) override {
    assert(std::isfinite(x) && std::isfinite(y));
  }
  void ClipPath(canvas::Path* path, SrSVGFillRule) override {
    assert(path);
    ++clips;
  }
  void DrawRect(const char*, float x, float y, float rx, float ry, float w,
                float h, const SrSVGRenderState& state) override {
    draw_rect = {x, y, rx, ry, w, h};
    last_opacity = state.opacity;
    last_dash_offset = state.stroke_state->stroke_dash_offset;
    ++shapes;
    last_fill = state.fill && state.fill->type == SERVAL_PAINT_COLOR
                    ? state.fill->content.color.color
                    : 0;
    last_rule = state.fill_rule;
  }
  void DrawCircle(const char*, float, float, float,
                  const SrSVGRenderState&) override {
    ++shapes;
  }
  void DrawEllipse(const char*, float, float, float, float,
                   const SrSVGRenderState&) override {
    ++shapes;
  }
  void DrawLine(const char*, float, float, float, float,
                const SrSVGRenderState&) override {}
  void DrawPolygon(const char*, float*, uint32_t,
                   const SrSVGRenderState&) override {}
  void DrawPolyline(const char*, float*, uint32_t,
                    const SrSVGRenderState&) override {}
  void DrawPath(const char*, uint8_t*, uint32_t, float*, uint32_t,
                const SrSVGRenderState&) override {}
  void DrawUse(const char*, float, float, float, float) override {}
  void DrawImage(const char*, float, float, float, float,
                 const SrSVGPreserveAspectRatio&, float) override {
    ++shapes;
  }
  void UpdateLinearGradient(const char*, const float (&)[6], GradientSpread,
                            float, float, float, float,
                            const std::vector<SrStop>&,
                            SrSVGObjectBoundingBoxUnitType) override {}
  void UpdateRadialGradient(const char*, const float (&)[6], GradientSpread,
                            float, float, float, float, float,
                            const std::vector<SrStop>&,
                            SrSVGObjectBoundingBoxUnitType) override {}
  void BeginMaskLayer(const SrSVGBox*, bool) override {
    ++masks;
    Save();
  }
  bool SupportsFilters() const override { return true; }
  bool SupportsFilterModel(const canvas::SrFilterModel& model) const override {
    return canvas::SrSupportsLinearSourceGraphicFilterModel(model);
  }
  bool TryBeginFilterLayer(const SrSVGBox*,
                           const canvas::SrFilterModel&) override {
    ++filter_attempts;
    if (fail_filter)
      return false;
    Save();
    return true;
  }
  void EndFilterLayer() override {
    ++filters_ended;
    Restore();
  }
  bool TryBeginMaskContentLayer() override {
    ++content_attempts;
    if (fail_mask_content)
      return false;
    Save();
    return true;
  }
  void EndMaskContentLayer() override {
    ++contents_ended;
    Restore();
  }
  bool GetOffscreenPixelCount(size_t* out) const override {
    if (!offscreen_pixels) {
      return false;
    }
    *out = *offscreen_pixels;
    return true;
  }
  bool TryBeginOpacityLayer(const SrSVGBox*, float) override {
    ++opacity_attempts;
    if (fail_opacity) {
      return false;
    }
    Save();
    return true;
  }
  void EndOpacityLayer() override {
    ++opacity_ends;
    Restore();
  }
  std::optional<size_t> offscreen_pixels;
  bool fail_opacity = false;
  unsigned opacity_attempts = 0, opacity_ends = 0;
  std::vector<TextRun> text_runs;
  std::vector<std::array<float, 2>> text_origins;
  int save_depth = 0, shapes = 0, clips = 0, masks = 0;
  int filter_attempts = 0, filters_ended = 0, content_attempts = 0,
      contents_ended = 0;
  bool fail_filter = false, fail_mask_content = false;
  int rect_paths = 0;
  float last_opacity = 0;
  float last_dash_offset = 0;
  std::array<float, 6> draw_rect{}, path_rect{};
  uint32_t last_fill = 0;
  SrSVGFillRule last_rule = SR_SVG_FILL;
};

class RecordingParagraph final : public canvas::Paragraph {
 public:
  explicit RecordingParagraph(std::vector<TextRun> runs)
      : runs_(std::move(runs)) {}
  void Layout(float) override {}
  void Draw(canvas::SrCanvas* canvas, float x, float y) override {
    auto& target = *static_cast<BoundaryCanvas*>(canvas);
    target.text_origins.push_back({x, y});
    target.text_runs.insert(target.text_runs.end(), runs_.begin(), runs_.end());
  }

 private:
  std::vector<TextRun> runs_;
};

class RecordingParagraphFactory final : public canvas::ParagraphFactory {
 public:
  std::unique_ptr<canvas::Paragraph> CreateParagraph() override {
    return std::make_unique<RecordingParagraph>(std::move(runs_));
  }
  void PushTextStyle(const SrTextStyle& style) override {
    styles_.push_back(style);
  }
  void PopTextStyle() override { styles_.pop_back(); }
  void SetParagraphStyle(SrParagraphStyle&&) override {}
  void AddText(const std::string& text) override {
    runs_.push_back({text, styles_.back()});
  }
  void Reset() override {
    runs_.clear();
    styles_.clear();
  }

 private:
  std::vector<TextRun> runs_;
  std::vector<SrTextStyle> styles_;
};

void TestTextRendering() {
  const auto render = [](BoundaryCanvas& canvas, const std::string& body) {
    const auto source = "<svg>\n" + body + "\n</svg>";
    auto dom = parser::SrSVGDOM::make(source.data(), source.size(), nullptr);
    assert(dom);
    dom->SetDefaultColor(NSVG_RGB(0, 0, 255));
    dom->Render(&canvas, {0, 0, 100, 100});
    assert(canvas.save_depth == 0);
  };
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<g color='red'><text color='currentColor' "
           "fill='currentColor'>A<tspan color='currentColor'>B</tspan>"
           "</text></g>"
           "<text color='currentColor' fill='currentColor'>Host</text>");
    assert(canvas.text_runs.size() == 3);
    assert(canvas.text_runs[0].text == "A" &&
           canvas.text_runs[0].style.color == NSVG_RGB(255, 0, 0));
    assert(canvas.text_runs[1].text == "B" &&
           canvas.text_runs[1].style.color == NSVG_RGB(255, 0, 0));
    assert(canvas.text_runs[2].text == "Host" &&
           canvas.text_runs[2].style.color == NSVG_RGB(0, 0, 255));
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<defs><linearGradient id='g'><stop stop-color='red'/>"
           "<stop offset='1' stop-color='blue'/></linearGradient></defs>"
           "<text fill='url(#g)'>Fallback<tspan fill='none'>Hidden</tspan>"
           "<tspan fill-opacity='0.5'>Faded</tspan></text>");
    assert(canvas.text_runs.size() == 3);
    assert(canvas.text_runs[0].style.color == NSVG_RGBA(0, 0, 0, 255));
    assert(canvas.text_runs[1].style.color == NSVG_RGBA(0, 0, 0, 0));
    assert(canvas.text_runs[2].style.color == NSVG_RGBA(0, 0, 0, 128));
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<text x='2em' y='3em' font-size='20'>Position</text>"
           "<text x='2em' y='3em'><tspan font-size='20'>Default</tspan></text>"
           "<text x='50%' y='25%' font-size='2em'>Percentage</text>");
    assert((canvas.text_origins ==
            std::vector<std::array<float, 2>>{{40, 60}, {28, 42}, {50, 25}}));
    assert(canvas.text_runs.size() == 3);
    assert(canvas.text_runs[2].style.font_size == 28.f);
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<text font-size='0'>Hidden"
           "<tspan font-size='20'>Shown</tspan>"
           "<tspan font-size='150%'>Hidden</tspan></text>");
    assert(canvas.text_runs.size() == 1);
    assert(canvas.text_runs[0].text == "Shown" &&
           canvas.text_runs[0].style.font_size == 20.f);
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<text>\n  A\t  B<tspan>\n C</tspan>\n  "
           "<tspan fill='red'> D  </tspan>E \r\n</text>");
    assert(canvas.text_runs.size() == 6);
    assert(canvas.text_runs[0].text == "A B");
    assert(canvas.text_runs[1].text == " C");
    assert(canvas.text_runs[2].text == " " &&
           canvas.text_runs[2].style.color == NSVG_RGB(0, 0, 0));
    assert(canvas.text_runs[3].text == "D");
    assert(canvas.text_runs[4].text == " " &&
           canvas.text_runs[4].style.color == NSVG_RGB(255, 0, 0));
    assert(canvas.text_runs[5].text == "E");
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<g xml:space='preserve'><text> A\r\n\tB "
           "<tspan xml:space='default'>C   D</tspan> </text></g>");
    assert(canvas.text_runs.size() == 3);
    assert(canvas.text_runs[0].text == " A  B ");
    assert(canvas.text_runs[1].text == "C D");
    assert(canvas.text_runs[2].text == " ");
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<text>A&amp;B&#32;<tspan>&#x9;&#x4E2D;&#128512;</tspan>"
           "&lt;&gt;&quot;&apos;&amp;lt;</text>"
           "<text xml:space='preserve'>&#13;&#10;X\r\nY&#xA0;</text>"
           "<text>&unknown; &#0; &#xD800; &#x110000; &#4294967296; "
           "&#12 &amp</text>");
    assert(canvas.text_runs.size() == 5);
    assert(canvas.text_runs[0].text == "A&B");
    assert(canvas.text_runs[1].text == u8" 中😀");
    assert(canvas.text_runs[2].text == "<>\"'&lt;");
    assert(canvas.text_runs[3].text == "  X Y\xC2\xA0");
    assert(canvas.text_runs[4].text ==
           "&unknown; &#0; &#xD800; &#x110000; &#4294967296; &#12 &amp");
  }
  {
    BoundaryCanvas canvas;
    render(canvas,
           "<text opacity='0.5'>Text</text><text opacity='0'>Hidden</text>");
    assert(canvas.opacity_attempts == 1 && canvas.opacity_ends == 1);
    assert(canvas.text_runs.size() == 1 && canvas.text_runs[0].text == "Text");
  }
  {
    BoundaryCanvas canvas;
    canvas.fail_opacity = true;
    render(canvas, "<text opacity='0.5'>Text</text>");
    assert(canvas.opacity_attempts == 1 && canvas.opacity_ends == 0);
    assert(canvas.text_runs.size() == 1);
  }
  {
    BoundaryCanvas canvas;
    canvas.offscreen_pixels = parser::SrSVGTraversalState::kMaxOffscreenPixels;
    render(canvas, "<g opacity='0.5'><text opacity='0.5'>Text</text></g>");
    assert(canvas.opacity_attempts == 1 && canvas.opacity_ends == 1);
    assert(canvas.text_runs.size() == 1);
  }
}

void TestNestedViewBox() {
  std::unique_ptr<element::SrSVGSVG> root(element::SrSVGSVG::Make()),
      inner(element::SrSVGSVG::Make());
  std::unique_ptr<element::SrSVGRect> rect(element::SrSVGRect::Make()),
      sibling(element::SrSVGRect::Make());
  root->ParseAndSetAttribute("viewBox", "0 0 100 100");
  inner->ParseAndSetAttribute("viewBox", "0 0 10 10");
  for (auto* shape : {rect.get(), sibling.get()}) {
    shape->ParseAndSetAttribute("width", "50%");
    shape->ParseAndSetAttribute("height", "50%");
  }
  inner->AppendChild(rect.get());
  root->AppendChild(inner.get());
  root->AppendChild(sibling.get());
  SrSVGRenderContext context{};
  context.view_port = {0, 0, 200, 200};
  context.view_box = {1, 2, 30, 40};
  parser::SrSVGTraversalState traversal;
  context.traversal_state = &traversal;
  BoundaryCanvas canvas;
  root->Render(&canvas, context);
  assert(canvas.shapes == 2 && canvas.draw_rect[4] == 50 &&
         canvas.draw_rect[5] == 50 && context.view_box.width == 30);
  assert(root->RenderChildAt(&canvas, context, 0));
  assert(canvas.draw_rect[4] == 5 && context.view_box.width == 30);
  assert(root->RenderChildPathAt(&canvas, context, {0, 0}));
  assert(canvas.draw_rect[5] == 5 && context.view_box.height == 40);
  assert(!root->RenderChildPathAt(&canvas, context, {0, 5}));
  assert(context.view_box.left == 1 && context.view_box.top == 2 &&
         context.view_box.width == 30 && context.view_box.height == 40);
  assert(canvas.save_depth == 0 && traversal.render_depth == 0 &&
         traversal.render_visits > 0);
}

void TestImplicitViewport() {
  auto make = [](const char* xml) {
    return parser::SrSVGDOM::make(xml, strlen(xml), nullptr);
  };
  auto dom = make(
      "<svg width='100' height='100'><rect width='50%' height='25%'/>"
      "<set attributeName='width' to='160px' dur='1s'/></svg>");
  assert(dom);
  BoundaryCanvas canvas;
  dom->Render(&canvas);
  assert(canvas.shapes == 1 && canvas.draw_rect[4] == 50 &&
         canvas.draw_rect[5] == 25);
  dom->Render(&canvas, {0, 0, 80, 40});
  assert(canvas.shapes == 2 && canvas.draw_rect[4] == 40 &&
         canvas.draw_rect[5] == 10);
  dom->Render(&canvas, {0, 0, 0, 100});
  dom->RenderAtTime(&canvas, {0, 0, 100, 0}, .5);
  assert(canvas.shapes == 2);
  dom->RenderAtTime(&canvas, .5);
  assert(canvas.shapes == 3 && canvas.draw_rect[4] == 80);
  dom->RenderAtTime(&canvas, 1.5);
  assert(canvas.shapes == 4 && canvas.draw_rect[4] == 50);

  auto physical = make(
      "<svg width='1in' height='72pt'>"
      "<rect width='100%' height='100%'/></svg>");
  assert(physical);
  physical->Render(&canvas);
  assert(canvas.shapes == 5 && canvas.draw_rect[4] == 96 &&
         canvas.draw_rect[5] == 96);
  physical->dpi_ = 120;
  physical->Render(&canvas);
  assert(canvas.shapes == 6 && canvas.draw_rect[4] == 120 &&
         canvas.draw_rect[5] == 120);

  auto styled = make(
      "<svg style='width:30px;height:20px'>"
      "<set attributeName='width' to='80px' dur='1s'/>"
      "<rect width='100%' height='100%'/></svg>");
  assert(styled);
  styled->RenderAtTime(&canvas, .5);
  assert(canvas.shapes == 7 && canvas.draw_rect[4] == 80);
  styled->RenderAtTime(&canvas, 1.5);
  assert(canvas.shapes == 8 && canvas.draw_rect[4] == 30 &&
         canvas.draw_rect[5] == 20);

  BoundaryCanvas empty_canvas;
  for (const char* xml :
       {"<svg width='100' height='0'><rect width='1' height='1'/></svg>",
        "<svg width='1e38in' height='10'><rect width='1' height='1'/></svg>",
        "<svg width='100' height='100' viewBox='0 0 0 20'>"
        "<rect width='1' height='1'/></svg>"}) {
    auto empty = make(xml);
    assert(empty);
    empty->Render(&empty_canvas);
  }
  assert(empty_canvas.shapes == 0 && empty_canvas.save_depth == 0);
}

void TestRenderBoundaries() {
  const SrSVGBox viewport{0, 0, 100, 100};
  auto make = [](const char* svg) {
    return parser::SrSVGDOM::make(svg, strlen(svg), nullptr);
  };
  {
    // An existing ID of a different type cannot be used as a clip path.
    auto dom = make(
        "<svg><defs><rect id='notClip' width='10' height='10'/></defs>"
        "<rect width='20' height='10' clip-path='url(#notClip)'/></svg>");
    BoundaryCanvas canvas;
    assert(dom);
    dom->Render(&canvas, viewport);
    assert(canvas.shapes == 1 && canvas.clips == 0 && canvas.save_depth == 0);
  }
  {
    // A mask already active on the traversal must not be entered again.
    auto dom = make(
        "<svg><defs><mask id='m' maskUnits='userSpaceOnUse' x='0' y='0' "
        "width='100' height='100'>"
        "<rect width='10' height='10' mask='url(#m)'/></mask></defs>"
        "<rect width='20' height='10' mask='url(#m)'/></svg>");
    BoundaryCanvas canvas;
    assert(dom);
    dom->Render(&canvas, viewport);
    assert(canvas.masks == 1 && canvas.shapes == 2 && canvas.save_depth == 0);
  }
  {
    auto dom = make(
        "<svg><rect width='-1' height='10'/><circle r='2e38'/><ellipse rx='-2' "
        "ry='3'/>"
        "<rect width='1e38in' height='10'/><rect x='3e38' width='1e38' "
        "height='20'/>"
        "<image width='-10' height='10'/><rect width='10' height='10'/></svg>");
    BoundaryCanvas canvas;
    assert(dom);
    dom->Render(&canvas, viewport);
    assert(canvas.shapes == 1 && canvas.save_depth == 0);
    dom->Render(&canvas, SrSVGBox{0, 0, INFINITY, 100});
    assert(canvas.shapes == 1);
  }
  {
    auto dom = make(
        "<svg><defs><g fill='red'><rect id='r' width='20' height='10'>"
        "<set attributeName='fill-rule' to='evenodd' dur='1s'/></rect>"
        "<set attributeName='fill' to='blue' dur='1s'/></g></defs>"
        "<use href='#r'/></svg>");
    BoundaryCanvas canvas;
    assert(dom);
    dom->RenderAtTime(&canvas, viewport, .5);
    assert(canvas.last_fill == NSVG_RGB(0, 0, 255) &&
           canvas.last_rule == SR_SVG_EO_FILL);
    dom->RenderAtTime(&canvas, viewport, 1.5);
    assert(canvas.last_fill == NSVG_RGB(255, 0, 0) &&
           canvas.last_rule == SR_SVG_FILL);
    assert(canvas.save_depth == 0);
  }
  {
    std::unique_ptr<element::SrSVGPath> node(element::SrSVGPath::Make());
    const char* value = "url(#ref)";
    for (auto attribute : {"clip-path", "mask", "filter", "fill", "stroke"}) {
      node->ParseAndSetAttribute(attribute, value);
      const size_t live = allocations.size();
      allocation_count = 0;
      fail_at = 2;  // IRI copy fails after the paint object was allocated.
      node->ParseAndSetAttribute(attribute, value);
      fail_at = 0;
      assert(allocations.size() == live);
      node->ParseAndSetAttribute(attribute, "none");
      assert(allocations.size() == live - 1);
    }
    node->ParseAndSetAttribute("stroke-dasharray", "8,4");
    for (auto invalid : {"8 -1", "8,,4", "8,", "3e38 3e38"}) {
      node->ParseAndSetAttribute("stroke-dasharray", invalid);
      assert((node->stroke_dash_array_ == std::vector<float>{8, 4}));
    }
    node->ParseAndSetAttribute("stroke-dasharray", "none");
    assert(node->stroke_dash_array_.empty());
  }
  {
    parser::SrSVGTraversalState state;
    SrSVGRenderContext context{};
    context.traversal_state = &state;
    assert(!element::PatternTileBudget(context, 0, 0, 100, 100, .01f, .01f)
                .HasRemaining());
    assert(!element::PatternTileBudget(context, 1e20f, 0, 2e20f, 10, 1, 1)
                .HasRemaining());
    element::PatternTileBudget budget(context, 0, 0, 10, 10, 5, 5);
    assert(budget.HasRemaining());
    state.pattern_tiles = parser::SrSVGTraversalState::kMaxPatternTiles - 1;
    budget.Consume();
    assert(!budget.HasRemaining());
  }
}

void TestGeometryAndEffectsContracts() {
  SrSVGRenderContext context{};
  context.dpi = 96.f;
  context.view_box = {0, 0, 1e20f, 1e20f};
  context.view_port = {0, 0, 100, 100};
  float out = 123.f;
  auto length = make_serval_length("1e38in");
  assert(!resolve_serval_length(&length, &context, SR_SVG_LENGTH_TYPE_OTHER,
                                &out) &&
         out == 123.f);
  length = make_serval_length("0in");
  assert(resolve_serval_length(&length, &context, SR_SVG_LENGTH_TYPE_OTHER,
                               &out) &&
         out == 0.f);
  length = make_serval_length("1%");
  assert(resolve_serval_length(&length, &context, SR_SVG_LENGTH_TYPE_OTHER,
                               &out) &&
         std::abs(out / 1e18f - 1.f) < 1e-6f);

  const float singular[] = {1e20f, 1e20f, 1e20f, 1e20f, 0.f, 0.f};
  const float large[] = {1e20f, 0.f, 0.f, 1e20f, 0.f, 0.f};
  float inverse[6] = {123.f};
  assert(!element::InvertAffineTransform(singular, inverse) &&
         inverse[0] == 123.f);
  assert(!element::ComposeAffineTransform(large, large, inverse) &&
         inverse[0] == 123.f);
  assert(element::InvertAffineTransform(large, inverse) &&
         inverse[0] == 1e-20f);
  SrSVGBox mapped{123.f, 0.f, 0.f, 0.f};
  assert(!element::TryMapBounds({0.f, 0.f, 1e20f, 1e20f}, large, &mapped) &&
         mapped.left == 123.f);

  BoundaryCanvas canvas;
  std::unique_ptr<element::SrSVGNodeBase> rect(element::SrSVGRect::Make());
  assert(rect->ParseAndSetAttribute("width", "20"));
  assert(rect->ParseAndSetAttribute("height", "10"));
  assert(rect->ParseAndSetAttribute("ry", "3"));
  assert(!rect->ParseAndSetAttribute("rx", "-1"));
  assert(!rect->ParseAndSetAttribute("height", "10junk"));
  assert(rect->ParseAndSetAttribute("opacity", "50%"));
  assert(!rect->ParseAndSetAttribute("opacity", "invalid"));
  assert(rect->AsPath(&canvas, &context, false));
  rect->Render(&canvas, context);
  assert(canvas.last_opacity == .5f && canvas.draw_rect == canvas.path_rect &&
         canvas.draw_rect[2] == 3.f && canvas.draw_rect[3] == 3.f);

  assert(rect->ParseAndSetAttribute("rx", "0"));
  assert(rect->AsPath(&canvas, &context, false));
  rect->Render(&canvas, context);
  assert(canvas.draw_rect == canvas.path_rect && canvas.draw_rect[2] == 0.f &&
         canvas.draw_rect[3] == 0.f);
  assert(rect->ParseAndSetAttribute("x", "1e38in"));
  assert(!rect->AsPath(&canvas, &context, false));
  rect->Render(&canvas, context);
  assert(canvas.shapes == 2 && canvas.save_depth == 0);

  auto make = [](const std::string& xml) {
    return parser::SrSVGDOM::make(xml.data(), xml.size(), nullptr);
  };
  auto hidden =
      make("<svg viewBox='0 0 0 20'><rect width='10' height='10'/></svg>");
  auto missing = make("<svg><rect width='10' height='10'/></svg>");
  auto invalid =
      make("<svg viewBox='0 0 -5 20'><rect width='10' height='10'/></svg>");
  assert(hidden && missing && invalid);
  BoundaryCanvas root_canvas;
  hidden->Render(&root_canvas, context.view_port);
  assert(root_canvas.shapes == 0);
  missing->Render(&root_canvas, context.view_port);
  invalid->Render(&root_canvas, context.view_port);
  assert(root_canvas.shapes == 2 && root_canvas.save_depth == 0);

  auto effects = make(
      "<svg><defs><mask id='m' maskUnits='userSpaceOnUse' width='100' "
      "height='100'>"
      "<rect width='10' height='10'/></mask>"
      "<filter id='f' filterUnits='userSpaceOnUse' width='100' height='100'>"
      "<feGaussianBlur stdDeviation='2'/></filter></defs>"
      "<rect width='10' height='10' filter='url(#f)' mask='url(#m)'/>"
      "<rect width='3' height='4'/></svg>");
  assert(effects);
  BoundaryCanvas fail_canvas;
  fail_canvas.fail_filter = fail_canvas.fail_mask_content = true;
  effects->Render(&fail_canvas, context.view_port);
  assert(fail_canvas.filter_attempts == 1 && fail_canvas.filters_ended == 0 &&
         fail_canvas.content_attempts == 1 && fail_canvas.contents_ended == 0 &&
         fail_canvas.clips == 1 && fail_canvas.shapes == 2 &&
         fail_canvas.save_depth == 0);
  BoundaryCanvas size_limited;
  effects->Render(&size_limited, {0, 0, 10000, 10000});
  assert(size_limited.filter_attempts == 0 && size_limited.masks == 0 &&
         size_limited.shapes == 2 && size_limited.save_depth == 0);

  parser::SrSVGTraversalState traversal;
  context.traversal_state = &traversal;
  context.view_port = {0, 0, 4096, 4096};
  parser::SrSVGRenderScope render_scope(&context);
  {
    parser::SrSVGOffscreenScope outer(context), inner(context);
    assert(outer.Reserve(canvas, 2, 4));
    assert(!inner.Reserve(canvas, 1));
  }
  parser::SrSVGOffscreenScope next(context), exhausted(context);
  assert(!traversal.live_offscreen_bytes && next.Reserve(canvas, 1, 4));
  assert(!exhausted.Reserve(canvas, 1, 1));

  std::string deep = "<svg><defs><rect id='end' width='3' height='4'/>";
  for (int i = 0; i < 300; ++i) {
    deep += "<use id='n" + std::to_string(i) + "' href='#" +
            (i == 299 ? std::string("end") : "n" + std::to_string(i + 1)) +
            "'/>";
  }
  deep += "</defs><use href='#n0'/><rect width='3' height='4'/></svg>";
  auto deep_dom = make(deep);
  assert(deep_dom);
  BoundaryCanvas deep_canvas;
  deep_dom->Render(&deep_canvas, {0, 0, 100, 100});
  assert(deep_canvas.shapes == 1 && deep_canvas.save_depth == 0);
}

void TestFilterResultRegions() {
  std::unique_ptr<element::SrSVGFilter> filter(element::SrSVGFilter::Make());
  std::unique_ptr<element::SrSVGFeGaussianBlur> first(
      element::SrSVGFeGaussianBlur::Make());
  std::unique_ptr<element::SrSVGFeGaussianBlur> second(
      element::SrSVGFeGaussianBlur::Make());
  std::unique_ptr<element::SrSVGFeOffset> offset(
      element::SrSVGFeOffset::Make());
  std::unique_ptr<element::SrSVGFeComposite> composite(
      element::SrSVGFeComposite::Make());
  filter->ParseAndSetAttribute("filterUnits", "userSpaceOnUse");
  filter->ParseAndSetAttribute("width", "100");
  filter->ParseAndSetAttribute("height", "100");
  first->ParseAndSetAttribute("result", "tile");
  first->ParseAndSetAttribute("width", "10");
  second->ParseAndSetAttribute("result", "tile");
  second->ParseAndSetAttribute("width", "20");
  offset->ParseAndSetAttribute("in", "tile");
  composite->ParseAndSetAttribute("in", "tile");
  composite->ParseAndSetAttribute("in2", "SourceAlpha");
  filter->AppendChild(first.get());
  filter->AppendChild(second.get());
  filter->AppendChild(offset.get());
  filter->AppendChild(composite.get());
  SrSVGRenderContext context{};
  context.view_box = {0, 0, 100, 100};
  canvas::SrFilterModel model;
  assert(filter->BuildFilterModel(context.view_box, true, context, &model));
  assert(model.primitives.size() == 4);
  assert(model.primitives[2].subregion.width == 20);
  assert(model.primitives[3].subregion.width == 100);
}

void TestStrokeDashOffset() {
  const SrSVGBox viewport{0, 0, 100, 100};
  SrSVGRenderContext context{};
  context.dpi = 96;
  context.view_port = context.view_box = viewport;
  std::unique_ptr<element::SrSVGNodeBase> rect(element::SrSVGRect::Make());
  assert(rect->ParseAndSetAttribute("width", "40"));
  assert(rect->ParseAndSetAttribute("height", "20"));
  BoundaryCanvas canvas;
  const struct {
    const char* value;
    float expected;
  } offsets[] = {{"3", 3},    {"3px", 3},  {"-3px", -3},
                 {"25%", 25}, {"1in", 96}, {"1e38in", 0}};
  for (const auto& offset : offsets) {
    assert(rect->ParseAndSetAttribute("stroke-dashoffset", offset.value));
    rect->Render(&canvas, context);
    assert(canvas.last_dash_offset == offset.expected);
  }
  assert(rect->ParseAndSetAttribute("stroke-dashoffset", "3px"));
  for (const char* invalid : {"3junk", "nan", "1e999px", "3px 4px"}) {
    assert(!rect->ParseAndSetAttribute("stroke-dashoffset", invalid));
    rect->Render(&canvas, context);
    assert(canvas.last_dash_offset == 3);
  }

  const char* animated =
      "<svg><rect width='40' height='20' stroke-dashoffset='2px'>"
      "<animate attributeName='stroke-dashoffset' from='1px' to='5px' "
      "dur='2s'/>"
      "<set attributeName='stroke-dashoffset' to='7junk' dur='2s'/>"
      "</rect></svg>";
  auto dom = parser::SrSVGDOM::make(animated, strlen(animated), nullptr);
  assert(dom);
  dom->RenderAtTime(&canvas, viewport, 1);
  assert(canvas.last_dash_offset == 3);
  dom->Render(&canvas, viewport);
  assert(canvas.last_dash_offset == 2);

  const char* styled =
      "<svg><rect width='40' height='20' style='stroke-dashoffset:3px'>"
      "<set attributeName='stroke-dashoffset' to='0px' begin='1s' dur='1s'/>"
      "</rect></svg>";
  dom = parser::SrSVGDOM::make(styled, strlen(styled), nullptr);
  assert(dom);
  dom->RenderAtTime(&canvas, viewport, 0);
  assert(canvas.last_dash_offset == 3);
  dom->RenderAtTime(&canvas, viewport, 1.5);
  assert(canvas.last_dash_offset == 0);
  dom->RenderAtTime(&canvas, viewport, 2.5);
  assert(canvas.last_dash_offset == 3);

  // A use-level zero overrides the referenced phase only during the set.
  const char* use =
      "<svg><defs><rect id='d' width='40' height='20' "
      "stroke-dashoffset='3px'/></defs><use href='#d'>"
      "<set attributeName='stroke-dashoffset' to='0px' dur='1s'/>"
      "</use></svg>";
  dom = parser::SrSVGDOM::make(use, strlen(use), nullptr);
  assert(dom);
  dom->RenderAtTime(&canvas, viewport, .5);
  assert(canvas.last_dash_offset == 0);
  dom->RenderAtTime(&canvas, viewport, 1.5);
  assert(canvas.last_dash_offset == 3);
}

void TestNumbers() {
  {
    double number = 0;
    const char* raw;
    const char* unit = SrSVGParseNumberWithSuffix(" 1e-1 px \t", &number, &raw);
    assert(unit && number == .1 && strcmp(unit, "px") == 0);
    assert(strcmp(raw, " px \t") == 0);
    assert(SrSVGParseNumberWithSuffix("10px", &number, nullptr) == unit);
    assert(!SrSVGParseNumberWithSuffix("7pxjunk", &number, &raw) &&
           number == 10 && strcmp(raw, " px \t") == 0);
  }
  {
    parser::SrXMLParserError error;
    const std::string noun(100, 'x');
    error.SetNoun(noun);
    error.SetNoun(error.GetNoun());
    assert(error.GetNoun() == noun);
    parser::SrXMLParserError copy(error);
    error.Reset();
    assert(!error.HasNoun() && copy.GetNoun() == noun);
  }

  SrSVGLength length{17.f, SR_SVG_UNITS_PX};
  for (const char* input :
       {"", "+", ".", "nan", "inf", "1e999", "1e39", "10junk", "1e+", "0x10"}) {
    assert(!parse_serval_length(input, &length));
    assert(length.value == 17.f && length.unit == SR_SVG_UNITS_PX);
    assert(make_serval_length(input).value == 0.f);
    assert(element::SrSVGNode::Atof(input) == 0.0);
  }
  for (const char* input : {" 1.25px ", "1em", "2ex", "-1e-2%", ".5"}) {
    assert(parse_serval_length(input, &length));
    assert(std::isfinite(length.value));
  }
  SrSVGBox box{1, 2, 3, 4};
  for (const char* input : {"0 0 1", "0 0 inf 2", "0 0 1e39 2", "0 0 -1 2",
                            "0 0 1 2junk", "0 0 1 2 3"}) {
    assert(!parse_serval_view_box(input, &box));
    assert(box.left == 1 && box.height == 4);
  }
  assert(parse_serval_view_box("-1, -2, 30, 40", &box));
  assert(box.left == -1 && box.height == 40);
}

void TestPathsAndPolygons() {
  assert(!make_serval_path("L10 20", nullptr));
  for (const char* input :
       {"M0 0 A1 1 0", "M0 0 A1 1 0 0", "M0 0 A1 1 0 2 1 2 3",
        "M0 0 A1 1 0 0 1 10", "M0 0 L10", "M0 0 Lnan 1", "M0 0 L1e999 1",
        "M0 0 L10junk", "M0 0 L1e+ 2"}) {
    // Exact allocation lets ASan detect a read beyond the string terminator.
    std::string copy(input);
    std::unique_ptr<char[]> text(new char[copy.size() + 1]);
    std::memcpy(text.get(), copy.c_str(), copy.size() + 1);
    assert(!make_serval_path(text.get(), nullptr));
  }
  assert(!make_serval_path(nullptr, nullptr));
  for (const char* input : {"0 0 10", "0 0 nan 1", "0 0 1e999 1", "0 0 10junk"})
    assert(!make_serval_polygon(input, nullptr));

  const std::string long_number = "M" + std::string(100000, '0') + "1 0";
  auto* path = make_serval_path(long_number.c_str(), nullptr);
  assert(path && path->n_ops == 1 && path->args[0] == 1.f);
  release_serval_path(path);

  const char* commands =
      "M0 0 l1 2 h3 v4 C1 2 3 4 5 6 s1 2 3 4 "
      "Q1 2 3 4 t1 2 A10 20 30 01 40 50 z";
  CheckAllocationFailures([&] {
    auto* p = make_serval_path(commands, nullptr);
    const bool success = p != nullptr;
    if (p)
      assert(p->n_ops == 10 && p->n_args == 37);
    release_serval_path(p);
    return success;
  });
  // The C circle builder retains its command counts and initial capacity
  // when an allocation fails partway through appending the circle.
  const uint32_t expected_ops[] = {0, 0, 2, 3, 4};
  const uint32_t expected_args[] = {0, 0, 8, 14, 20};
  for (size_t site = 1; site <= 5; ++site) {
    auto* p = make_serval_path("", nullptr);
    assert(p);
    allocation_count = 0;
    fail_at = site;
    add_circle_to_path(p, 20, 30, 5);
    fail_at = 0;
    assert(p->n_ops == expected_ops[site - 1] &&
           p->n_args == expected_args[site - 1]);
    assert(p->c_ops == (site == 1 ? 0 : 8));
    release_serval_path(p);
    assert(allocations.empty());
  }
  CheckAllocationFailures([] {
    auto* p = make_serval_polygon(
        "0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7 8 8 9 9 10 10 11 11 12 12 13 13 14 14 "
        "15 15 16 16",
        nullptr);
    const bool success = p != nullptr;
    if (p)
      assert(p->n_points == 17);
    release_serval_polygon_path(p);
    return success;
  });
}

void TestTransforms() {
  std::unique_ptr<element::SrSVGG> group(element::SrSVGG::Make());
  assert(group->ParseAndSetAttribute("transform", "matrix(2 1 3 4 5 6)"));
  const float expected[] = {2, 1, 3, 4, 5, 6};
  for (const char* invalid :
       {"scale()", "translate(7 8) rotate(10,20)", "scale(1e30) scale(1e30)"}) {
    assert(!group->ParseAndSetAttribute("transform", invalid));
    for (size_t i = 0; i < 6; ++i)
      assert(group->transform_[i] == expected[i]);
  }

  for (const char* input :
       {"translate()", "scale()", "rotate(10,20)", "matrix(1 0)",
        "translate(1e999)", "translate(1junk)", "scale(1 2 3)", "rotate(10",
        "skewX()", "translate(1) garbage", "scale(1),"}) {
    float out[6] = {7, 7, 7, 7, 7, 7};
    assert(!element::SrSVGNode::TryParseTransform(input, out));
    for (float v : out)
      assert(v == 7);
    element::SrSVGNode::ParseTransform(input, out);
    assert(out[0] == 1 && out[3] == 1 && out[1] == 0 && out[4] == 0);
  }
  for (const char* input :
       {"translate(1)", "scale(2 3)", "rotate(10,20,30)", "matrix(1 0 0 1 2 3)",
        "translate(1,2) scale(2)", "skewX(10),skewY(20)"}) {
    float out[6];
    assert(element::SrSVGNode::TryParseTransform(input, out));
    for (float v : out)
      assert(std::isfinite(v));
  }
}

__attribute__((noinline)) std::unique_ptr<element::SrSVGAnimation>
DelayedAnimation() {
  const std::string text =
      "<animate attributeName='d' from='M0 0 A1 1 0' "
      "to='M0 0 A2 2 0' dur='1s'/>";
  serval::svg::parser::SrDOM xml;
  assert(xml.build(text.data(), text.size(), nullptr, nullptr));
  int reports = 0;
  SrSVGDiagnosticSink sink{&reports, [](void* ctx, SrSVGDiagnosticCode,
                                        const char*, const char*, uint8_t) {
                             ++*static_cast<int*>(ctx);
                           }};
  element::IDMapper ids;
  std::list<element::SrSVGNodeBase*> holder;
  auto* node =
      construct_svg_node(xml, nullptr, xml.GetRootNode(), &ids, holder, &sink);
  return std::unique_ptr<element::SrSVGAnimation>(
      static_cast<element::SrSVGAnimation*>(node));
}

std::unique_ptr<element::SrSVGAnimation> AllocationTestAnimation(bool motion) {
  std::unique_ptr<element::SrSVGAnimation> a(
      motion ? element::SrSVGAnimation::MakeAnimateMotion()
             : element::SrSVGAnimation::MakeAnimate());
  a->ParseAndSetAttribute("dur", "1s");
  if (motion) {
    a->ParseAndSetAttribute("path", "M0 0 C1 2 3 4 5 6 L10 20");
  } else {
    a->ParseAndSetAttribute("attributeName", "d");
    a->ParseAndSetAttribute("from", "M0 0 L10 20");
    a->ParseAndSetAttribute("to", "M10 20 L30 40");
  }
  return a;
}

void TestAnimationAllocationFailures() {
  for (bool motion : {false, true}) {
    size_t c_count, new_count;
    {
      auto a = AllocationTestAnimation(motion);
      allocation_count = 0;
      cpp_count = 0;
      cpp_fail_at = 0;
      fail_cpp = true;
      element::SrSVGAnimation::Effect effect;
      assert(a->Evaluate(.5, nullptr, "", &effect));
      fail_cpp = false;
      c_count = allocation_count;
      new_count = cpp_count;
    }
    assert(allocations.empty());
    const size_t live = cpp_live;
    for (size_t i = 1; i <= new_count; ++i) {
      {
        auto a = AllocationTestAnimation(motion);
        element::SrSVGAnimation::Effect effect;
        cpp_count = 0;
        cpp_fail_at = i;
        fail_cpp = true;
        const bool success = a->Evaluate(.5, nullptr, "", &effect);
        fail_cpp = false;
        if (!success)
          assert(!effect.path_data && effect.value.empty());
        assert(a->Evaluate(.5, nullptr, "", &effect));
      }
      assert(allocations.empty() && cpp_live == live);
    }
    for (size_t i = 1; i <= c_count; ++i) {
      {
        auto a = AllocationTestAnimation(motion);
        element::SrSVGAnimation::Effect effect;
        allocation_count = 0;
        fail_at = i;
        a->Evaluate(.5, nullptr, "", &effect);
        fail_at = 0;
        assert(a->Evaluate(.5, nullptr, "", &effect));
        if (!motion)
          assert(effect.path_data);
      }
      assert(allocations.empty() && cpp_live == live);
    }
  }
}

void TestDashArrayNoneAnimation() {
  for (const char* value : {"none", " \t none \n"}) {
    std::unique_ptr<element::SrSVGAnimation> animation(
        element::SrSVGAnimation::MakeSet());
    animation->ParseAndSetAttribute("attributeName", "stroke-dasharray");
    animation->ParseAndSetAttribute("to", value);
    element::SrSVGAnimation::Effect effect;
    assert(animation->Evaluate(0, nullptr, "8 4", &effect));
    assert(effect.attribute == "stroke-dasharray" && effect.value == value);

    std::unique_ptr<element::SrSVGPath> path(element::SrSVGPath::Make());
    path->ParseAndSetAttribute("stroke-dasharray", "8 4");
    path->StoreAttribute("stroke-dasharray", "8 4");
    path->AddAnimation(animation.get());
    path->ApplyAnimations(0, nullptr);
    assert(path->stroke_dash_array_.empty());
    path->RestoreAnimatedAttributes();
    assert((path->stroke_dash_array_ == std::vector<float>{8, 4}));
    animation->ParseAndSetAttribute("begin", ".5s");
    animation->ParseAndSetAttribute("dur", "1s");
    path->ApplyAnimations(0, nullptr);
    assert((path->stroke_dash_array_ == std::vector<float>{8, 4}));
    path->ApplyAnimations(1, nullptr);
    assert(path->stroke_dash_array_.empty());
    path->RestoreAnimatedAttributes();
    path->ApplyAnimations(1.75, nullptr);
    assert((path->stroke_dash_array_ == std::vector<float>{8, 4}));
  }
  std::unique_ptr<element::SrSVGAnimation> discrete(
      element::SrSVGAnimation::MakeAnimate());
  discrete->ParseAndSetAttribute("attributeName", "stroke-dasharray");
  discrete->ParseAndSetAttribute("values", "8 4;none;2 2");
  discrete->ParseAndSetAttribute("calcMode", "discrete");
  discrete->ParseAndSetAttribute("dur", "3s");
  element::SrSVGAnimation::Effect effect;
  for (const auto& sample : {std::pair<double, const char*>{.5, "8 4"},
                             {1.5, "none"},
                             {2.5, "2 2"}}) {
    assert(discrete->Evaluate(sample.first, nullptr, "8 4", &effect));
    assert(effect.value == sample.second);
  }
  for (const char* attribute : {"stroke-dasharray", "points", "viewBox"}) {
    std::unique_ptr<element::SrSVGAnimation> animation(
        element::SrSVGAnimation::MakeSet());
    animation->ParseAndSetAttribute("attributeName", attribute);
    for (const char* invalid : {"nonejunk", "nan", "inf", "1e999", "10junk"}) {
      animation->ParseAndSetAttribute("to", invalid);
      assert(!animation->Evaluate(0, nullptr, "8 4", &effect));
      assert(effect.value.empty());
    }
    if (strcmp(attribute, "stroke-dasharray")) {
      animation->ParseAndSetAttribute("to", "none");
      assert(!animation->Evaluate(0, nullptr, "8 4", &effect));
    }
  }
}

void TestAnimations() {
  auto delayed = DelayedAnimation();
  element::SrSVGAnimation::Effect delayed_effect;
  assert(!delayed->Evaluate(.5, nullptr, "", &delayed_effect));

  for (const char* values :
       {"0 0;10junk 20", "0 0;nan 20", "0 0;1e999 20", "0 0;10,,20"}) {
    std::unique_ptr<element::SrSVGAnimation> a(
        element::SrSVGAnimation::MakeAnimate());
    a->ParseAndSetAttribute("attributeName", "points");
    a->ParseAndSetAttribute("values", values);
    a->ParseAndSetAttribute("dur", "1s");
    a->ParseAndSetAttribute("fill", "freeze");
    element::SrSVGAnimation::Effect effect;
    assert(!a->Evaluate(.5, nullptr, "", &effect) || effect.value == "0 0");
    assert(!a->Evaluate(1, nullptr, "", &effect));
  }
  std::unique_ptr<element::SrSVGAnimation> a(
      element::SrSVGAnimation::MakeAnimate());
  a->ParseAndSetAttribute("attributeName", "points");
  a->ParseAndSetAttribute("values", "0 0;10 20");
  a->ParseAndSetAttribute("dur", "1s");
  element::SrSVGAnimation::Effect effect;
  assert(a->Evaluate(.5, nullptr, "", &effect) && effect.value == "5 10");
  a->ParseAndSetAttribute("keyTimes", "0;10junk;1");
  assert(!a->Evaluate(.5, nullptr, "", &effect));
  a->ParseAndSetAttribute("keyTimes", "0;;1");
  assert(!a->Evaluate(.5, nullptr, "", &effect));
  a->ParseAndSetAttribute("keyTimes", "0;1");
  assert(a->Evaluate(.5, nullptr, "", &effect));
  std::unique_ptr<element::SrSVGAnimation> morph(
      element::SrSVGAnimation::MakeAnimate());
  morph->ParseAndSetAttribute("attributeName", "d");
  morph->ParseAndSetAttribute("from", "M-3e38 0 L-3e38 1");
  morph->ParseAndSetAttribute("to", "M3e38 0 L3e38 1");
  morph->ParseAndSetAttribute("dur", "1s");
  element::SrSVGAnimation::Effect path_effect;
  assert(morph->Evaluate(.5, nullptr, "", &path_effect));
  assert(path_effect.path_data);
  for (uint32_t i = 0; i < path_effect.path_data->n_args; ++i)
    assert(std::isfinite(path_effect.path_data->args[i]));
  a->ParseAndSetAttribute("dur", "1e-300s");
  a->ParseAndSetAttribute("repeatCount", "indefinite");
  assert(!a->Evaluate(1.0, nullptr, "", &effect));
}

bool BuildXML(const std::string& text) {
  serval::svg::parser::SrDOM dom;
  parser::SrXMLParserError error;
  return dom.build(text.data(), text.size(), &error, nullptr) != nullptr;
}

void TestXML() {
  CheckAllocationFailures([] {
    return BuildXML(
        "<svg width='10' height='20'><g id='a'><path d='M0 0 L1 "
        "1'/></g></svg>");
  });
  std::string deep;
  for (int i = 0; i < parser::kSVGMaxDepth; ++i)
    deep += "<g>";
  std::string close;
  for (int i = 0; i < parser::kSVGMaxDepth; ++i)
    close += "</g>";
  assert(BuildXML(deep + close));
  assert(!BuildXML(deep + "<g/>" + close));
  std::string nodes = "<svg>";
  for (size_t i = 1; i < parser::kSVGMaxNodes; ++i)
    nodes += "<g/>";
  assert(BuildXML(nodes + "</svg>"));
  assert(!BuildXML(nodes + "<g/></svg>"));
  assert(BuildXML("<svg a='" + std::string(parser::kSVGMaxAttributeBytes, 'x') +
                  "'/>"));
  assert(!BuildXML("<svg a='" +
                   std::string(parser::kSVGMaxAttributeBytes + 1, 'x') +
                   "'/>"));
  std::string max_document = "<svg><!--";
  const std::string suffix = "--></svg>";
  max_document.append(
      parser::kSVGMaxDocumentBytes - max_document.size() - suffix.size(), ' ');
  max_document += suffix;
  assert(max_document.size() == parser::kSVGMaxDocumentBytes);
  assert(BuildXML(max_document));
  assert(!BuildXML(max_document + " "));
  std::string attrs = "<svg";
  for (int i = 0; i < 256; ++i)
    attrs += " a" + std::to_string(i) + "='x'";
  assert(BuildXML(attrs + "/>"));
  assert(!BuildXML(attrs + " extra='x'/>"));
  // Exercise the copied-storage budget without requiring an oversized document.
  {
    parser::SrDOMParser parser;
    assert(!parser.StartElement("svg"));
    const std::string value(parser::kSVGMaxAttributeBytes, 'x');
    bool stopped = false;
    for (size_t i = 0; i < 40 && !stopped; ++i)
      stopped = parser.AddAttribute("a", 1, value.data(), value.size());
    assert(stopped && !parser.Finish());
  }
  assert(allocations.empty());
}
}  // namespace

extern "C" void* svg_test_malloc(size_t size) {
  if (FailAllocation())
    return nullptr;
  void* p = std::malloc(size);
  if (p) {
    const bool saved = fail_cpp;
    fail_cpp = false;
    allocations.insert(p);
    fail_cpp = saved;
  }
  return p;
}
extern "C" void* svg_test_calloc(size_t count, size_t size) {
  if (FailAllocation())
    return nullptr;
  void* p = std::calloc(count, size);
  if (p) {
    const bool saved = fail_cpp;
    fail_cpp = false;
    allocations.insert(p);
    fail_cpp = saved;
  }
  return p;
}
extern "C" void* svg_test_realloc(void* ptr, size_t size) {
  if (FailAllocation())
    return nullptr;
  void* p = std::realloc(ptr, size);
  if (p) {
    const bool saved = fail_cpp;
    fail_cpp = false;
    allocations.erase(ptr);
    allocations.insert(p);
    fail_cpp = saved;
  }
  return p;
}
extern "C" void svg_test_free(void* ptr) {
  allocations.erase(ptr);
  std::free(ptr);
}

void* operator new(size_t size) {
  // Fail persistently after the chosen call, including during stack unwinding.
  if (fail_cpp && ++cpp_count >= cpp_fail_at && cpp_fail_at)
    throw std::bad_alloc();
  if (void* ptr = std::malloc(size ? size : 1)) {
    ++cpp_live;
    return ptr;
  }
  throw std::bad_alloc();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* ptr) noexcept {
  if (ptr)
    --cpp_live;
  std::free(ptr);
}
void operator delete[](void* ptr) noexcept {
  ::operator delete(ptr);
}

namespace serval {
namespace svg {
namespace log {
void Log(SrLogger*, const SrSvgLogLevel&) {}
}  // namespace log
}  // namespace svg
}  // namespace serval

namespace serval {
namespace svg {
namespace canvas {
std::unique_ptr<ParagraphFactory> CreateParagraphFactoryFactory(
    const SrCanvas*) {
  return std::make_unique<RecordingParagraphFactory>();
}
}  // namespace canvas
}  // namespace svg
}  // namespace serval

int main(int argc, char** argv) {
  TestTextRendering();
  TestNestedViewBox();
  TestImplicitViewport();
  TestRenderBoundaries();
  TestGeometryAndEffectsContracts();
  TestNumbers();
  TestStrokeDashOffset();
  TestFilterResultRegions();
  TestPaints();
  TestThrowingDiagnostics();
  TestMalformedXMLOOM();
  TestPathsAndPolygons();
  TestTransforms();
  TestDashArrayNoneAnimation();
  TestAnimations();
  TestAnimationAllocationFailures();
  TestXML();
  TestCppAllocationFailures();
  for (int i = 1; i < argc; ++i) {
    std::ifstream file(argv[i]);
    std::string text((std::istreambuf_iterator<char>(file)), {});
    assert(file.good() || file.eof());
    std::vector<parser::SrSVGDiagnostic> diagnostics;
    auto dom = parser::SrSVGDOM::make(text.data(), text.size(), &diagnostics);
    assert(dom);
  }
  assert(allocations.empty());
  std::cout
      << "SVG parser regressions passed (including allocation failures).\n";
}
