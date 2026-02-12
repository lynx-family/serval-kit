// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDCANVAS_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDCANVAS_H_

#include <memory>
#include <vector>

#include "canvas/SrCanvas.h"
#include "platform/android/SrAndroidPathFactory.h"
#include "platform/android/SrScopedJavaRef.h"

namespace serval {
namespace svg {
namespace android {

class SrAndroidCanvas : public canvas::SrCanvas {
 public:
  SrAndroidCanvas(JNIEnv* jni_env, jobject j_engine, jobject j_render)
      : SrCanvas(),
        jni_env_(jni_env),
        j_engine_(j_engine),
        j_render_(j_render) {
    path_factory_ = std::make_unique<SrAndroidPathFactory>();
    path_factory_->SetJNIEnv(jni_env);
    path_factory_->SetJEngine(j_engine);
    path_factory_->SetJRender(j_render);
  };
  ~SrAndroidCanvas() override = default;

  void SetViewBox(float x, float y, float width, float height) override;
  void Save() override;
  void Restore() override;
  void Translate(float x, float y) override;
  void Transform(const float (&form)[6]) override;
  void DrawRect(const char* id, float x, float y, float rx, float ry,
                float width, float height,
                const SrSVGRenderState& render_state) override;
  void DrawLine(const char* id, float start_x, float start_y, float end_x,
                float end_y, const SrSVGRenderState& render_state) override;
  void DrawCircle(const char* id, float cx, float cy, float r,
                  const SrSVGRenderState& render_state) override;
  void DrawEllipse(const char* id, float center_x, float center_y,
                   float radius_x, float radius_y,
                   const SrSVGRenderState& render_state) override;
  void DrawPolygon(const char* id, float points[], uint32_t n_points,
                   const SrSVGRenderState& render_state) override;
  void DrawPolyline(const char* id, float points[], uint32_t n_points,
                    const SrSVGRenderState& render_state) override;
  void DrawPath(const char* id, uint8_t ops[], uint32_t n_ops, float args[],
                uint32_t n_args, const SrSVGRenderState& render_state) override;

  void UpdateLinearGradient(const char* id, const float (&form)[6],
                            const GradientSpread spread, float x1, float x2,
                            float y1, float y2, const std::vector<SrStop>&,
                            SrSVGObjectBoundingBoxUnitType box_type) override;
  void UpdateRadialGradient(
      const char* id, const float (&form)[6], const GradientSpread spread,
      float cx, float cy, float fr, float fx, float fy,
      const std::vector<SrStop>&,
      SrSVGObjectBoundingBoxUnitType bounding_box_type) override;
  void DrawUse(const char* href, float x, float y, float width,
               float height) override;

  void DrawImage(
      const char* url, float x, float y, float width, float height,
      const SrSVGPreserveAspectRatio& preserve_aspect_radio) override;
  void ClipPath(canvas::Path* path, SrSVGFillRule clip_rule) override;
  canvas::PathFactory* PathFactory() override { return path_factory_.get(); }

 private:
  JavaLocalRef<jobject> MakeFillPaint(const SrSVGRenderState& render_state);
  JavaLocalRef<jobject> MakeStrokePaint(const SrSVGRenderState& render_state);
  JavaLocalRef<jobject> MakeStopModel(const SrStop& stop);
  void Draw(JavaLocalRef<jobject>& path_ref,
            const SrSVGRenderState& render_state);

 public:
  std::unique_ptr<SrAndroidPathFactory> path_factory_;
  static intptr_t g_SVGRenderEngine_makeFillPaintModel_;
  static intptr_t g_SVGRenderEngine_makeStrokePaintModel_;
  static intptr_t g_SVGRenderEngine_makeStopModel_;
  static intptr_t g_SVGRenderEngine_op_;
  static intptr_t g_SVGRenderEngine_makeMutablePath_;
  static intptr_t g_SVGRenderEngine_makeRectPath_;
  static intptr_t g_SVGRenderEngine_makeCirclePath_;
  static intptr_t g_SVGRenderEngine_makeLinePath_;
  static intptr_t g_SVGRenderEngine_makeEllipsePath_;
  static intptr_t g_SVGRenderEngine_makePolygonPath_;
  static intptr_t g_SVGRenderEngine_makePolyLinePath_;
  static intptr_t g_SVGRenderEngine_makePath_;
  static intptr_t g_SVGRenderEngine_makeStrokePath_;
  static intptr_t g_SVGRenderEngine_setFillType_;
  static intptr_t g_SVGRenderEngine_makeLinearGradient_;
  static intptr_t g_SVGRenderEngine_makeRadialGradient_;
  static intptr_t g_SVGRender_setViewBox_;
  static intptr_t g_SVGRender_save_;
  static intptr_t g_SVGRender_restore_;
  static intptr_t g_SVGRender_translate_;
  static intptr_t g_SVGRender_transform_;
  static intptr_t g_SVGRender_draw_;
  static intptr_t g_SVGRender_draw_image_;
  static intptr_t g_SVGRender_clipPath_;
  static intptr_t g_SVGRender_calculatePathBoundsArray_;
  static intptr_t g_SVGRender_applyTransform_;
  static intptr_t g_SVGRenderEngine_makeSpanStringBuilder_;
  static intptr_t g_SVGRenderEngine_appendSpan_;
  static intptr_t g_SVGRender_drawText_;

 private:
  jobject j_engine_;
  jobject j_render_;
  JNIEnv* jni_env_;

 public:
  JNIEnv* GetJNIEnv() const { return jni_env_; }
  jobject GetJEngine() const { return j_engine_; }
  jobject GetJRender() const { return j_render_; }
};

}  // namespace android
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDCANVAS_H_
