// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_SKITY_SRSKITYCANVAS_H_
#define SVG_INCLUDE_PLATFORM_SKITY_SRSKITYCANVAS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "canvas/SrCanvas.h"
#include "skity/io/data.hpp"
#include "skity/render/canvas.hpp"

namespace serval {
namespace svg {
namespace skity {
class SrWinPath : public canvas::Path {
 public:
  SrWinPath(uint8_t ops[], uint64_t n_ops, float args[], uint64_t n_args)
      : SrWinPath() {}
  ~SrWinPath();
  SrWinPath() : path_(::skity::Path()) {}
  SrWinPath(::skity::Path path) : path_(std::move(path)) {}
  void AddPath(canvas::Path* path) override;
  SrSVGBox GetBounds() const override;
  std::unique_ptr<canvas::Path> CreateTransformCopy(
      const float (&xform)[6]) const override;
  void Transform(const float (&xform)[6]) override;
  void SetFillType(SrSVGFillRule rule) override;
  ::skity::Path* GetSkityPath() { return &path_; }

 private:
  ::skity::Path path_;
};

class SrPathFactorySkity : public canvas::PathFactory {
 public:
  SrPathFactorySkity() = default;
  std::unique_ptr<canvas::Path> CreateCircle(float cx, float cy,
                                             float r) override;
  std::unique_ptr<canvas::Path> CreateMutable() override;
  std::unique_ptr<canvas::Path> CreateRect(float x, float y, float rx, float ry,
                                           float width, float height) override;
  std::unique_ptr<canvas::Path> CreatePath(uint8_t ops[], uint64_t n_ops,
                                           float args[],
                                           uint64_t n_args) override;
  void Op(canvas::Path* path1, canvas::Path* path2, canvas::OP type) override;
  std::unique_ptr<canvas::Path> CreateStrokePath(const canvas::Path* path,
                                                 float width,
                                                 SrSVGStrokeCap cap,
                                                 SrSVGStrokeJoin join,
                                                 float miter_limit) override;
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

class SrSkityCanvas : public canvas::SrCanvas {
 public:
  using ImageCallback =
      std::function<std::shared_ptr<::skity::Image>(std::string)>;
  explicit SrSkityCanvas(::skity::Canvas* canvas, ImageCallback callback);

  canvas::PathFactory* PathFactory() override;
  ~SrSkityCanvas() override;
  void Save() override;
  void Restore() override;
  void SetAntiAlias(bool anti_alias);
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

  void UpdateLinearGradient(
      const char* id, const float (&gradient_transform)[6],
      const GradientSpread spread, float x1, float x2, float y1, float y2,
      const std::vector<SrStop>& stops,
      SrSVGObjectBoundingBoxUnitType bounding_box_type) override;

  void UpdateRadialGradient(
      const char* id, const float (&gradient_transform)[6],
      const GradientSpread spread, float cx, float cy, float fr, float fx,
      float fy, const std::vector<SrStop>& stops,
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
  void ClipPath(canvas::Path* path, SrSVGFillRule clip_rule) override;

  std::shared_ptr<::skity::Data> GetSrSvgDrawImageWithData(
      std::shared_ptr<::skity::Data> data, float width, float height,
      SrSkityCanvas::ImageCallback image_callback);

 private:
  ::skity::Paint ConvertToPaint(const SrSVGRenderState& render_state,
                                ::skity::Rect bound, bool is_stroke);

 private:
  ::skity::Canvas* canvas_{nullptr};
  ImageCallback image_callback_;
  std::unique_ptr<SrPathFactorySkity> path_factory_;
  std::unordered_map<std::string, canvas::LinearGradientModel> lg_models_;
  std::unordered_map<std::string, canvas::RadialGradientModel> rg_models_;
};

}  // namespace skity
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_SKITY_SRSKITYCANVAS_H_
