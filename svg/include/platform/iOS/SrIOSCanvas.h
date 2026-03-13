// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SERVAL_SVG_PLATFORM_IOS_SR_IOS_CANVAS_H
#define SERVAL_SVG_PLATFORM_IOS_SR_IOS_CANVAS_H

#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "canvas/SrCanvas.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace ios {

class PathQuartz2D : public canvas::Path {
 public:
  PathQuartz2D(uint8_t ops[], uint64_t n_ops, float args[], uint64_t n_args)
      : PathQuartz2D() {}
  ~PathQuartz2D();
  PathQuartz2D() : path_(CGPathCreateMutable()) {}
  PathQuartz2D(CGMutablePathRef path) : path_(path) { CGPathRetain(path_); }
  void AddPath(Path* path) override;
  CGPathRef GetPath() const;
  SrSVGBox GetBounds() const override;
  std::unique_ptr<Path> CreateTransformCopy(
      const float (&xform)[6]) const override;
  void Transform(const float (&xform)[6]) override;

 private:
  CGMutablePathRef path_;
};

class PathFactoryQuartz2D : public canvas::PathFactory {
 public:
  PathFactoryQuartz2D() = default;
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
                                           float end_x, float end_y) override {
    return nullptr;
  }
  std::unique_ptr<canvas::Path> CreateEllipse(float center_x, float center_y,
                                              float radius_x,
                                              float radius_y) override {
    return nullptr;
  }
  std::unique_ptr<canvas::Path> CreatePolygon(float points[],
                                              uint32_t n_points) override {
    return nullptr;
  }
  std::unique_ptr<canvas::Path> CreatePolyline(float points[],
                                               uint32_t n_points) override {
    return nullptr;
  }
};

class SrIOSCanvas : public canvas::SrCanvas {
 public:
  using ImageCallback = std::function<UIImage*(NSString*)>;
  SrIOSCanvas(CGContextRef context);
  SrIOSCanvas(CGContextRef context, ImageCallback callback);
  SrIOSCanvas(CGFloat width, CGFloat height);
  canvas::PathFactory* PathFactory() override;
  ~SrIOSCanvas();
#pragma mark PreparingContext
  void Save() override;
  void Restore() override;

#pragma mark RenderingCommands
  void DrawLine(const char*, float x1, float y1, float x2, float y2,
                const SrSVGRenderState& render_state) override;
  void DrawRect(const char* id, float x, float y, float rx, float ry,
                float width, float height,
                const SrSVGRenderState& render_state) override;
  void DrawCircle(const char*, float cx, float cy, float r,
                  const SrSVGRenderState& render_state) override;
  void DrawPolygon(const char*, float points[], uint32_t n_points,
                   const SrSVGRenderState& render_state) override;
  void DrawPath(const char*, uint8_t ops[], uint32_t n_ops, float args[],
                uint32_t n_args, const SrSVGRenderState& render_state) override;

  void SetViewBox(float x, float y, float width, float height) override;

  void UpdateLinearGradient(const char* id,
                            const float (&gradient_transform)[6],
                            const GradientSpread spread, float x1, float x2,
                            float y1, float y2,
                            const std::vector<SrStop>& vector,
                            SrSVGObjectBoundingBoxUnitType obb_type) override;

  void UpdateRadialGradient(
      const char* id, const float (&gradient_transform)[6],
      const GradientSpread spread, float cx, float cy, float fr, float fx,
      float fy, const std::vector<SrStop>& vector,
      SrSVGObjectBoundingBoxUnitType bounding_box_type) override;

  void DrawUse(const char* href, float x, float y, float width,
               float height) override;

  void DrawImage(
      const char* url, float x, float y, float width, float height,
      const SrSVGPreserveAspectRatio& preserve_aspect_radio) override;

  void DrawEllipse(const char*, float center_x, float center_y, float radius_x,
                   float radius_y,
                   const SrSVGRenderState& render_state) override;

  void DrawPolyline(const char*, float* points, uint32_t n_points,
                    const SrSVGRenderState& render_state) override;
  void Translate(float x, float y) override;
  void Transform(const float (&form)[6]) override;
  void ClipPath(canvas::Path*, SrSVGFillRule clip_rule) override;

  CGContextRef Context() { return _context; }

 private:
  void FillPath(CGMutablePathRef cgPath, const SrSVGRenderState& renderState);
  void StrokePath(CGMutablePathRef cgPath, const SrSVGRenderState& renderState);

  // Todo: add more drawing methods as needed
 private:
  CGContextRef _context;
  ImageCallback image_callback_;
  std::unique_ptr<PathFactoryQuartz2D> path_factory_;
  std::unordered_map<std::string, canvas::LinearGradientModel> lg_models_;
  std::unordered_map<std::string, canvas::RadialGradientModel> rg_models_;
};

}  // namespace ios
}  // namespace svg
}  // namespace serval

#endif  // SERVAL_SVG_PLATFORM_IOS_SR_IOS_CANVAS_H
