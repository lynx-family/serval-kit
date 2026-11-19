// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "canvas/SrCanvasTransformStack.h"
#include "core_test_support.h"
#include "parser/SrDOM.h"
#include "parser/SrSVGParse.h"
#include "parser/SrXMLParser.h"
#include "parser/SrXMLParserError.h"
#include "utils/SrSVGArc.h"
namespace parser = serval::svg::parser;
using serval::svg::parser::SrXMLParserError;
namespace {
size_t sweep_cases = 0, swept_sites = 0;
template <typename F>
void Sweep(F parse) {
  SVGTestReset(0);
  assert(parse());
  assert(!SVGTestLive());
  size_t count = SVGTestAllocations();
  assert(count);
  ++sweep_cases;
  swept_sites += count;
  for (size_t n = 1; n <= count; ++n) {
    SVGTestReset(n, true);
    assert(!parse());
    assert(!SVGTestLive());
  }
  SVGTestReset(0);
  assert(parse());
  assert(!SVGTestLive());
}
void CanvasTransformSaves() {
  using Stack = serval::svg::canvas::SrCanvasTransformStack;
  Stack stack;
  Stack::Matrix matrix{1, 0, 0, 1, 0, 0};
  int begins = 0;
  auto begin = [&] {
    ++begins;
    return true;
  };

  SVGTestReset(1);
  stack.Save();  // No allocation for a plain save.
  assert(!stack.TrySaveLayer(matrix, begin) && begins == 0);
  assert(stack.BeforeTransform(matrix));
  matrix[4] = 10;
  assert(!stack.TrySaveLayer(matrix, [] { return false; }));
  assert(stack.TrySaveLayer(matrix, begin) && begins == 1);
  matrix[4] = 20;
  stack.Save();
  assert(stack.BeforeTransform(matrix));
  matrix[4] = 30;
  stack.Restore(matrix);
  assert(matrix[4] == 20);
  stack.Restore(matrix);
  assert(matrix[4] == 10);
  stack.Restore(matrix);
  assert(matrix[4] == 0);
}

void CanvasTransformGrowthFailure() {
  using Stack = serval::svg::canvas::SrCanvasTransformStack;
  Stack stack;
  Stack::Matrix matrix{1, 0, 0, 1, 0, 0};
  SVGTestReset(2, true);
  assert(stack.TrySaveLayer(matrix, [] { return true; }));
  assert(stack.BeforeTransform(matrix));
  matrix[4] = 10;

  bool called = false;
  assert(!stack.TrySaveLayer(matrix, [&] {
    called = true;
    return true;
  }));
  assert(!called);
  stack.Save();
  assert(!stack.BeforeTransform(matrix));
  stack.Restore(matrix);
  assert(matrix[4] == 10);
  stack.Restore(matrix);
  assert(matrix[4] == 0);
}

void Bounds() {
  SrPathData* unchanged = reinterpret_cast<SrPathData*>(1);
  SrSVGParseError limit_error;
  assert(!SrSVGParsePath("", SIZE_MAX, &unchanged, &limit_error) &&
         limit_error.code == SR_PARSE_LIMIT);
  assert(unchanged == reinterpret_cast<SrPathData*>(1));
  const char number[] = {'1', '.', '5'};
  SrSVGParseCursor c = SrSVGCursor(number, sizeof(number));
  double out = 0;
  assert(SrSVGParseScalar(&c, &out, nullptr) && out == 1.5 && c.pos == c.end);
  const char bad[] = {'1', 'e', '+'};
  c = SrSVGCursor(bad, sizeof(bad));
  out = 42;
  SrSVGParseError error;
  assert(!SrSVGParseScalar(&c, &out, &error) && c.pos == bad && out == 42 &&
         error.code == SR_PARSE_INVALID);
  for (const char* data :
       {"M0 0 A1 1 0", "M0 0 L10", "M0 0 A1 1 0 2 1 0 0", "M0 0 Z1", "M,0 0",
        "M0 0,", "M0 0,L1 1", "M0 0 L1e39 2"}) {
    SrPathData* path = reinterpret_cast<SrPathData*>(1);
    assert(!SrSVGParsePath(data, strlen(data), &path, &error));
    assert(path == reinterpret_cast<SrPathData*>(1) &&
           error.code != SR_PARSE_OK);
  }
  SrSVGBuffer buffer{};
  assert(SrSVGBufferAppend(&buffer, "01234567890123456789012345678901", 32));
  assert(SrSVGBufferAppend(&buffer, buffer.data, 32));
  assert(buffer.size == 64 &&
         !memcmp(buffer.data, (char*)buffer.data + 32, 32));
  SrSVGBufferRelease(&buffer);
  SrPathData* path = nullptr;
  const char* smooth = "M0 0 Q10 10 20 0 S30 0 40 0 T50 0";
  assert(SrSVGParsePath(smooth, strlen(smooth), &path, nullptr));
  assert(path->args[6] == 20 &&
         path->args[7] == 0);  // Q -> S has no reflected cubic control.
  assert(path->args[12] == 40 &&
         path->args[13] == 0);  // S -> T has no reflected quadratic control.
  release_serval_path(path);
}
void Arcs() {
  for (float radius : {0.f, -10.f, 10.f, FLT_MIN, FLT_MAX}) {
    for (float angle : {0.f, 45.f, FLT_MAX}) {
      const float args[9] = {0, 0, radius, radius, angle, 1, 1, 20, 30};
      SrSVGArc arc;
      assert(SrSVGArcToCubics(args, &arc) &&
             arc.count <= SR_SVG_ARC_MAX_CUBICS);
      for (size_t i = 0; i < arc.count; ++i)
        for (double v : arc.cubics[i])
          assert(std::isfinite(v) && fabs(v) <= FLT_MAX);
      if (radius == 0)
        assert(arc.line && arc.count == 0);
    }
  }
  float pos[9] = {1, 2, 10, 20, 30, 0, 1, 40, 50};
  float neg[9];
  memcpy(neg, pos, sizeof(pos));
  neg[2] = -neg[2];
  neg[3] = -neg[3];
  SrSVGArc a{}, b{};
  assert(SrSVGArcToCubics(pos, &a) && SrSVGArcToCubics(neg, &b));
  assert(a.count == b.count && a.line == b.line &&
         !memcmp(a.cubics, b.cubics, a.count * sizeof(a.cubics[0])));

  // A tiny half-chord denominator used to overflow before radius correction.
  const float tiny[] = {0, 0, 1e-20f, 1e-20f, 0, 0, 1, 20, 0};
  const float corrected[] = {0, 0, 10, 10, 0, 0, 1, 20, 0};
  assert(SrSVGArcToCubics(tiny, &a) && SrSVGArcToCubics(corrected, &b));
  assert(!a.line && a.count >= 4 && b.count == a.count);
  for (size_t i = 0; i < a.count; ++i) {
    for (size_t j = 0; j < 6; ++j)
      assert(std::abs(a.cubics[i][j] - b.cubics[i][j]) < 1e-12);
  }
  assert(std::abs(a.cubics[a.count / 2][5]) > 9);

  // Close endpoints can still describe nearly a full revolution.
  const float near[] = {0, 0, 100, 100, 0, 1, 1, .05f, 0};
  assert(SrSVGArcToCubics(near, &a) && !a.line && a.count == 8);
  assert(std::abs(a.cubics[3][5]) > 199);
  assert(a.cubics[7][4] == near[7] && a.cubics[7][5] == near[8]);

  // Coincidence takes precedence even with a zero radius.
  const float coincident[] = {40, 40, 0, 10, 0, 1, 1, 40, 40};
  assert(SrSVGArcToCubics(coincident, &a) && !a.line && a.count == 0);

  const float shallow[] = {0, 0, 1e30f, 1e30f, 0, 0, 1, 20, 0};
  assert(SrSVGArcToCubics(shallow, &a) && !a.line && a.count == 1);
  assert(std::abs(a.cubics[0][0] - 20.0 / 3) < 1e-12 &&
         std::abs(a.cubics[0][2] - 40.0 / 3) < 1e-12);
  pos[2] = NAN;
  a.count = 15;
  assert(!SrSVGArcToCubics(pos, &a) && a.count == 15);
}
}  // namespace
int main() {
  CanvasTransformSaves();
  CanvasTransformGrowthFailure();
  SVGTestReset(0);
  Bounds();
  Arcs();
  assert(!SVGTestLive());
  Sweep([] {
    SrPathData* p = nullptr;
    const char* d = "M0 0 C1 2 3 4 5 6 A2 3 45 01 8 9 L10 10 Z";
    SrSVGParseError e;
    bool ok = SrSVGParsePath(d, strlen(d), &p, &e);
    if (!ok)
      assert(!p && e.code == SR_PARSE_OOM);
    release_serval_path(p);
    return ok;
  });
  Sweep([] {
    SrPolygon* p = nullptr;
    const char* d = "0 0 1 1 2 2 3 3 4 4 5 5";
    SrSVGParseError e;
    bool ok = SrSVGParsePolygon(d, strlen(d), &p, &e);
    if (!ok)
      assert(!p && e.code == SR_PARSE_OOM);
    release_serval_polygon_path(p);
    return ok;
  });
  Sweep([] {
    const char* d =
        "<svg id='x'><text>buffered text</text><g><path d='M0 0'/></g></svg>";
    parser::SrDOM dom;
    SrXMLParserError error;
    return dom.build(d, strlen(d), &error, nullptr) != nullptr;
  });
  Sweep([] {
    auto* p = make_serval_paint("url(#paint)");
    bool ok = p;
    release_serval_paint(p);
    return ok;
  });
  Sweep([] {
    parser::SrDOM dom;
    const auto* original = dom.build("<svg/>", 6, nullptr, nullptr);
    if (!original)
      return false;
    const char* next = "<svg id='replacement'><g/></svg>";
    const auto* updated = dom.build(next, strlen(next), nullptr, nullptr);
    if (!updated)
      assert(dom.GetRootNode() == original);
    return updated != nullptr;
  });
  Sweep([] {
    parser::SrDOM dom;
    auto* parser = dom.BeginParsing();
    if (!parser)
      return false;
    const char* d = "<svg id='incremental'><g/></svg>";
    if (!parser->parse(d, strlen(d)))
      return false;
    return dom.FinishParsing() != nullptr;
  });
  Sweep([] {
    parser::SrDOM source, destination;
    const char* d = "<svg id='copy'><g><text>copy me</text></g></svg>";
    const auto* original = source.build(d, strlen(d), nullptr, nullptr);
    if (!original)
      return false;
    return destination.Copy(source, original) != nullptr;
  });
  char long_number[4097];
  memset(long_number, '0', sizeof(long_number));
  long_number[4095] = '1';
  long_number[4096] = 0;
  Sweep([&] {
    SrSVGParseCursor c = SrSVGCursor(long_number, 4096);
    double n = 0;
    SrSVGParseError e;
    bool ok = SrSVGParseScalar(&c, &n, &e);
    if (ok)
      assert(n == 1);
    else
      assert(e.code == SR_PARSE_OOM && n == 0 && c.pos == long_number);
    return ok;
  });
  SVGTestReset(1, true);
  {
    parser::SrDOM dom;
    SrXMLParserError e;
    assert(!dom.build("<svg/>", 6, &e, nullptr));
    assert(e.HasError());
  }
  SVGTestReset(0);
  for (const char* d :
       {"</svg>", "</long-closing-tag-without-root>",
        "<svg/><extra-long-second-root-element/>", "<svg><g></svg>"}) {
    parser::SrDOM dom;
    SrXMLParserError e;
    assert(!dom.build(d, strlen(d), &e, nullptr));
  }
  assert(!SVGTestLive());
  printf(
      "No-exception parser regressions passed: %zu scenarios, %zu OOM sites; "
      "operator new forbidden.\n",
      sweep_cases, swept_sites);
  return 0;
}
