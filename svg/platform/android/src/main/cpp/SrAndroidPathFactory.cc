// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidPathFactory.h"

#include "platform/android/SrAndroidCanvas.h"
#include "platform/android/SrAndroidPath.h"
#include "platform/android/SrJNIUtils.h"
namespace serval {
namespace svg {
namespace android {

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateMutable() {
  LOGD("SrAndroidCanvas::CreateMutable");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }
  jmethodID j_make_mutable_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeMutablePath",
      "()"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeMutablePath_));
  if (j_make_mutable_path) {
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_mutable_path),
        this);
  }
  return nullptr;
}

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateCircle(float cx,
                                                                 float cy,
                                                                 float r) {
  LOGD("SrAndroidCanvas::drawCircle");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }
  jmethodID j_make_circle_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeCirclePath",
      "(FFF)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeCirclePath_));
  if (j_make_circle_path) {
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_circle_path, cx, cy, r),
        this);
  }
  return nullptr;
}

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateRect(
    float x, float y, float rx, float ry, float width, float height) {
  LOGD("SrAndroidCanvas::CreateRect");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }
  jmethodID j_make_rect_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeRectPath",
                "(FFFFFF)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeRectPath_));
  if (j_make_rect_path) {
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_rect_path, x, y, rx, ry, width,
                                         height),
        this);
  }
  return nullptr;
}

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateLine(float start_x,
                                                               float start_y,
                                                               float end_x,
                                                               float end_y) {
  LOGD("SrAndroidCanvas::CreateLine");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }

  jmethodID j_make_line_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeLinePath",
                "(FFFF)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makeRectPath_));
  if (j_make_line_path) {
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_line_path, start_x, start_y,
                                         end_x, end_y),
        this);
  }
  return nullptr;
};

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateEllipse(
    float center_x, float center_y, float radius_x, float radius_y) {

  LOGD("SrAndroidCanvas::CreateEllipse");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }

  jmethodID j_make_ellipse_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makeEllipsePath",
      "(FFFF)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makeEllipsePath_));
  if (j_make_ellipse_path) {
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_ellipse_path, center_x,
                                         center_y, radius_x, radius_y),
        this);
  }
  return nullptr;
}

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreatePolygon(
    float points[], uint32_t n_points) {
  LOGD("SrAndroidCanvas::CreatePolygon");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
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
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(),
                                         j_make_polygon_path, array_ref.Get()),
        this);
  }
  return nullptr;
};

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreatePolyline(
    float points[], uint32_t n_points) {
  LOGD("SrAndroidCanvas::CreatePolyline");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }

  jmethodID j_make_polygon_line_path = GetMethod(
      jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makePolyLinePath",
      "([F)"
      "Landroid/graphics/Path;",
      &(SrAndroidCanvas::g_SVGRenderEngine_makePolyLinePath_));
  if (j_make_polygon_line_path) {
    JavaLocalRef<jfloatArray> array_ref(jni_env_,
                                        jni_env_->NewFloatArray(n_points * 2));
    jni_env_->SetFloatArrayRegion(array_ref.Get(), 0, n_points * 2, points);
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(
            engine_clazz_ref.Get(), j_make_polygon_line_path, array_ref.Get()),
        this);
  }
  return nullptr;
};

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreatePath(
    uint8_t ops[], uint64_t n_ops, float args[], uint64_t n_args) {

  LOGD("SrAndroidCanvas::CreatePath");
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return nullptr;
  }

  jmethodID j_make_path =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "makePath",
                "([B[F)"
                "Landroid/graphics/Path;",
                &(SrAndroidCanvas::g_SVGRenderEngine_makePolyLinePath_));
  if (j_make_path) {
    JavaLocalRef<jbyteArray> ops_ref(jni_env_, jni_env_->NewByteArray(n_ops));
    jni_env_->SetByteArrayRegion(ops_ref.Get(), 0, n_ops,
                                 reinterpret_cast<const jbyte*>(ops));
    JavaLocalRef<jfloatArray> args_ref(jni_env_,
                                       jni_env_->NewFloatArray(n_args));
    jni_env_->SetFloatArrayRegion(args_ref.Get(), 0, n_args, args);
    return std::make_unique<SrAndroidPath>(
        jni_env_,
        jni_env_->CallStaticObjectMethod(engine_clazz_ref.Get(), j_make_path,
                                         ops_ref.Get(), args_ref.Get()),
        this);
  }
  return nullptr;
}

void SrAndroidPathFactory::Op(canvas::Path* path1, canvas::Path* path2,
                              canvas::OP op_type) {
  LOGD("SrAndroidCanvas::op");
  if (!path1 || !path2) {
    return;
  }
  jobject j_path_1 = static_cast<SrAndroidPath*>(path1)->GetJPath();
  jobject j_path_2 = static_cast<SrAndroidPath*>(path2)->GetJPath();
  if (!j_path_1 || !j_path_2) {
    return;
  }
  JavaLocalRef<jclass> engine_clazz_ref = GetClass(jni_env_, j_engine_);
  if (engine_clazz_ref.IsNull()) {
    return;
  }
  jmethodID j_op =
      GetMethod(jni_env_, engine_clazz_ref.Get(), STATIC_METHOD, "op",
                "(Landroid/graphics/Path;Landroid/graphics/Path;I)V",
                &(SrAndroidCanvas::g_SVGRenderEngine_op_));
  if (j_op) {
    jni_env_->CallStaticVoidMethod(engine_clazz_ref.Get(), j_op, j_path_1,
                                   j_path_2, op_type);
  }
}

SrSVGBox SrAndroidPathFactory::GetBounds(
    const JavaGlobalRef<jobject>& j_path_ref) const {
  LOGD("SrAndroidCanvas::GetBounds");
  SrSVGBox svg_box = {0.f, 0.f, 1.f, 1.f};
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull() || !j_path_ref.Get()) {
    return svg_box;
  }
  jmethodID j_calculate_path_bounds_array =
      GetMethod(jni_env_, render_clazz_ref.Get(), STATIC_METHOD,
                "calculatePathBoundsArray", "(Landroid/graphics/Path;)[F",
                &(SrAndroidCanvas::g_SVGRender_calculatePathBoundsArray_));
  if (j_calculate_path_bounds_array) {
    JavaLocalRef<jfloatArray> j_bounds_ref(
        jni_env_, static_cast<jfloatArray>(jni_env_->CallStaticObjectMethod(
                      render_clazz_ref.Get(), j_calculate_path_bounds_array,
                      j_path_ref.Get())));
    jfloatArray j_bounds = j_bounds_ref.Get();
    if (j_bounds && jni_env_->GetArrayLength(j_bounds) == 4) {
      std::vector<jfloat> bounds(4, 0.f);
      jni_env_->GetFloatArrayRegion(j_bounds, 0, 4, bounds.data());
      svg_box.left = bounds[0];
      svg_box.top = bounds[1];
      svg_box.width = bounds[2];
      svg_box.height = bounds[3];
    }
  }
  LOGD("SrAndroidCanvas::GetBounds: [%f, %f, %f, %f]", svg_box.left,
       svg_box.top, svg_box.width, svg_box.height);
  return svg_box;
}

std::unique_ptr<canvas::Path> SrAndroidPathFactory::CreateTransformCopy(
    const SrAndroidPath& path, const float (&xform)[6]) {
  ApplyTransform(path, xform);
  return std::make_unique<SrAndroidPath>(path);
}

void SrAndroidPathFactory::ApplyTransform(const SrAndroidPath& path,
                                          const float (&xform)[6]) {
  JavaLocalRef<jclass> render_clazz_ref = GetClass(jni_env_, j_render_);
  if (render_clazz_ref.IsNull() || !path.GetJPath()) {
    return;
  }
  jmethodID j_apply_transform =
      GetMethod(jni_env_, render_clazz_ref.Get(), STATIC_METHOD,
                "applyTransform", "(Landroid/graphics/Path;[F)V",
                &(SrAndroidCanvas::g_SVGRender_applyTransform_));
  if (j_apply_transform) {
    // make matrix
    JavaLocalRef<jfloatArray> j_transform_ref(jni_env_,
                                              jni_env_->NewFloatArray(6));
    jni_env_->SetFloatArrayRegion(j_transform_ref.Get(), 0, 6, xform);
    // apply transform
    jni_env_->CallStaticVoidMethod(render_clazz_ref.Get(), j_apply_transform,
                                   path.GetJPath(), j_transform_ref.Get());
  }
}

}  // namespace android
}  // namespace svg
}  // namespace serval

