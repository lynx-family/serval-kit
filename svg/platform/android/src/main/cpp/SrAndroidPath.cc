// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/android/SrAndroidPath.h"

namespace serval {
namespace svg {
namespace android {

SrSVGBox SrAndroidPath::GetBounds() const {
  if (path_factory_) {
    return path_factory_->GetBounds(j_path_ref_);
  }
  return {0.f, 0.f, 1.f, 1.f};
}

std::unique_ptr<canvas::Path> SrAndroidPath::CreateTransformCopy(
    const float (&xform)[6]) const {
  if (path_factory_) {
    path_factory_->CreateTransformCopy(*this, xform);
  }
  return std::make_unique<SrAndroidPath>(*this);
}

void SrAndroidPath::Transform(const float (&xform)[6]) {
  if (path_factory_) {
    path_factory_->ApplyTransform(*this, xform);
  }
}

void SrAndroidPath::SetFillType(SrSVGFillRule rule) {
  if (path_factory_) {
    path_factory_->SetFillType(*this, rule);
  }
}

}  // namespace android
}  // namespace svg
}  // namespace serval
