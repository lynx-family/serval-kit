// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "canvas/SrCanvas.h"

#include <initializer_list>

namespace serval::svg::canvas {
namespace {

class PathCommands {
 public:
  void Add(uint8_t op, std::initializer_list<float> values) {
    ops_.push_back(op);
    args_.insert(args_.end(), values);
  }

  void Arc(float x1, float y1, float rx, float ry, float x2, float y2) {
    Add(SPO_ELLIPTICAL_ARC, {x1, y1, rx, ry, 0, 0, 1, x2, y2});
  }

  std::unique_ptr<Path> Build(PathFactory* factory) {
    return factory->CreatePath(ops_.data(), ops_.size(), args_.data(),
                               args_.size());
  }

 private:
  std::vector<uint8_t> ops_;
  std::vector<float> args_;
};

std::unique_ptr<Path> BuildPolyline(PathFactory* factory, float* points,
                                    uint32_t count, bool closed) {
  if (!points || !count) {
    return nullptr;
  }
  PathCommands path;
  for (uint32_t i = 0; i < count; ++i) {
    const size_t offset = static_cast<size_t>(i) * 2;
    path.Add(i == 0 ? SPO_MOVE_TO : SPO_LINE_TO,
             {points[offset], points[offset + 1]});
  }
  if (closed) {
    path.Add(SPO_CLOSE, {});
  }
  return path.Build(factory);
}

}  // namespace

std::unique_ptr<Path> PathFactory::CreateLine(float x1, float y1, float x2,
                                              float y2) {
  PathCommands path;
  path.Add(SPO_MOVE_TO, {x1, y1});
  path.Add(SPO_LINE_TO, {x2, y2});
  return path.Build(this);
}

std::unique_ptr<Path> PathFactory::CreatePolyline(float* points,
                                                  uint32_t count) {
  return BuildPolyline(this, points, count, false);
}

std::unique_ptr<Path> PathFactory::CreatePolygon(float* points,
                                                 uint32_t count) {
  return BuildPolyline(this, points, count, true);
}

std::unique_ptr<Path> PathFactory::CreateEllipse(float cx, float cy, float rx,
                                                 float ry) {
  if (!(rx > 0 && ry > 0)) {
    return nullptr;
  }
  PathCommands path;
  path.Add(SPO_MOVE_TO, {cx + rx, cy});
  path.Arc(cx + rx, cy, rx, ry, cx, cy + ry);
  path.Arc(cx, cy + ry, rx, ry, cx - rx, cy);
  path.Arc(cx - rx, cy, rx, ry, cx, cy - ry);
  path.Arc(cx, cy - ry, rx, ry, cx + rx, cy);
  path.Add(SPO_CLOSE, {});
  return path.Build(this);
}

std::unique_ptr<Path> PathFactory::CreateCircle(float cx, float cy, float r) {
  return CreateEllipse(cx, cy, r, r);
}

std::unique_ptr<Path> PathFactory::CreateRect(float x, float y, float rx,
                                              float ry, float width,
                                              float height) {
  if (width <= 0 || height <= 0) {
    return nullptr;
  }
  rx = std::min(std::max(rx, 0.f), width / 2);
  ry = std::min(std::max(ry, 0.f), height / 2);
  PathCommands path;
  if (rx == 0 || ry == 0) {
    path.Add(SPO_MOVE_TO, {x, y});
    path.Add(SPO_LINE_TO, {x + width, y});
    path.Add(SPO_LINE_TO, {x + width, y + height});
    path.Add(SPO_LINE_TO, {x, y + height});
  } else {
    path.Add(SPO_MOVE_TO, {x + rx, y});
    path.Add(SPO_LINE_TO, {x + width - rx, y});
    path.Arc(x + width - rx, y, rx, ry, x + width, y + ry);
    path.Add(SPO_LINE_TO, {x + width, y + height - ry});
    path.Arc(x + width, y + height - ry, rx, ry, x + width - rx, y + height);
    path.Add(SPO_LINE_TO, {x + rx, y + height});
    path.Arc(x + rx, y + height, rx, ry, x, y + height - ry);
    path.Add(SPO_LINE_TO, {x, y + ry});
    path.Arc(x, y + ry, rx, ry, x + rx, y);
  }
  path.Add(SPO_CLOSE, {});
  return path.Build(this);
}

}  // namespace serval::svg::canvas
