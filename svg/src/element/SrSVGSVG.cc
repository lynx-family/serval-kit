// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGSVG.h"

#include <algorithm>
#include <cmath>

#include "element/SrSVGContainer.h"
#include "element/SrSVGTypes.h"
#include "parser/SrSVGTraversalState.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGLog.h"

namespace serval {
namespace svg {
namespace element {

namespace {

void ApplyOriginWrappedTransform(canvas::SrCanvas* canvas,
                                 const float (&transform)[6], float origin_x,
                                 float origin_y) {
  float centered[6];
  xform_identity(centered);
  xform_pre_translate(centered, origin_x, origin_y);
  xform_multiply(centered, transform);
  xform_pre_translate(centered, -origin_x, -origin_y);
  if (std::all_of(centered, centered + 6,
                  [](float value) { return std::isfinite(value); })) {
    canvas->Transform(centered);
  } else {
    canvas->Transform(transform);
  }
}

void ResolveRootCssOrigin(const SrSVGSVG& svg, SrSVGRenderContext& context,
                          canvas::PathFactory* path_factory, float* x,
                          float* y) {
  if (svg.ResolveTransformOrigin(x, y, context, path_factory)) {
    return;
  }
  *x = context.view_box.left + context.view_box.width * 0.5f;
  *y = context.view_box.top + context.view_box.height * 0.5f;
}

}  // namespace

SrSVGSVG::SrSVGSVG(SrSVGTag tag)
    : SrSVGContainer(tag),
      preserve_aspect_radio_(make_default_preserve_aspect_radio()),
      view_box_{0.f, 0.f, 0.f, 0.f} {}

bool SrSVGSVG::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "viewBox") == 0) {
    if (!parse_serval_view_box(value, &view_box_))
      return false;
    has_view_box_ = true;
    LOGV("viewBox =[%f, %f, %f, %f]", view_box_.left, view_box_.top,
         view_box_.width, view_box_.height);
    return true;
  } else if (strcmp(name, "width") == 0 || strcmp(name, "height") == 0) {
    auto& length = strcmp(name, "width") == 0 ? width_ : height_;
    if (!ParseLengthAttribute(value, length, LengthRange::kNonNegative))
      return false;
    if (parsing_style_)
      StoreAttribute(name, value);
    return true;
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    if (!parse_serval_preserve_aspect_ratio(value, &preserve_aspect_radio_))
      return false;
    LOGV("preserveAspectRatio =[%f, %f, %f]", preserve_aspect_radio_.scale,
         preserve_aspect_radio_.align_x, preserve_aspect_radio_.align_y);
    return true;
  } else if (strcmp(name, "style") == 0) {
    parsing_style_ = true;
    ParseStyle(value);
    parsing_style_ = false;
    return true;
  } else if (strcmp(name, "transform") == 0 && parsing_style_) {
    if (!TryParseTransform(value, css_transform_))
      return false;
    has_css_transform_ = true;
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

SrSVGSVG::~SrSVGSVG() = default;

void SrSVGSVG::ClearAnimatedAttribute(const std::string& name) {
  if (name == "viewBox") {
    view_box_ = {};
    has_view_box_ = false;
  } else if (name == "width") {
    width_.reset();
  } else if (name == "height") {
    height_.reset();
  } else {
    SrSVGContainer::ClearAnimatedAttribute(name);
  }
}

SrSVGBox SrSVGSVG::ResolveImplicitViewport(float dpi) const {
  if (has_view_box_)
    return view_box_;
  SrSVGRenderContext context{};
  context.dpi = dpi;
  SrSVGBox viewport{};
  if (!width_ || !height_ ||
      !resolve_serval_length(&*width_, &context, SR_SVG_LENGTH_TYPE_HORIZONTAL,
                             &viewport.width) ||
      !resolve_serval_length(&*height_, &context, SR_SVG_LENGTH_TYPE_VERTICAL,
                             &viewport.height))
    return {};
  return viewport;
}

bool SrSVGSVG::OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  if (!serval_svg_valid_box(&context.view_port))
    return false;
  if (has_view_box_) {
    if (!serval_svg_valid_box(&view_box_))
      return false;
    context.view_box = view_box_;
  } else {
    context.view_box = {0.f, 0.f, context.view_port.width,
                        context.view_port.height};
  }

  float xform[6];
  if (!resolve_serval_view_box_transform(&context.view_port, &context.view_box,
                                         preserve_aspect_radio_, xform))
    return false;
  canvas->SetViewBox(context.view_port.left, context.view_port.top,
                     context.view_port.width, context.view_port.height);
  canvas->Transform(xform);
  return true;
}

bool SrSVGSVG::ShouldRender(const SrSVGRenderContext& context) const {
  if (!serval_svg_valid_box(&context.view_port))
    return false;
  if (!has_view_box_)
    return true;
  float transform[6];
  return resolve_serval_view_box_transform(&context.view_port, &view_box_,
                                           preserve_aspect_radio_, transform);
}

void SrSVGSVG::OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) {
  if (!ShouldRender(context))
    return;
  if (has_css_transform_) {
    float origin_x = 0.f;
    float origin_y = 0.f;
    ResolveRootCssOrigin(*this, context, canvas->PathFactory(), &origin_x,
                         &origin_y);
    ApplyOriginWrappedTransform(canvas, css_transform_, origin_x, origin_y);
  }
  SrSVGContainer::OnRender(canvas, context);
}

bool SrSVGSVG::RenderChildAt(canvas::SrCanvas* canvas,
                             SrSVGRenderContext& context, size_t index) {
  parser::SrSVGViewBoxScope view_box_scope(context);
  canvas->Save();
  canvas->SetRenderContext(&context);
  if (!OnPrepareToRender(canvas, context)) {
    canvas->Restore();
    return false;
  }
  if (has_css_transform_) {
    float origin_x = 0.f;
    float origin_y = 0.f;
    ResolveRootCssOrigin(*this, context, canvas->PathFactory(), &origin_x,
                         &origin_y);
    ApplyOriginWrappedTransform(canvas, css_transform_, origin_x, origin_y);
  }
  bool rendered = SrSVGContainer::RenderChildAt(canvas, context, index);
  canvas->Restore();
  return rendered;
}

bool SrSVGSVG::RenderChildPathAt(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context,
                                 const std::vector<size_t>& path,
                                 size_t depth) {
  parser::SrSVGViewBoxScope view_box_scope(context);
  canvas->Save();
  canvas->SetRenderContext(&context);
  if (!OnPrepareToRender(canvas, context)) {
    canvas->Restore();
    return false;
  }
  if (has_css_transform_) {
    float origin_x = 0.f;
    float origin_y = 0.f;
    ResolveRootCssOrigin(*this, context, canvas->PathFactory(), &origin_x,
                         &origin_y);
    ApplyOriginWrappedTransform(canvas, css_transform_, origin_x, origin_y);
  }
  bool rendered =
      SrSVGContainer::RenderChildPathAt(canvas, context, path, depth);
  canvas->Restore();
  return rendered;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
