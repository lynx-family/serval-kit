// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPH_H_
#define SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPH_H_

#include "canvas/SrCanvas.h"
#include "canvas/SrParagraph.h"
#include "platform/android/SrJNIUtils.h"
namespace serval {
namespace svg {
namespace canvas {
class SrAndroidParagraph : public serval::svg::canvas::Paragraph {
 public:
  SrAndroidParagraph(jobject jParagraphRef, SrParagraphStyle paragraph_style);
  ~SrAndroidParagraph() = default;

  void Layout(float max_width) override;

  void Draw(serval::svg::canvas::SrCanvas* canvas, float x, float y) override;

  jobject j_paragraph_;
  SrParagraphStyle paragraph_style_;
};

}  // namespace canvas
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_ANDROID_SRANDROIDPARAGRAPH_H_
