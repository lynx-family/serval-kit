// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATHFACTORY_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATHFACTORY_H_

#include <jni.h>
#include <memory>

#include "canvas/SrCanvas.h"
#include "platform/android/SrJNIUtils.h"

namespace serval {
namespace svg {
namespace android {

class SrAndroidPath;

class SrAndroidPathFactory : public canvas::PathFactory {
 public:
  SrAndroidPathFactory() {}
  std::unique_ptr<canvas::Path> CreateCircle(float cx, float cy,
                                             float r) override;
  std::unique_ptr<canvas::Path> CreateRect(float x, float y, float rx, float ry,
                                           float width, float height) override;
  std::unique_ptr<canvas::Path> CreateLine(float start_x, float start_y,
                                           float end_x, float end_y) override;
  std::unique_ptr<canvas::Path> CreateEllipse(float center_x, float center_y,
                                              float radius_x,
                                              float radius_y) override;
  std::unique_ptr<canvas::Path> CreatePolygon(float points[],
                                              uint32_t n_points) override;
  std::unique_ptr<canvas::Path> CreatePolyline(float points[],
                                               uint32_t n_points) override;
  std::unique_ptr<canvas::Path> CreatePath(uint8_t ops[], uint64_t n_ops,
                                           float args[],
                                           uint64_t n_args) override;

  std::unique_ptr<canvas::Path> CreateMutable() override;
  void Op(canvas::Path* path1, canvas::Path* path2,
          canvas::OP op_type) override;

  SrSVGBox GetBounds(const JavaGlobalRef<jobject>& j_path_ref) const;

  std::unique_ptr<canvas::Path> CreateTransformCopy(const SrAndroidPath& path,
                                                    const float (&xform)[6]);

  void ApplyTransform(const SrAndroidPath& path, const float (&xform)[6]);

  void SetJNIEnv(JNIEnv* jni_env) { jni_env_ = jni_env; }
  void SetJEngine(jobject j_engine) { j_engine_ = j_engine; }
  void SetJRender(jobject j_render) { j_render_ = j_render; }

 private:
  JNIEnv* jni_env_;
  jobject j_engine_;
  jobject j_render_;
};

}  // namespace android
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATHFACTORY_H_
