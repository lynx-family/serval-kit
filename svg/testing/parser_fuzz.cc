// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "core_test_support.h"
#include "parser/SrDOM.h"
#include "parser/SrSVGParse.h"
#include "parser/SrXMLParserError.h"
#include "utils/SrSVGArc.h"
using serval::svg::parser::SrDOM;
using serval::svg::parser::SrXMLParserError;
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size > 65536)
    return 0;
  SVGTestReset(size ? data[0] % 32 : 0, true);
  const char* text = reinterpret_cast<const char*>(data);
  SrSVGParseError error;
  SrPathData* path = nullptr;
  if (SrSVGParsePath(text, size, &path, &error)) {
    assert(path && error.code == SR_PARSE_OK);
    const size_t counts[] = {2, 2, 6, 4, 9, 0};
    size_t count = 0;
    for (uint32_t i = 0; i < path->n_ops; ++i) {
      assert(path->ops[i] < 6);
      count += counts[path->ops[i]];
    }
    assert(count == path->n_args);
    for (uint32_t i = 0; i < path->n_args; ++i)
      assert(std::isfinite(path->args[i]));
  } else
    assert(!path && error.code != SR_PARSE_OK && error.offset <= size);
  release_serval_path(path);
  SrPolygon* polygon = nullptr;
  if (SrSVGParsePolygon(text, size, &polygon, &error)) {
    assert(polygon && error.code == SR_PARSE_OK);
    for (size_t i = 0; i < (size_t)polygon->n_points * 2; ++i)
      assert(std::isfinite(polygon->points[i]));
  } else
    assert(!polygon && error.code != SR_PARSE_OK && error.offset <= size);
  release_serval_polygon_path(polygon);
  SrSVGLength length{42, SR_SVG_UNITS_PX};
  if (!SrSVGParseLength(text, size, &length, &error))
    assert(length.value == 42 && length.unit == SR_SVG_UNITS_PX);
  float transform[6] = {42, 42, 42, 42, 42, 42};
  if (!SrSVGParseTransform(text, size, transform, &error))
    for (float v : transform)
      assert(v == 42);
  else
    for (float v : transform)
      assert(std::isfinite(v));
  SrSVGBox box{1, 2, 3, 4};
  if (!SrSVGParseViewBox(text, size, &box, &error))
    assert(box.left == 1 && box.top == 2 && box.width == 3 && box.height == 4);
  {
    SrDOM dom;
    SrXMLParserError xml_error;
    dom.build(text, size, &xml_error, nullptr);
  }
  if (size >= sizeof(float) * 9) {
    float args[9];
    memcpy(args, data, sizeof(args));
    SrSVGArc arc;
    if (SrSVGArcToCubics(args, &arc)) {
      assert(arc.count <= SR_SVG_ARC_MAX_CUBICS);
      for (size_t i = 0; i < arc.count; ++i)
        for (double v : arc.cubics[i])
          assert(std::isfinite(v) && fabs(v) <= FLT_MAX);
    }
  }
  assert(!SVGTestLive());
  SVGTestAllowNew();
  return 0;
}
