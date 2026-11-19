// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "parser/SrSVGTraversalState.h"
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidCanvas.h"

#include <jni.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include "element/SrSVGNode.h"
#include "element/SrSVGPattern.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGPatternUtils.h"

#include "platform/android/SrAndroidPath.h"

namespace serval {
namespace svg {
namespace android {

namespace {

constexpr jint kAndroidFilterBlur = 0;
constexpr jint kAndroidFilterOffset = 1;
constexpr jint kAndroidFilterColorMatrix = 2;
constexpr jint kAndroidFilterLuminanceToAlpha = 3;

bool EncodeFilterModel(const canvas::SrFilterModel& filter, jint* operations,
                       jfloat* values, jsize& operation_count,
                       jsize& value_count) {

  for (const auto& primitive : filter.primitives) {
    switch (primitive.type) {
      case canvas::SrFilterPrimitiveType::kGaussianBlur:
        operations[operation_count++] = kAndroidFilterBlur;
        values[value_count++] = primitive.std_deviation_x;
        values[value_count++] = primitive.std_deviation_y;
        break;
      case canvas::SrFilterPrimitiveType::kOffset:
        operations[operation_count++] = kAndroidFilterOffset;
        values[value_count++] = primitive.dx;
        values[value_count++] = primitive.dy;
        break;
      case canvas::SrFilterPrimitiveType::kColorMatrix:
        if (primitive.color_matrix_type == "luminanceToAlpha") {
          operations[operation_count++] = kAndroidFilterLuminanceToAlpha;
        } else {
          operations[operation_count++] = kAndroidFilterColorMatrix;
          if (primitive.color_matrix_values.size() != 20)
            return false;
          for (float value : primitive.color_matrix_values)
            values[value_count++] = value;
        }
        break;
      case canvas::SrFilterPrimitiveType::kComposite:
      case canvas::SrFilterPrimitiveType::kBlend:
      case canvas::SrFilterPrimitiveType::kFlood:
        break;
    }
  }
  return true;
}

}  // namespace

static inline void CopyTransformArray(const std::array<float, 6>& src,
                                      float (&out)[6]) {
  for (size_t i = 0; i < src.size(); ++i) {
    out[i] = src[i];
  }
}

intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeFillPaintModel_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeStrokePaintModel_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeStopModel_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_op_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeMutablePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeRectPath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeCirclePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeLinePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeEllipsePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makePolygonPath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makePolyLinePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeStrokePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeTransformedStrokePath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_setFillType_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeLinearGradient_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeRadialGradient_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_setViewBox_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_save_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_restore_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_translate_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_transform_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_draw_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_draw_image_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_clipPath_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_clipRect_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_saveLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_restoreLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_beginOpacityLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_beginFilterLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_endFilterLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_beginMaskLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_beginMaskContentLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_endMaskContentLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_endMaskLayer_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_calculatePathBoundsArray_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_applyTransform_ = 0;

intptr_t SrAndroidCanvas::g_SVGRenderEngine_makeSpanStringBuilder_ = 0;
intptr_t SrAndroidCanvas::g_SVGRenderEngine_appendSpan_ = 0;
intptr_t SrAndroidCanvas::g_SVGRender_drawText_ = 0;

void SrAndroidCanvas::SetViewBox(float x, float y, float width, float height) {
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_set_view_box =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "setViewBox",
                "(FFFF)V", &(SrAndroidCanvas::g_SVGRender_setViewBox_));
  if (j_set_view_box) {
    jni_env_->CallVoidMethod(j_render_, j_set_view_box, x, y, width, height);
  }
}

void SrAndroidCanvas::Save() {
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_save =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "save", "()V",
                &(SrAndroidCanvas::g_SVGRender_save_));
  if (j_save) {
    jni_env_->CallVoidMethod(j_render_, j_save);
    transform_stack_.Save();
  }
}

void SrAndroidCanvas::Restore() {
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_restore =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "restore",
                "()V", &(SrAndroidCanvas::g_SVGRender_restore_));
  if (j_restore) {
    jni_env_->CallVoidMethod(j_render_, j_restore);
    transform_stack_.Restore(current_transform_);
  }
}

void SrAndroidCanvas::Translate(float x, float y) {
  float current[6];
  float out[6];
  CopyTransformArray(current_transform_, current);
  float translation[6];
  xform_set_translation(translation, x, y);
  if (!element::ComposeAffineTransform(current, translation, out))
    return;
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_translate =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "translate",
                "(FF)V", &(SrAndroidCanvas::g_SVGRender_translate_));
  if (j_translate && transform_stack_.BeforeTransform(current_transform_)) {
    jni_env_->CallVoidMethod(j_render_, j_translate, x, y);
    current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
  }
}

void SrAndroidCanvas::Transform(const float (&form)[6]) {
  float current[6];
  float out[6];
  CopyTransformArray(current_transform_, current);
  if (!element::ComposeAffineTransform(current, form, out))
    return;
  LOGD("SrAndroidCanvas::transform");
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_transform =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "transform",
                "([F)V", &(SrAndroidCanvas::g_SVGRender_transform_));
  if (j_transform) {
    // make matrix
    JavaLocalRef<jfloatArray> j_transform_ref(jni_env_,
                                              jni_env_->NewFloatArray(6));
    if (j_transform_ref.IsNull()) {
      if (jni_env_->ExceptionCheck())
        jni_env_->ExceptionClear();
      return;
    }
    jni_env_->SetFloatArrayRegion(j_transform_ref.Get(), 0, 6, form);
    if (!transform_stack_.BeforeTransform(current_transform_))
      return;
    // transform
    jni_env_->CallVoidMethod(j_render_, j_transform, j_transform_ref.Get());
    current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
  }
}

void SrAndroidCanvas::DrawRect(const char* id, float x, float y, float rx,
                               float ry, float width, float height,
                               const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawRect");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_rect_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeRectPath",
                "(FFFFFF)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeRectPath_));
  if (j_make_rect_path) {
    JavaLocalRef<jobject> path_ref(
        jni_env_, jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                                   j_make_rect_path, x, y, rx,
                                                   ry, width, height));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawCircle(const char* id, float cx, float cy, float r,
                                 const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawCircle");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_circle_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeCirclePath",
      "(FFF)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeCirclePath_));
  if (j_make_circle_path) {
    JavaLocalRef<jobject> path_ref(
        jni_env_, jni_env_->CallStaticObjectMethod(
                      engine_clazz_ref.Get(), j_make_circle_path, cx, cy, r));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawLine(const char* id, float start_x, float start_y,
                               float end_x, float end_y,
                               const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawLine");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_line_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeLinePath",
                "(FFFF)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeLinePath_));
  if (j_make_line_path) {
    JavaLocalRef<jobject> path_ref(
        jni_env_, jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                                   j_make_line_path, start_x,
                                                   start_y, end_x, end_y));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawEllipse(const char* id, float center_x,
                                  float center_y, float radius_x,
                                  float radius_y,
                                  const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawEllipse");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_ellipse_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeEllipsePath",
      "(FFFF)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeEllipsePath_));
  if (j_make_ellipse_path) {
    JavaLocalRef<jobject> path_ref(
        jni_env_, jni_env_->CallStaticObjectMethod(
                      engine_clazz_ref.Get(), j_make_ellipse_path, center_x,
                      center_y, radius_x, radius_y));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawPolygon(const char* id, float points[],
                                  uint32_t n_points,
                                  const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawPolygon");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_polygon_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makePolygonPath",
      "([F)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makePolygonPath_));
  if (j_make_polygon_path) {
    JavaLocalRef<jfloatArray> array_ref(jni_env_,
                                        jni_env_->NewFloatArray(n_points * 2));
    jni_env_->SetFloatArrayRegion(array_ref.Get(), 0, n_points * 2, points);
    JavaLocalRef<jobject> path_ref(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_polygon_path, array_ref.Get()));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawPolyline(const char* id, float points[],
                                   uint32_t n_points,
                                   const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawPolyline");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_polyline_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makePolyLinePath",
      "([F)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makePolyLinePath_));
  if (j_make_polyline_path) {
    JavaLocalRef<jfloatArray> array_ref(jni_env_,
                                        jni_env_->NewFloatArray(n_points * 2));
    jni_env_->SetFloatArrayRegion(array_ref.Get(), 0, n_points * 2, points);
    JavaLocalRef<jobject> path_ref(
        jni_env_,
        jni_env_->CallStaticObjectMethod(
            engine_clazz_ref.Get(), j_make_polyline_path, array_ref.Get()));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::DrawPath(const char* id, uint8_t ops[], uint32_t n_ops,
                               float args[], uint32_t n_args,
                               const SrSVGRenderState& render_state) {
  LOGD("SrAndroidCanvas::drawPath");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_make_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makePath",
                "([B[F)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makePath_));
  if (j_make_path) {
    JavaLocalRef<jbyteArray> ops_ref(jni_env_, jni_env_->NewByteArray(n_ops));
    jni_env_->SetByteArrayRegion(ops_ref.Get(), 0, n_ops,
                                 reinterpret_cast<const jbyte*>(ops));
    JavaLocalRef<jfloatArray> args_ref(jni_env_,
                                       jni_env_->NewFloatArray(n_args));
    jni_env_->SetFloatArrayRegion(args_ref.Get(), 0, n_args, args);
    JavaLocalRef<jobject> path_ref(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(), j_make_path,
                                         ops_ref.Get(), args_ref.Get()));
    Draw(path_ref, render_state);
  }
}

void SrAndroidCanvas::UpdateLinearGradient(
    const char* id, const float (&gradient_transform)[6], GradientSpread spread,
    float x1, float x2, float y1, float y2,
    const std::vector<SrStop>& vector_model,
    SrSVGObjectBoundingBoxUnitType box_type) {
  LOGD("SrAndroidCanvas::UpdateLinearGradient");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  JavaLocalRef<jclass> stop_model_clazz_ref =
      GetClass(jni_env_, "com/lynx/serval/svg/model/StopModel");
  if (engine_clazz_ref.IsNull() || render_clazz_ref.IsNull() ||
      stop_model_clazz_ref.IsNull()) {
    return;
  }
  std::vector<JavaLocalRef<jobject>> stops;
  for (auto& sr_stop : vector_model) {
    stops.emplace_back(MakeStopModel(sr_stop));
  }
  jmethodID j_make_linear_gradient = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeLinearGradient",
      "(Lcom/lynx/serval/svg/SVGRender;"
      "Ljava/lang/String;"
      "[FIFFFFI"
      "[Lcom/lynx/serval/svg/model/StopModel;"
      ")V",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeLinearGradient_));
  if (j_make_linear_gradient) {
    JavaLocalRef<jstring> j_id_ref = {jni_env_, jni_env_->NewStringUTF(id)};
    JavaLocalRef<jfloatArray> j_transform_ref = {jni_env_,
                                                 jni_env_->NewFloatArray(6)};
    jni_env_->SetFloatArrayRegion(j_transform_ref.Get(), 0, 6,
                                  gradient_transform);

    JavaLocalRef<jobjectArray> j_stop_array_ref = {
        jni_env_,
        jni_env_->NewObjectArray(vector_model.size(),
                                 stop_model_clazz_ref.Get(), nullptr)};
    for (size_t i = 0; i < stops.size(); ++i) {
      const auto& j_stop_ref = stops[i];
      jni_env_->SetObjectArrayElement(j_stop_array_ref.Get(), i,
                                      j_stop_ref.Get());
    }
    jni_env_->CallStaticVoidMethod(
        engine_clazz_ref.Get(), j_make_linear_gradient, j_render_,
        j_id_ref.Get(), j_transform_ref.Get(), static_cast<int>(spread), x1, x2,
        y1, y2, box_type, j_stop_array_ref.Get());
  }
}

void SrAndroidCanvas::UpdateRadialGradient(
    const char* id, const float (&gradient_transform)[6],
    const GradientSpread spread, float cx, float cy, float fr, float fx,
    float fy, const std::vector<SrStop>& vector_model,
    SrSVGObjectBoundingBoxUnitType bounding_box_type) {
  LOGD("SrAndroidCanvas::UpdateRadialGradient");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  JavaLocalRef<jclass> stop_model_clazz_ref =
      GetClass(jni_env_, "com/lynx/serval/svg/model/StopModel");
  if (engine_clazz_ref.IsNull() || render_clazz_ref.IsNull() ||
      stop_model_clazz_ref.IsNull()) {
    return;
  }
  std::vector<JavaLocalRef<jobject>> stops;
  for (auto& sr_stop : vector_model) {
    stops.emplace_back(MakeStopModel(sr_stop));
  }
  jmethodID j_make_radial_gradient = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeRadialGradient",
      "(Lcom/lynx/serval/svg/SVGRender;"
      "Ljava/lang/String;"
      "[FIFFFFFI"
      "[Lcom/lynx/serval/svg/model/StopModel;"
      ")V",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeRadialGradient_));
  if (j_make_radial_gradient) {
    JavaLocalRef<jstring> j_id_ref = {jni_env_, jni_env_->NewStringUTF(id)};

    // make matrix

    JavaLocalRef<jfloatArray> j_transform_ref = {jni_env_,
                                                 jni_env_->NewFloatArray(6)};

    jni_env_->SetFloatArrayRegion(j_transform_ref.Get(), 0, 6,
                                  gradient_transform);

    JavaLocalRef<jobjectArray> j_stop_array_ref = {
        jni_env_,
        jni_env_->NewObjectArray(vector_model.size(),
                                 stop_model_clazz_ref.Get(), nullptr)};
    for (size_t i = 0; i < stops.size(); ++i) {
      const auto& j_stop_ref = stops[i];
      jni_env_->SetObjectArrayElement(j_stop_array_ref.Get(), i,
                                      j_stop_ref.Get());
    }
    jni_env_->CallStaticVoidMethod(
        engine_clazz_ref.Get(), j_make_radial_gradient, j_render_,
        j_id_ref.Get(), j_transform_ref.Get(), static_cast<int>(spread), cx, cy,
        fr, fx, fy, bounding_box_type, j_stop_array_ref.Get());
  }
}

bool SrAndroidCanvas::CalculatePathBounds(JavaLocalRef<jobject>& path_ref,
                                          SrSVGBox* bounds) {
  if (!bounds || path_ref.IsNull()) {
    return false;
  }
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return false;
  }
  jmethodID j_calculate_path_bounds =
      GetMethod(jni_env_, render_clazz_ref.Get(), STATIC_METHOD,
                "calculatePathBoundsArray", "(Landroid/graphics/Path;)[F",
                &(SrAndroidCanvas::g_SVGRender_calculatePathBoundsArray_));
  if (!j_calculate_path_bounds) {
    return false;
  }
  JavaLocalRef<jfloatArray> j_bounds_ref(
      jni_env_,
      static_cast<jfloatArray>(jni_env_->CallStaticObjectMethod(
          render_clazz_ref.Get(), j_calculate_path_bounds, path_ref.Get())));
  if (j_bounds_ref.IsNull() ||
      jni_env_->GetArrayLength(j_bounds_ref.Get()) < 4) {
    return false;
  }
  float raw_bounds[4] = {0.f, 0.f, 0.f, 0.f};
  jni_env_->GetFloatArrayRegion(j_bounds_ref.Get(), 0, 4, raw_bounds);
  bounds->left = raw_bounds[0];
  bounds->top = raw_bounds[1];
  bounds->width = raw_bounds[2];
  bounds->height = raw_bounds[3];
  return true;
}

void SrAndroidCanvas::RenderPatternTiles(
    JavaLocalRef<jobject>& clip_path_ref,
    const element::ResolvedPattern& resolved_pattern,
    const SrSVGBox& target_bounds) {
  (void)clip_path_ref;

  SrSVGBox pattern_area = target_bounds;
  if (!element::IsIdentityTransform(resolved_pattern.pattern_transform)) {
    float inverse[6];
    if (!element::InvertAffineTransform(resolved_pattern.pattern_transform,
                                        inverse) ||
        !element::TryMapBounds(target_bounds, inverse, &pattern_area))
      return;
    Transform(resolved_pattern.pattern_transform);
  }

  float origin_x =
      resolved_pattern.x + std::floor((pattern_area.left - resolved_pattern.x) /
                                      resolved_pattern.width) *
                               resolved_pattern.width;
  float origin_y =
      resolved_pattern.y + std::floor((pattern_area.top - resolved_pattern.y) /
                                      resolved_pattern.height) *
                               resolved_pattern.height;
  float right = pattern_area.left + pattern_area.width;
  float bottom = pattern_area.top + pattern_area.height;
  element::PatternTileBudget budget(
      *current_render_context_, origin_x, origin_y, right, bottom,
      resolved_pattern.width, resolved_pattern.height);
  if (!budget.HasRemaining() ||
      !active_pattern_ids_.insert(resolved_pattern.id).second)
    return;
  const bool has_resolved_view_box =
      resolved_pattern.has_view_box &&
      FloatsLarger(resolved_pattern.view_box.width, 0.f) &&
      FloatsLarger(resolved_pattern.view_box.height, 0.f);
  const bool uses_object_bounding_box_content_units =
      resolved_pattern.pattern_content_units ==
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
  SrSVGRenderContext base_tile_context = *current_render_context_;
  // A pattern changes the content transform, but does not establish an SVG
  // viewport. Keep the caller's viewport/viewBox for percentage lengths.

  // TODO: optimize this tile loop further.
  for (float step_y = origin_y; step_y < bottom && budget.HasRemaining();
       step_y += resolved_pattern.height) {
    for (float step_x = origin_x; step_x < right && budget.HasRemaining();
         step_x += resolved_pattern.width) {
      budget.Consume();
      Save();
      ClipRect(step_x, step_y, step_x + resolved_pattern.width,
               step_y + resolved_pattern.height);

      SrSVGRenderContext tile_context = base_tile_context;
      if (has_resolved_view_box) {
        SrSVGBox tile_view_port{step_x, step_y, resolved_pattern.width,
                                resolved_pattern.height};
        float view_box_xform[6];
        calculate_view_box_transform(
            &tile_view_port, &resolved_pattern.view_box,
            resolved_pattern.preserve_aspect_ratio, view_box_xform);
        Transform(view_box_xform);
      } else {
        if (uses_object_bounding_box_content_units) {
          Translate(step_x, step_y);
          float scale_xform[6];
          xform_set_scale(scale_xform, target_bounds.width,
                          target_bounds.height);
          Transform(scale_xform);
        } else {
          Translate(step_x, step_y);
        }
      }

      auto* previous_render_context = current_render_context_;
      resolved_pattern.content_pattern->RenderContent(this, tile_context);
      current_render_context_ = previous_render_context;
      Restore();
    }
  }

  active_pattern_ids_.erase(resolved_pattern.id);
}

bool SrAndroidCanvas::RenderPatternFill(JavaLocalRef<jobject>& path_ref,
                                        const SrSVGRenderState& render_state,
                                        const char* iri) {
  if (!current_render_context_ || !iri || iri[0] != '#' ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
  if (!CalculatePathBounds(path_ref, &bounds)) {
    return false;
  }

  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_, bounds,
                                      active_pattern_ids_, &resolved_pattern)) {
    return false;
  }

  Save();
  SrAndroidPath clip_path(jni_env_, path_ref.Get(), path_factory_.get());
  ClipPath(&clip_path, render_state.fill_rule);
  RenderPatternTiles(path_ref, resolved_pattern, bounds);
  Restore();
  return true;
}

bool SrAndroidCanvas::RenderPatternStroke(JavaLocalRef<jobject>& path_ref,
                                          const SrSVGRenderState& render_state,
                                          const char* iri) {
  if (!current_render_context_ || !iri || iri[0] != '#' ||
      !render_state.stroke || !FloatsLarger(render_state.stroke_width, 0.f) ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  SRSVGStrokeState* stroke_state = render_state.stroke_state;
  SrSVGBox object_bounds{0.f, 0.f, 0.f, 0.f};
  if (!CalculatePathBounds(path_ref, &object_bounds)) {
    return false;
  }

  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_,
                                      object_bounds, active_pattern_ids_,
                                      &resolved_pattern)) {
    return false;
  }

  SrSVGStrokeCap stroke_line_cap = SR_SVG_STROKE_CAP_BUTT;
  SrSVGStrokeJoin stroke_line_join = SR_SVG_STROKE_JOIN_MITER;
  float stroke_miter_limit = element::SrSVGNode::s_stroke_miter_limit;
  if (stroke_state) {
    stroke_line_cap = stroke_state->stroke_line_cap;
    stroke_line_join = stroke_state->stroke_line_join;
    stroke_miter_limit = stroke_state->stroke_miter_limit;
  }

  SrAndroidPath source_path(jni_env_, path_ref.Get(), path_factory_.get());
  float stroke_dash_offset = 0.f;
  float* dash_array = nullptr;
  size_t dash_array_length = 0;
  if (stroke_state) {
    stroke_dash_offset = stroke_state->stroke_dash_offset;
    dash_array = stroke_state->dash_array;
    dash_array_length = stroke_state->dash_array_length;
  }
  float current[6];
  float inverse[6];
  bool use_non_scaling_stroke = false;
  if (render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
    CopyTransformArray(current_transform_, current);
    use_non_scaling_stroke = element::InvertAffineTransform(current, inverse);
  }

  std::unique_ptr<canvas::Path> stroke_clip_path;
  if (use_non_scaling_stroke) {
    stroke_clip_path = path_factory_->CreateTransformedStrokePath(
        &source_path, current, render_state.stroke_width, stroke_line_cap,
        stroke_line_join, stroke_miter_limit, stroke_dash_offset, dash_array,
        dash_array_length);
  } else {
    stroke_clip_path = path_factory_->CreateStrokePath(
        &source_path, render_state.stroke_width, stroke_line_cap,
        stroke_line_join, stroke_miter_limit, stroke_dash_offset, dash_array,
        dash_array_length);
  }
  if (!stroke_clip_path) {
    return false;
  }

  SrSVGBox pattern_bounds = stroke_clip_path->GetBounds();
  if (use_non_scaling_stroke) {
    pattern_bounds = element::MapBounds(pattern_bounds, inverse);
  }

  Save();
  if (use_non_scaling_stroke) {
    Transform(inverse);
  }
  ClipPath(stroke_clip_path.get(), SR_SVG_FILL);
  if (use_non_scaling_stroke) {
    Transform(current);
  }
  RenderPatternTiles(path_ref, resolved_pattern, pattern_bounds);
  Restore();
  return true;
}

void SrAndroidCanvas::Draw(JavaLocalRef<jobject>& path_ref,
                           const SrSVGRenderState& render_state) {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_draw =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "draw",
                "(Landroid/graphics/Path;"
                "Lcom/lynx/serval/svg/model/FillPaintModel;"
                "Lcom/lynx/serval/svg/model/StrokePaintModel;"
                ")V",
                &(SrAndroidCanvas::g_SVGRender_draw_));
  if (j_draw) {
    const bool rendered_pattern_fill =
        render_state.fill && render_state.fill->type == SERVAL_PAINT_IRI &&
        RenderPatternFill(path_ref, render_state,
                          render_state.fill->content.iri);
    const bool rendered_pattern_stroke =
        render_state.stroke && render_state.stroke->type == SERVAL_PAINT_IRI &&
        FloatsLarger(render_state.stroke_width, 0.f) &&
        element::IsPatternIri(render_state.stroke->content.iri,
                              *current_render_context_);

    JavaLocalRef<jobject> fill_paint_ref(jni_env_, nullptr);
    if (rendered_pattern_fill) {
      JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
      if (!engine_clazz_ref.IsNull()) {
        jmethodID j_make_fill_none = GetMethod(
            jni_env_, engine_clazz_ref.Get(), STATIC_METHOD,
            "makeFillPaintModel",
            "(ILjava/lang/String;JIF)"
            "Lcom/lynx/serval/svg/model/FillPaintModel;",
            &(SrAndroidCanvas::g_SVGRenderEngine_makeFillPaintModel_));
        if (j_make_fill_none) {
          JavaLocalRef<jstring> j_empty_iri(jni_env_,
                                            jni_env_->NewStringUTF(""));
          fill_paint_ref = {
              jni_env_, jni_env_->CallStaticObjectMethod(
                            engine_clazz_ref.Get(), j_make_fill_none,
                            static_cast<jint>(SERVAL_PAINT_NONE),
                            j_empty_iri.Get(), static_cast<jlong>(0),
                            static_cast<jint>(render_state.fill_rule),
                            static_cast<jfloat>(render_state.fill_opacity))};
        }
      }
    } else {
      // make fill paint
      LOGD("draw: makeFillPaint");
      fill_paint_ref = MakeFillPaint(path_ref, render_state);
    }
    // make stroke paint
    LOGD("draw: makeStrokePaint");
    JavaLocalRef<jobject> stroke_paint_ref(jni_env_, nullptr);
    if (!rendered_pattern_stroke) {
      stroke_paint_ref = MakeStrokePaint(path_ref, render_state);
    }
    // invoke draw
    LOGD("draw: invoke");
    jni_env_->CallVoidMethod(j_render_, j_draw, path_ref.Get(),
                             fill_paint_ref.Get(), stroke_paint_ref.Get());
    if (rendered_pattern_stroke) {
      RenderPatternStroke(path_ref, render_state,
                          render_state.stroke->content.iri);
    }
  }
}

void SrAndroidCanvas::DrawImage(
    const char* href, float x, float y, float width, float height,
    const SrSVGPreserveAspectRatio& preserve_aspect_radio, float opacity) {
  LOGD("SrAndroidCanvas::drawImage");
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_draw_image =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "drawImage",
                "(Ljava/lang/String;FFFFIIIF)V",
                &(SrAndroidCanvas::g_SVGRender_draw_image_));
  if (j_draw_image) {
    JavaLocalRef<jstring> j_string_ref(
        jni_env_, jni_env_->NewStringUTF(href != nullptr ? href : ""));
    jni_env_->CallVoidMethod(j_render_, j_draw_image, j_string_ref.Get(), x, y,
                             width, height, preserve_aspect_radio.align_x,
                             preserve_aspect_radio.align_y,
                             preserve_aspect_radio.scale, opacity);
  }
}

JavaLocalRef<jobject> SrAndroidCanvas::MakeFillPaint(
    JavaLocalRef<jobject>& path_ref, const SrSVGRenderState& render_state) {
  JavaLocalRef<jclass> clazz_ref = GetClass(jni_env_, j_engine_);
  if (clazz_ref.IsNull() || !render_state.fill) {
    return {jni_env_, nullptr};
  }
  jmethodID j_method =
      GetMethod(jni_env_, clazz_ref.Get(), STATIC_METHOD, "makeFillPaintModel",
                "(ILjava/lang/String;JIF)"
                "Lcom/lynx/serval/svg/model/FillPaintModel;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeFillPaintModel_));
  if (j_method) {
    jint j_fill_type = render_state.fill->type;
    jint j_fill_rule = static_cast<int>(render_state.fill_rule);
    jfloat j_opacity = render_state.fill_opacity;
    jlong j_color = 0;
    JavaLocalRef<jstring> j_iri_ref(jni_env_, jni_env_->NewStringUTF(""));
    if (render_state.fill->type == SERVAL_PAINT_COLOR) {
      j_color = render_state.fill->content.color.color;
    } else if (render_state.fill->type == SERVAL_PAINT_IRI) {
      const char* iri = render_state.fill->content.iri;
      if (iri == nullptr || iri[0] == '\0') {
        j_fill_type = SERVAL_PAINT_NONE;
        iri = "";
      }
      j_iri_ref = JavaLocalRef<jstring>(
          jni_env_, jni_env_->NewStringUTF(iri != nullptr ? iri : ""));
    }
    return {jni_env_, jni_env_->CallStaticObjectMethod(
                          clazz_ref.Get(), j_method, j_fill_type,
                          j_iri_ref.Get(), j_color, j_fill_rule, j_opacity)};
  }
  return {jni_env_, nullptr};
}

void SrAndroidCanvas::DrawUse(const char* href, float x, float y, float width,
                              float height) {}

JavaLocalRef<jobject> SrAndroidCanvas::MakeStrokePaint(
    JavaLocalRef<jobject>& path_ref, const SrSVGRenderState& render_state) {
  JavaLocalRef<jclass> clazz_ref = GetClass(jni_env_, j_engine_);
  if (clazz_ref.IsNull() || !render_state.stroke) {
    return {jni_env_, nullptr};
  }
  jmethodID j_method = GetMethod(
      jni_env_, clazz_ref.Get(), STATIC_METHOD, "makeStrokePaintModel",
      "(ILjava/lang/String;JFFIIFF[FI)"
      "Lcom/lynx/serval/svg/model/StrokePaintModel;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeStrokePaintModel_));
  if (j_method) {
    jint j_stroke_type = render_state.stroke->type;
    jfloat j_width = render_state.stroke_width;
    jfloat j_opacity = render_state.stroke_opacity;
    jint j_vector_effect = static_cast<jint>(render_state.vector_effect);
    jlong j_color = 0;
    SrSVGStrokeCap stroke_line_cap{SR_SVG_STROKE_CAP_BUTT};
    SrSVGStrokeJoin stroke_line_join{SR_SVG_STROKE_JOIN_MITER};
    float stroke_miter_limit{element::SrSVGNode::s_stroke_miter_limit};
    float stroke_dash_offset{0};
    size_t dash_array_length = 0;
    float* dash_array{nullptr};

    SRSVGStrokeState* stroke_state = render_state.stroke_state;
    if (stroke_state) {
      stroke_dash_offset = stroke_state->stroke_dash_offset;
      stroke_line_cap = stroke_state->stroke_line_cap;
      stroke_miter_limit = stroke_state->stroke_miter_limit;
      stroke_line_join = stroke_state->stroke_line_join;
      dash_array_length = stroke_state->dash_array_length;
      dash_array = stroke_state->dash_array;
    }

    JavaLocalRef<jstring> j_iri_ref(jni_env_, jni_env_->NewStringUTF(""));
    if (render_state.stroke->type == SERVAL_PAINT_COLOR) {
      j_color = render_state.stroke->content.color.color;
    } else if (render_state.stroke->type == SERVAL_PAINT_IRI) {
      const char* iri = render_state.stroke->content.iri;
      if (iri == nullptr || iri[0] == '\0') {
        j_stroke_type = SERVAL_PAINT_NONE;
        iri = "";
      }
      j_iri_ref = JavaLocalRef<jstring>(
          jni_env_, jni_env_->NewStringUTF(iri != nullptr ? iri : ""));
    }
    JavaLocalRef<jfloatArray> array_ref(
        jni_env_, jni_env_->NewFloatArray(dash_array_length));
    jni_env_->SetFloatArrayRegion(array_ref.Get(), 0, dash_array_length,
                                  dash_array);
    return {jni_env_,
            jni_env_->CallStaticObjectMethod(
                clazz_ref.Get(), j_method, j_stroke_type, j_iri_ref.Get(),
                j_color, j_width, j_opacity, static_cast<int>(stroke_line_cap),
                static_cast<int>(stroke_line_join), stroke_miter_limit,
                stroke_dash_offset, array_ref.Get(), j_vector_effect)};
  }
  return {jni_env_, nullptr};
}

JavaLocalRef<jobject> SrAndroidCanvas::MakeStopModel(const SrStop& stop) {
  JavaLocalRef<jclass> clazz_ref = GetClass(jni_env_, j_engine_);
  if (clazz_ref.IsNull()) {
    return {jni_env_, nullptr};
  }
  jmethodID j_make_stop_model =
      GetMethod(jni_env_, clazz_ref.Get(), STATIC_METHOD, "makeStopModel",
                "(FJF)"
                "Lcom/lynx/serval/svg/model/StopModel;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeStopModel_));
  if (j_make_stop_model) {
    jfloat j_offset = stop.offset.value;
    jlong j_color = stop.stopColor.color;
    jfloat j_opacity = stop.stopOpacity.value;
    return {jni_env_,
            jni_env_->CallStaticObjectMethod(clazz_ref.Get(), j_make_stop_model,
                                             j_offset, j_color, j_opacity)};
  }
  return {jni_env_, nullptr};
}

void SrAndroidCanvas::ClipPath(canvas::Path* path, SrSVGFillRule clip_rule) {
  LOGD("SrAndroidCanvas::clipPath");
  if (!path) {
    return;
  }
  jobject j_path = static_cast<SrAndroidPath*>(path)->GetJPath();
  if (!j_path) {
    return;
  }
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_clip_path = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "clipPath",
      "(Landroid/graphics/Path;I)V", &(SrAndroidCanvas::g_SVGRender_clipPath_));
  if (j_clip_path) {
    jni_env_->CallVoidMethod(j_render_, j_clip_path, j_path,
                             static_cast<int>(clip_rule));
  }
}

void SrAndroidCanvas::ClipRect(float left, float top, float right,
                               float bottom) {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_clip_rect =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "clipRect",
                "(FFFF)V", &(SrAndroidCanvas::g_SVGRender_clipRect_));
  if (j_clip_rect) {
    jni_env_->CallVoidMethod(j_render_, j_clip_rect, left, top, right, bottom);
  }
}

bool SrAndroidCanvas::SupportsFilterModel(
    const canvas::SrFilterModel& filter) const {
  return canvas::SrSupportsLinearSourceGraphicFilterModel(filter);
}

void SrAndroidCanvas::SaveLayer(const SrSVGBox* bounds) {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_save_layer =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "saveLayer",
                "(FFFF)V", &(SrAndroidCanvas::g_SVGRender_saveLayer_));
  if (j_save_layer) {
    if (bounds) {
      jni_env_->CallVoidMethod(j_render_, j_save_layer, bounds->left,
                               bounds->top, bounds->left + bounds->width,
                               bounds->top + bounds->height);
    } else {
      jni_env_->CallVoidMethod(j_render_, j_save_layer, 0.f, 0.f, 0.f, 0.f);
    }
    transform_stack_.Save();
  }
}

void SrAndroidCanvas::RestoreLayer() {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_restore_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "restoreLayer", "()V",
      &(SrAndroidCanvas::g_SVGRender_restoreLayer_));
  if (j_restore_layer) {
    jni_env_->CallVoidMethod(j_render_, j_restore_layer);
    transform_stack_.Restore(current_transform_);
  }
}

void SrAndroidCanvas::BeginOpacityLayer(const SrSVGBox* bounds, float opacity) {
  if (!TryBeginOpacityLayer(bounds, opacity))
    ++opacity_fallback_depth_;
}

bool SrAndroidCanvas::TryBeginOpacityLayer(const SrSVGBox* bounds,
                                           float opacity) {
  if (opacity_fallback_depth_)
    return false;
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return false;
  }
  jmethodID j_begin_opacity_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "tryBeginOpacityLayer",
      "(FFFFF)Z", &(SrAndroidCanvas::g_SVGRender_beginOpacityLayer_));
  const SrSVGBox box = bounds ? *bounds : SrSVGBox{};
  if (!j_begin_opacity_layer)
    return false;
  return transform_stack_.TrySaveLayer(current_transform_, [&] {
    return jni_env_->CallBooleanMethod(j_render_, j_begin_opacity_layer,
                                       box.left, box.top, box.left + box.width,
                                       box.top + box.height, opacity);
  });
}

void SrAndroidCanvas::EndOpacityLayer() {
  if (opacity_fallback_depth_) {
    --opacity_fallback_depth_;
    return;
  }
  RestoreLayer();
}

void SrAndroidCanvas::BeginFilterLayer(const SrSVGBox* bounds,
                                       const canvas::SrFilterModel& filter) {
  if (!TryBeginFilterLayer(bounds, filter))
    ++filter_fallback_depth_;
}

bool SrAndroidCanvas::TryBeginFilterLayer(const SrSVGBox* bounds,
                                          const canvas::SrFilterModel& filter) {
  if (filter_fallback_depth_)
    return false;
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return false;
  }
  jmethodID j_begin_filter_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "tryBeginFilterLayer",
      "(FFFF[I[F)Z", &(SrAndroidCanvas::g_SVGRender_beginFilterLayer_));
  if (!j_begin_filter_layer) {
    return false;
  }

  constexpr size_t kMaxPrimitives =
      parser::SrSVGTraversalState::kMaxFilterPrimitives;
  if (filter.primitives.size() > kMaxPrimitives)
    return false;
  jint operations[kMaxPrimitives];
  jfloat values[20 * kMaxPrimitives];
  jsize operation_count = 0, value_count = 0;
  if (!EncodeFilterModel(filter, operations, values, operation_count,
                         value_count))
    return false;
  JavaLocalRef<jintArray> j_operations_ref(
      jni_env_, jni_env_->NewIntArray(operation_count));
  if (j_operations_ref.IsNull()) {
    if (jni_env_->ExceptionCheck())
      jni_env_->ExceptionClear();
    return false;
  }
  JavaLocalRef<jfloatArray> j_values_ref(jni_env_,
                                         jni_env_->NewFloatArray(value_count));
  if (j_values_ref.IsNull()) {
    if (jni_env_->ExceptionCheck())
      jni_env_->ExceptionClear();
    return false;
  }
  if (operation_count)
    jni_env_->SetIntArrayRegion(j_operations_ref.Get(), 0, operation_count,
                                operations);
  if (value_count)
    jni_env_->SetFloatArrayRegion(j_values_ref.Get(), 0, value_count, values);

  float left = 0.f;
  float top = 0.f;
  float right = 0.f;
  float bottom = 0.f;
  if (bounds) {
    left = bounds->left;
    top = bounds->top;
    right = bounds->left + bounds->width;
    bottom = bounds->top + bounds->height;
  }
  return transform_stack_.TrySaveLayer(current_transform_, [&] {
    return jni_env_->CallBooleanMethod(
        j_render_, j_begin_filter_layer, left, top, right, bottom,
        j_operations_ref.Get(), j_values_ref.Get());
  });
}

void SrAndroidCanvas::EndFilterLayer() {
  if (filter_fallback_depth_) {
    --filter_fallback_depth_;
    return;
  }
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_end_filter_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "endFilterLayer",
      "()V", &(SrAndroidCanvas::g_SVGRender_endFilterLayer_));
  if (j_end_filter_layer) {
    jni_env_->CallVoidMethod(j_render_, j_end_filter_layer);
    transform_stack_.Restore(current_transform_);
  }
}

void SrAndroidCanvas::BeginMaskLayer(const SrSVGBox* bounds,
                                     bool is_luminance) {
  if (!TryBeginMaskLayer(bounds, is_luminance))
    ++mask_fallback_depth_;
}

bool SrAndroidCanvas::TryBeginMaskLayer(const SrSVGBox* bounds,
                                        bool is_luminance) {
  if (mask_fallback_depth_)
    return false;
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return false;
  }
  jmethodID j_begin_mask_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "tryBeginMaskLayer",
      "(FFFFZ)Z", &(SrAndroidCanvas::g_SVGRender_beginMaskLayer_));
  const SrSVGBox box = bounds ? *bounds : SrSVGBox{};
  if (!j_begin_mask_layer)
    return false;
  return transform_stack_.TrySaveLayer(current_transform_, [&] {
    return jni_env_->CallBooleanMethod(
        j_render_, j_begin_mask_layer, box.left, box.top, box.left + box.width,
        box.top + box.height, static_cast<jboolean>(is_luminance));
  });
}

void SrAndroidCanvas::BeginMaskContentLayer() {
  TryBeginMaskContentLayer();
}

bool SrAndroidCanvas::TryBeginMaskContentLayer() {
  if (mask_fallback_depth_)
    return false;
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return false;
  }
  jmethodID j_begin_mask_content_layer =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD,
                "tryBeginMaskContentLayer", "()Z",
                &(SrAndroidCanvas::g_SVGRender_beginMaskContentLayer_));
  if (!j_begin_mask_content_layer)
    return false;
  return transform_stack_.TrySaveLayer(current_transform_, [&] {
    return jni_env_->CallBooleanMethod(j_render_, j_begin_mask_content_layer);
  });
}

void SrAndroidCanvas::EndMaskContentLayer() {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull())
    return;
  jmethodID j_end_mask_content_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "endMaskContentLayer",
      "()V", &(SrAndroidCanvas::g_SVGRender_endMaskContentLayer_));
  if (j_end_mask_content_layer) {
    jni_env_->CallVoidMethod(j_render_, j_end_mask_content_layer);
    transform_stack_.Restore(current_transform_);
  }
}

void SrAndroidCanvas::EndMaskLayer() {
  if (mask_fallback_depth_) {
    --mask_fallback_depth_;
    return;
  }
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_end_mask_layer = GetMethod(
      jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "endMaskLayer", "()V",
      &(SrAndroidCanvas::g_SVGRender_endMaskLayer_));
  if (j_end_mask_layer) {
    jni_env_->CallVoidMethod(j_render_, j_end_mask_layer);
    transform_stack_.Restore(current_transform_);
  }
}

}  // namespace android
}  // namespace svg
}  // namespace serval
