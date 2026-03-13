// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATH_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATH_H_

#include <jni.h>
#include <memory>
#include <utility>

#include "canvas/SrCanvas.h"
#include "platform/android/SrAndroidPathFactory.h"
#include "platform/android/SrJNIUtils.h"

namespace serval {
namespace svg {
namespace android {

class SrAndroidPath : public canvas::Path {
 public:
  SrAndroidPath(JNIEnv* jni_env, jobject j_path,
                SrAndroidPathFactory* path_factory)
      : canvas::Path(),
        j_path_ref_(jni_env, j_path),
        path_factory_(path_factory) {}

  SrAndroidPath(const SrAndroidPath& other)
      : j_path_ref_(other.j_path_ref_), path_factory_(other.path_factory_) {}

  SrAndroidPath(SrAndroidPath&& other)
      : j_path_ref_(std::move(other.j_path_ref_)),
        path_factory_(other.path_factory_) {}

  ~SrAndroidPath() override { path_factory_ = nullptr; }

  SrSVGBox GetBounds() const override;

  std::unique_ptr<canvas::Path> CreateTransformCopy(
      const float (&xform)[6]) const override;

  void Transform(const float (&xform)[6]) override;

  void AddPath(canvas::Path* path) override {}

  jobject GetJPath() const { return j_path_ref_.Get(); }

 private:
  JavaGlobalRef<jobject> j_path_ref_;
  SrAndroidPathFactory* path_factory_{nullptr};
};

}  // namespace android
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPATH_H_
