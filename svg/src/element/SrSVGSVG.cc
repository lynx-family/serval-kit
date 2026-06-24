// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGSVG.h"

#include "element/SrSVGContainer.h"
#include "element/SrSVGTypes.h"
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
  canvas->Transform(centered);
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
    view_box_ = make_serval_view_box(value);
    LOGV("viewBox =[%f, %f, %f, %f]", view_box_.left, view_box_.top,
         view_box_.width, view_box_.height);
    return true;
  } else if (strcmp(name, "preserveAspectRatio") == 0) {
    preserve_aspect_radio_ = make_preserve_aspect_radio(value);
    LOGV("preserveAspectRatio =[%f, %f, %f]", preserve_aspect_radio_.scale,
         preserve_aspect_radio_.align_x, preserve_aspect_radio_.align_y);
    return true;
  } else if (strcmp(name, "style") == 0) {
    parsing_style_ = true;
    ParseStyle(value);
    parsing_style_ = false;
    return true;
  } else if (strcmp(name, "transform") == 0 && parsing_style_) {
    ParseTransform(value, css_transform_);
    has_css_transform_ = true;
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

SrSVGSVG::~SrSVGSVG() = default;

bool SrSVGSVG::OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
  if (IsZero(context.view_port.width) || IsZero(context.view_port.height)) {
    LOGW("skip render due to zero viewport: vp=[%f,%f,%f,%f]",
         context.view_port.left, context.view_port.top, context.view_port.width,
         context.view_port.height);
    return false;
  }

  if (IsZero(context.view_box.width) || IsZero(context.view_box.height)) {
    // W3C SVG behavior: if the root <svg> has no valid viewBox, treat the
    // initial user coordinate system as the viewport coordinate system.
    context.view_box =
        SrSVGBox{0.f, 0.f, context.view_port.width, context.view_port.height};
  }

  canvas->SetViewBox(context.view_port.left, context.view_port.top,
                     context.view_port.width, context.view_port.height);
  float xform[6];
  calculate_view_box_transform(&(context.view_port), &(context.view_box),
                               preserve_aspect_radio_, xform);
  canvas->Transform(xform);
  return true;
}

void SrSVGSVG::OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) {
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
