// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_CANVAS_SRCANVAS_H_
#define SVG_INCLUDE_CANVAS_SRCANVAS_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace canvas {

enum OP {
  // Subtract the second path from the first path.
  DIFFERENCE = 0,
  // Intersect the two paths.
  INTERSECT,
  // Union (inclusive-or) the two paths.
  UNION,
  // Exclusive-or the two paths.
  XOR,
  // Subtract the first path from the second path.
  REVERSE_DIFFERENCE,
};

enum class SrFilterPrimitiveType {
  kGaussianBlur,
  kOffset,
  kColorMatrix,
  kComposite,
  kBlend,
  kFlood,
};

struct SrFilterPrimitiveModel {
  SrFilterPrimitiveType type{SrFilterPrimitiveType::kGaussianBlur};
  SrSVGBox subregion{0.f, 0.f, 0.f, 0.f};
  std::string input;
  std::string input2;
  std::string result;
  float std_deviation_x{0.f};
  float std_deviation_y{0.f};
  float dx{0.f};
  float dy{0.f};
  std::string color_matrix_type{"matrix"};
  std::vector<float> color_matrix_values;
  std::string composite_operator{"over"};
  float k1{0.f};
  float k2{0.f};
  float k3{0.f};
  float k4{0.f};
  std::string blend_mode{"normal"};
  uint32_t flood_color{0xFF000000};
  float flood_opacity{1.f};
};

struct SrFilterModel {
  SrSVGBox region{0.f, 0.f, 0.f, 0.f};
  std::vector<SrFilterPrimitiveModel> primitives;
};

inline bool SrFilterBoxesEqual(const SrSVGBox& first, const SrSVGBox& second) {
  constexpr float kEpsilon = 1e-5f;
  return std::fabs(first.left - second.left) <= kEpsilon &&
         std::fabs(first.top - second.top) <= kEpsilon &&
         std::fabs(first.width - second.width) <= kEpsilon &&
         std::fabs(first.height - second.height) <= kEpsilon;
}

inline bool SrFilterPrimitiveSupportedInLinearLayer(
    const SrFilterPrimitiveModel& primitive) {
  switch (primitive.type) {
    case SrFilterPrimitiveType::kGaussianBlur:
    case SrFilterPrimitiveType::kOffset:
      return true;
    case SrFilterPrimitiveType::kColorMatrix:
      if (primitive.color_matrix_type == "luminanceToAlpha") {
        return true;
      }
      return primitive.color_matrix_type == "matrix" &&
             primitive.color_matrix_values.size() == 20;
    case SrFilterPrimitiveType::kComposite:
    case SrFilterPrimitiveType::kBlend:
    case SrFilterPrimitiveType::kFlood:
      return false;
  }
  return false;
}

inline bool SrSupportsLinearSourceGraphicFilterModel(
    const SrFilterModel& filter) {
  if (filter.region.width <= 0.f || filter.region.height <= 0.f) {
    return true;
  }

  std::string expected_input = "SourceGraphic";
  for (const auto& primitive : filter.primitives) {
    if (!SrFilterPrimitiveSupportedInLinearLayer(primitive)) {
      return false;
    }
    if (primitive.input != expected_input) {
      return false;
    }
    if (!primitive.input2.empty()) {
      return false;
    }
    if (!SrFilterBoxesEqual(primitive.subregion, filter.region)) {
      return false;
    }
    expected_input = primitive.result;
  }
  return true;
}

class Path {
 public:
  Path() = default;
  virtual ~Path() = default;
  virtual SrSVGBox GetBounds() const = 0;
  virtual void Transform(const float (&xform)[6]) = 0;
  virtual std::unique_ptr<Path> CreateTransformCopy(
      const float (&xform)[6]) const = 0;
  virtual void AddPath(Path* path) = 0;
  virtual void SetFillType(SrSVGFillRule rule) = 0;
};

class PathFactory {
 public:
  virtual ~PathFactory() = default;
  virtual std::unique_ptr<Path> CreateCircle(float cx, float cy, float r) = 0;
  virtual std::unique_ptr<Path> CreateRect(float x, float y, float rx, float ry,
                                           float width, float height) = 0;
  virtual std::unique_ptr<Path> CreateLine(float start_x, float start_y,
                                           float end_x, float end_y) = 0;

  virtual std::unique_ptr<Path> CreateEllipse(float center_x, float center_y,
                                              float radius_x,
                                              float radius_y) = 0;

  virtual std::unique_ptr<Path> CreatePolygon(float points[],
                                              uint32_t n_points) = 0;
  virtual std::unique_ptr<Path> CreatePolyline(float points[],
                                               uint32_t n_points) = 0;

  virtual std::unique_ptr<Path> CreateMutable() = 0;
  virtual std::unique_ptr<Path> CreatePath(uint8_t ops[], uint64_t n_ops,
                                           float args[], uint64_t n_args) = 0;
  virtual void Op(Path* path1, Path* path2, OP type) = 0;
  virtual std::unique_ptr<Path> CreateStrokePath(const Path* path, float width,
                                                 SrSVGStrokeCap cap,
                                                 SrSVGStrokeJoin join,
                                                 float miter_limit) = 0;
};

class GradientModel {
 public:
  GradientModel() = default;
  GradientModel(const GradientSpread spread_mode, const float (&form)[6],
                const std::vector<SrStop>& stops,
                SrSVGObjectBoundingBoxUnitType obb_type)
      : spread_mode_(spread_mode), stops_(stops), obb_type_(obb_type) {
    std::copy(std::begin(form), std::end(form), gradient_transformer_);
  }
  size_t stop_size() const { return stops_.size(); }

 public:
  GradientSpread spread_mode_{pad};
  float gradient_transformer_[6] = {1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  std::vector<SrStop> stops_{};
  SrSVGObjectBoundingBoxUnitType obb_type_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
};

class LinearGradientModel : public GradientModel {
 public:
  LinearGradientModel() = default;
  LinearGradientModel(const GradientSpread spread_mode, float x1, float x2,
                      float y1, float y2, const float (&form)[6],
                      const std::vector<SrStop>& stops,
                      SrSVGObjectBoundingBoxUnitType obb_type)
      : GradientModel(spread_mode, form, stops, obb_type),
        x1_(x1),
        x2_(x2),
        y1_(y1),
        y2_(y2) {}

  float x1_{0.f};
  float x2_{0.f};
  float y1_{0.f};
  float y2_{0.f};
};

class RadialGradientModel : public GradientModel {
 public:
  RadialGradientModel() = default;
  RadialGradientModel(const GradientSpread spread_mode, float cx, float cy,
                      float r, float fx, float fy, const float (&form)[6],
                      const std::vector<SrStop>& stops,
                      SrSVGObjectBoundingBoxUnitType obb_type)
      : GradientModel(spread_mode, form, stops, obb_type),
        cx_(cx),
        cy_(cy),
        r_(r),
        fx_(fx),
        fy_(fy) {}

  float cx_{0.f};
  float cy_{0.f};
  float r_{0.f};
  float fx_{0.f};
  float fy_{0.f};
};

class SrCanvas {
 public:
  virtual ~SrCanvas() = default;
  virtual void SetRenderContext(const SrSVGRenderContext* context) {}
  virtual void SetViewBox(float x, float y, float width, float height) = 0;
  virtual void DrawRect(const char* id, float x, float y, float rx, float ry,
                        float width, float height,
                        const SrSVGRenderState& render_state) = 0;
  virtual void DrawCircle(const char* id, float cx, float cy, float r,
                          const SrSVGRenderState& render_state) = 0;
  virtual void DrawPolygon(const char* id, float points[], uint32_t n_points,
                           const SrSVGRenderState& render_state) = 0;
  virtual void DrawPolyline(const char* id, float points[], uint32_t n_points,
                            const SrSVGRenderState& render_state) = 0;
  virtual void DrawLine(const char* id, float start_x, float start_y,
                        float end_x, float end_y,
                        const SrSVGRenderState& render_state) = 0;
  virtual void DrawPath(const char* id, uint8_t ops[], uint32_t n_ops,
                        float args[], uint32_t n_args,
                        const SrSVGRenderState& render_state) = 0;
  virtual void DrawEllipse(const char* id, float center_x, float center_y,
                           float radius_x, float radius_y,
                           const SrSVGRenderState& render_state) = 0;
  virtual void UpdateLinearGradient(
      const char* id, const float (&form)[6], GradientSpread spread, float x1,
      float x2, float y1, float y2, const std::vector<SrStop>&,
      SrSVGObjectBoundingBoxUnitType obb_type) = 0;
  virtual void UpdateRadialGradient(
      const char* id, const float (&form)[6], GradientSpread spread, float cx,
      float cy, float fr, float fx, float fy, const std::vector<SrStop>&,
      SrSVGObjectBoundingBoxUnitType bounding_box_type) = 0;
  virtual void DrawUse(const char* href, float x, float y, float width,
                       float height) = 0;
  virtual void DrawImage(const char* url, float x, float y, float width,
                         float height,
                         const SrSVGPreserveAspectRatio& preserve_aspect_radio,
                         float opacity = 1.f) = 0;
  virtual void Translate(float x, float y) = 0;
  virtual void Transform(const float (&form)[6]) = 0;
  virtual void ClipPath(Path*, SrSVGFillRule clip_rule) = 0;
  virtual void Save() = 0;
  virtual void Restore() = 0;
  virtual bool SupportsFilters() const { return false; }
  virtual void SaveLayer(const SrSVGBox* bounds = nullptr) {
    (void)bounds;
    Save();
  }
  virtual void RestoreLayer() { Restore(); }
  virtual void BeginOpacityLayer(const SrSVGBox* bounds, float opacity) {
    (void)opacity;
    SaveLayer(bounds);
  }
  virtual void EndOpacityLayer() { RestoreLayer(); }
  virtual bool SupportsFilterModel(const SrFilterModel& filter) const {
    return false;
  }
  virtual void BeginFilterLayer(const SrSVGBox* bounds,
                                const SrFilterModel& filter) {
    (void)bounds;
    (void)filter;
    SaveLayer(bounds);
  }
  virtual void EndFilterLayer() { RestoreLayer(); }
  virtual void BeginMaskLayer(const SrSVGBox* bounds, bool is_luminance) {
    (void)is_luminance;
    SaveLayer(bounds);
  }
  virtual void BeginMaskContentLayer() {}
  virtual void EndMaskContentLayer() {}
  virtual void EndMaskLayer() { RestoreLayer(); }
  virtual PathFactory* PathFactory() = 0;
  SrCanvas() = default;
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_CANVAS_SRCANVAS_H_
