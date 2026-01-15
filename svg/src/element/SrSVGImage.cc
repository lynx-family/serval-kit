// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGImage.h"

#include "canvas/SrCanvas.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace serval {
namespace svg {
namespace element {

bool SrSVGImage::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "href") == 0 || strcmp(name, "xlink:href") == 0) {
    href_ = value;
    return true;
  } else if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
    return true;
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    preserve_aspect_radio_ = make_preserve_aspect_radio(value);
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

void SrSVGImage::onDraw(canvas::SrCanvas* canvas,
                        SrSVGRenderContext& context) const {
  canvas->DrawImage(href_.c_str(), x_.value, y_.value, width_.value,
                    height_.value, preserve_aspect_radio_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
