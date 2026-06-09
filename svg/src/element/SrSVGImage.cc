// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGImage.h"

#include "canvas/SrCanvas.h"
#include "utils/SrFloatComparison.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace serval {
namespace svg {
namespace element {

namespace {

float ResolveImageLength(const SrSVGLength& length, SrSVGRenderContext* context,
                         SrSVGLengthType length_type) {
  if (!context) {
    return length.value;
  }
  return convert_serval_length_to_float(&length, context, length_type);
}

}  // namespace

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
  const float x =
      ResolveImageLength(x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  const float y = ResolveImageLength(y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL);
  const float width = width_ ? ResolveImageLength(*width_, &context,
                                                  SR_SVG_LENGTH_TYPE_HORIZONTAL)
                             : 0.f;
  const float height = height_ ? ResolveImageLength(*height_, &context,
                                                    SR_SVG_LENGTH_TYPE_VERTICAL)
                               : 0.f;
  if (!width_ || !height_ || !FloatsLarger(width, 0.f) ||
      !FloatsLarger(height, 0.f)) {
    return;
  }

  canvas->DrawImage(href_.c_str(), x, y, width, height, preserve_aspect_radio_,
                    render_state_.opacity);
}

std::unique_ptr<canvas::Path> SrSVGImage::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  const float x =
      ResolveImageLength(x_, context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
  const float y = ResolveImageLength(y_, context, SR_SVG_LENGTH_TYPE_VERTICAL);
  const float width = width_ ? ResolveImageLength(*width_, context,
                                                  SR_SVG_LENGTH_TYPE_HORIZONTAL)
                             : 0.f;
  const float height = height_ ? ResolveImageLength(*height_, context,
                                                    SR_SVG_LENGTH_TYPE_VERTICAL)
                               : 0.f;
  if (!width_ || !height_ || !FloatsLarger(width, 0.f) ||
      !FloatsLarger(height, 0.f)) {
    return nullptr;
  }
  return path_factory->CreateRect(x, y, 0.f, 0.f, width, height);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
