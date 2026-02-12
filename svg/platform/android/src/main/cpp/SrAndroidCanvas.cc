// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidCanvas.h"

#include <jni.h>
#include <vector>
#include "element/SrSVGNode.h"

#include "platform/android/SrAndroidPath.h"

namespace serval {
namespace svg {
namespace android {

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
  }
}

void SrAndroidCanvas::Translate(float x, float y) {
  JavaLocalRef<jclass> j_render_clazz = GetClass(jni_env_, j_render_);
  if (j_render_clazz.IsNull()) {
    return;
  }
  jmethodID j_translate =
      GetMethod(jni_env_, j_render_clazz.Get(), INSTANCE_METHOD, "translate",
                "(FF)V", &(SrAndroidCanvas::g_SVGRender_translate_));
  if (j_translate) {
    jni_env_->CallVoidMethod(j_render_, j_translate, x, y);
  }
}

void SrAndroidCanvas::Transform(const float (&form)[6]) {
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
    jni_env_->SetFloatArrayRegion(j_transform_ref.Get(), 0, 6, form);
    // transform
    jni_env_->CallVoidMethod(j_render_, j_transform, j_transform_ref.Get());
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
    // make fill paint
    LOGD("draw: makeFillPaint");
    JavaLocalRef<jobject> fill_paint_ref = MakeFillPaint(render_state);
    // make stroke paint
    LOGD("draw: makeStrokePaint");
    JavaLocalRef<jobject> stroke_paint_ref = MakeStrokePaint(render_state);
    // invoke draw
    LOGD("draw: invoke");
    jni_env_->CallVoidMethod(j_render_, j_draw, path_ref.Get(),
                             fill_paint_ref.Get(), stroke_paint_ref.Get());
  }
}

void SrAndroidCanvas::DrawImage(
    const char* href, float x, float y, float width, float height,
    const SrSVGPreserveAspectRatio& preserve_aspect_radio) {
  LOGD("SrAndroidCanvas::drawImage");
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_draw_image =
      GetMethod(jni_env_, render_clazz_ref.Get(), INSTANCE_METHOD, "drawImage",
                "(Ljava/lang/String;FFFFIII)V",
                &(SrAndroidCanvas::g_SVGRender_draw_image_));
  if (j_draw_image) {
    JavaLocalRef<jstring> j_string_ref(
        jni_env_, jni_env_->NewStringUTF(href != nullptr ? href : ""));
    jni_env_->CallVoidMethod(j_render_, j_draw_image, j_string_ref.Get(), x, y,
                             width, height, preserve_aspect_radio.align_x,
                             preserve_aspect_radio.align_y,
                             preserve_aspect_radio.scale);
  }
}

JavaLocalRef<jobject> SrAndroidCanvas::MakeFillPaint(
    const SrSVGRenderState& render_state) {
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
      j_iri_ref = JavaLocalRef<jstring>(
          jni_env_, jni_env_->NewStringUTF(render_state.fill->content.iri));
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
    const SrSVGRenderState& render_state) {
  JavaLocalRef<jclass> clazz_ref = GetClass(jni_env_, j_engine_);
  if (clazz_ref.IsNull() || !render_state.stroke) {
    return {jni_env_, nullptr};
  }
  jmethodID j_method = GetMethod(
      jni_env_, clazz_ref.Get(), STATIC_METHOD, "makeStrokePaintModel",
      "(ILjava/lang/String;JFFIIFF[F)"
      "Lcom/lynx/serval/svg/model/StrokePaintModel;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeStrokePaintModel_));
  if (j_method) {
    jint j_stroke_type = render_state.stroke->type;
    jfloat j_width = render_state.stroke_width;
    jfloat j_opacity = render_state.stroke_opacity;
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
      j_iri_ref = JavaLocalRef<jstring>(
          jni_env_, jni_env_->NewStringUTF(render_state.stroke->content.iri));
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
                stroke_dash_offset, array_ref.Get())};
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

}  // namespace android
}  // namespace svg
}  // namespace serval
