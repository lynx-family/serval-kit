// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_PATH_FACTORY_HARMONY_IMPL_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_PATH_FACTORY_HARMONY_IMPL_H_
#include <memory>

#include "canvas/SrCanvas.h"
namespace serval {
namespace svg {
namespace harmony {
class PathFactoryHarmonyImpl : public canvas::PathFactory {
 public:
  PathFactoryHarmonyImpl() = default;
  ~PathFactoryHarmonyImpl() = default;
  std::unique_ptr<canvas::Path> CreateCircle(float cx, float cy,
                                             float r) override;
  std::unique_ptr<canvas::Path> CreateMutable() override;
  std::unique_ptr<canvas::Path> CreateRect(float x, float y, float rx, float ry,
                                           float width, float height) override;
  std::unique_ptr<canvas::Path> CreatePath(uint8_t ops[], uint64_t n_ops,
                                           float args[],
                                           uint64_t n_args) override;
  void Op(canvas::Path* path1, canvas::Path* path2, canvas::OP type) override;

  std::unique_ptr<canvas::Path> CreateLine(float start_x, float start_y,
                                           float end_x, float end_y) override;

  std::unique_ptr<canvas::Path> CreateEllipse(float center_x, float center_y,
                                              float radius_x,
                                              float radius_y) override;

  std::unique_ptr<canvas::Path> CreatePolygon(float points[],
                                              uint32_t n_points) override;
  std::unique_ptr<canvas::Path> CreatePolyline(float points[],
                                               uint32_t n_points) override;
};

}  // namespace harmony
}  // namespace svg
}  // namespace serval
#endif  // SVG_INCLUDE_PLATFORM_HARMONY_PATH_FACTORY_HARMONY_IMPL_H_
