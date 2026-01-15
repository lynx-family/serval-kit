// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_
#define SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_rect.h>
#include "canvas/SrCanvas.h"
#include "platform/harmony/path_factory_harmony_impl.h"

namespace serval {
namespace svg {
namespace harmony {

class SrHarmonyCanvas : public canvas::SrCanvas {
 public:
  explicit SrHarmonyCanvas(OH_Drawing_Canvas* canvas);
  void Reset(OH_Drawing_Canvas* canvas);
  canvas::PathFactory* PathFactory() override;
  ~SrHarmonyCanvas() override;
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

  void FillPath(OH_Drawing_Path* path, const SrSVGRenderState& render_state);
  void StrokePath(OH_Drawing_Path* path, const SrSVGRenderState& render_state);
  void DrawLinearGradientShader(OH_Drawing_Canvas* canvas,
                                const canvas::LinearGradientModel& lg_model,
                                OH_Drawing_Path* path,
                                const SrSVGRenderState& render_state,
                                bool is_stroke);
  void DrawRadialGradientShader(OH_Drawing_Canvas* canvas,
                                const canvas::RadialGradientModel& rg_model,
                                OH_Drawing_Path* path,
                                const SrSVGRenderState& render_state,
                                bool is_stroke);

  void InitStrokePaint(const SrSVGRenderState& render_state, bool anti_alias);

  void InitFillPaint(const SrSVGRenderState& render_state, bool anti_alias);
};
}  // namespace harmony
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_HARMONY_SR_HARMONY_CANVAS_H_
