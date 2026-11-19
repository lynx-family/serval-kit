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

bool SrSVGImage::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "href") == 0 || strcmp(name, "xlink:href") == 0) {
    href_ = value;
    return true;
  } else if (strcmp(name, "x") == 0) {
    return ParseLengthAttribute(value, x_);
  } else if (strcmp(name, "y") == 0) {
    return ParseLengthAttribute(value, y_);
  } else if (strcmp(name, "width") == 0) {
    return ParseLengthAttribute(value, width_, LengthRange::kNonNegative);
  } else if (strcmp(name, "height") == 0) {
    return ParseLengthAttribute(value, height_, LengthRange::kNonNegative);
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    if (!parse_serval_preserve_aspect_ratio(value, &preserve_aspect_radio_))
      return false;
    return true;
  }
  return SrSVGShape::ParseAndSetAttribute(name, value);
}

void SrSVGImage::ClearAnimatedAttribute(const std::string& name) {
  if (name == "width")
    width_.reset();
  else if (name == "height")
    height_.reset();
  else
    SrSVGShape::ClearAnimatedAttribute(name);
}

bool SrSVGImage::ResolveGeometry(const SrSVGRenderContext& context,
                                 SrSVGBox& box) const {
  return width_ && height_ &&
         resolve_serval_length(&x_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                               &box.left) &&
         resolve_serval_length(&y_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                               &box.top) &&
         resolve_serval_length(&*width_, &context,
                               SR_SVG_LENGTH_TYPE_HORIZONTAL, &box.width) &&
         resolve_serval_length(&*height_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                               &box.height) &&
         serval_svg_valid_box(&box);
}

void SrSVGImage::onDraw(canvas::SrCanvas* canvas,
                        SrSVGRenderContext& context) const {
  SrSVGBox box;
  if (!ResolveGeometry(context, box))
    return;
  canvas->DrawImage(href_.c_str(), box.left, box.top, box.width, box.height,
                    preserve_aspect_radio_, render_state_.opacity);
}

std::unique_ptr<canvas::Path> SrSVGImage::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context,
    bool include_transform) const {
  SrSVGBox box;
  if (!path_factory || !context || !ResolveGeometry(*context, box))
    return nullptr;
  auto path = path_factory->CreateRect(box.left, box.top, 0.f, 0.f, box.width,
                                       box.height);
  if (include_transform && path) {
    float xform[6];
    ResolvedTransform(xform, *context, path_factory);
    path->Transform(xform);
  }
  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
