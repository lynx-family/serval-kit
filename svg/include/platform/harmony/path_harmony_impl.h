// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_PATH_HARMONY_IMPL_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_PATH_HARMONY_IMPL_H_

#include <memory>

#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_types.h>
#include "canvas/SrCanvas.h"
namespace serval {
namespace svg {
namespace harmony {
class PathHarmonyImpl : public canvas::Path {
 public:
  PathHarmonyImpl(uint8_t ops[], uint64_t n_ops, float args[], uint64_t n_args);
  PathHarmonyImpl() : path_(OH_Drawing_PathCreate()) {}
  explicit PathHarmonyImpl(const PathHarmonyImpl& pathHarmonyImpl);
  ~PathHarmonyImpl() override;
  void AddPath(canvas::Path* path) override;
  OH_Drawing_Path* GetPath() const;
  SrSVGBox GetBounds() const override;
  std::unique_ptr<canvas::Path> CreateTransformCopy(
      const float (&xform)[6]) const override;
  void Transform(const float (&xform)[6]) override;

 private:
  OH_Drawing_Path* path_;
  void SRSVGArcToBezier(OH_Drawing_Path* path, float cx, float cy, float a,
                        float b, float e1x, float e1y, float theta, float start,
                        float sweep);
  void SrSVGDrawArc(OH_Drawing_Path* path, float x, float y, float x1, float y1,
                    float a, float b, float theta, bool isMoreThanHalf,
                    bool isPositiveArc);
};
}  // namespace harmony
}  // namespace svg
}  // namespace serval
#endif  // SVG_INCLUDE_PLATFORM_HARMONY_PATH_HARMONY_IMPL_H_
