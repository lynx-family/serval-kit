// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_rect.h>
#include "canvas/SrCanvas.h"
#include "element/SrSVGPatternResolver.h"
#include "platform/harmony/path_factory_harmony_impl.h"

namespace serval {
namespace svg {
namespace harmony {

class SrHarmonyCanvas : public canvas::SrCanvas {
 public:
  struct ImageData {
    OH_Drawing_PixelMap* draw_pixel_map{nullptr};
    uint32_t width{0};
    uint32_t height{0};
  };
  using ImageProvider = std::function<const ImageData*(const std::string&)>;

  explicit SrHarmonyCanvas(OH_Drawing_Canvas* canvas);
  void SetRenderContext(const SrSVGRenderContext* context) override {
    current_render_context_ = context;
  }
  void Reset(OH_Drawing_Canvas* canvas);
  canvas::PathFactory* PathFactory() override;
  ~SrHarmonyCanvas() override;
  void Save() override;
  void Restore() override;
  void SetAntiAlias(bool anti_alias);
  void SetImageProvider(ImageProvider provider) {
    image_provider_ = std::move(provider);
  }
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
  inline OH_Drawing_Canvas* Context() { return context_; }

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
  void SaveLayer(const SrSVGBox* bounds = nullptr) override;
  void RestoreLayer() override;
  void SetBlendMode(canvas::SrCanvasBlendMode blend_mode) override;
  void SetMaskIsLuminance(bool is_luminance) override;
  void ApplyLuminanceToAlpha() override;

 private:
  OH_Drawing_Canvas* context_{nullptr};
  OH_Drawing_Pen* pen_{nullptr};
  OH_Drawing_Brush* brush_{nullptr};
  OH_Drawing_ShaderEffect* shader_{nullptr};
  OH_Drawing_PathEffect* path_effect_{nullptr};
  std::unique_ptr<canvas::PathFactory> path_factory_{nullptr};
  std::unordered_map<std::string, canvas::LinearGradientModel> lg_models_;
  std::unordered_map<std::string, canvas::RadialGradientModel> rg_models_;
  bool anti_alias_{true};
  canvas::SrCanvasBlendMode blend_mode_{canvas::SrCanvasBlendMode::kSrcOver};
  bool mask_is_luminance_{false};
  OH_Drawing_SamplingOptions* image_sampling_{nullptr};
  std::array<float, 6> current_transform_{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  std::vector<std::array<float, 6>> transform_stack_;
  ImageProvider image_provider_{};

  void FillPath(OH_Drawing_Path* path, const SrSVGRenderState& render_state);
  void StrokePath(OH_Drawing_Path* path, const SrSVGRenderState& render_state);
  void DrawLinearGradientShader(OH_Drawing_Canvas* canvas,
                                const canvas::LinearGradientModel& lg_model,
                                OH_Drawing_Path* path,
                                const SrSVGRenderState& render_state,
                                bool is_stroke,
                                OH_Drawing_Path* bounds_path = nullptr,
                                const float* extra_transform = nullptr);
  void DrawRadialGradientShader(OH_Drawing_Canvas* canvas,
                                const canvas::RadialGradientModel& rg_model,
                                OH_Drawing_Path* path,
                                const SrSVGRenderState& render_state,
                                bool is_stroke,
                                OH_Drawing_Path* bounds_path = nullptr,
                                const float* extra_transform = nullptr);

  void InitStrokePaint(const SrSVGRenderState& render_state, bool anti_alias);

  void InitFillPaint(const SrSVGRenderState& render_state, bool anti_alias);
  bool CalculatePathBounds(OH_Drawing_Path* path, SrSVGBox* bounds);
  void RenderPatternTiles(const element::ResolvedPattern& resolved_pattern,
                          const SrSVGBox& target_bounds);
  bool RenderPatternFill(OH_Drawing_Path* path,
                         const SrSVGRenderState& render_state, const char* iri);
  bool RenderPatternStroke(OH_Drawing_Path* path,
                           const SrSVGRenderState& render_state,
                           const char* iri);
  void ClipRect(float left, float top, float right, float bottom);

  const SrSVGRenderContext* current_render_context_{nullptr};
  std::unordered_set<std::string> active_pattern_ids_;
};
}  // namespace harmony
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_
