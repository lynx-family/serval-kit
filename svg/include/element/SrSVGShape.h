// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_

#include <memory>

#include "SrSVGNode.h"
#include "SrSVGTypes.h"
#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGShape : public SrSVGNode {
 public:
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory, SrSVGRenderContext* context,
      bool include_transform = true) const override;
  void AppendChild(SrSVGNodeBase* node) override;
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  void OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) final;
  explicit SrSVGShape(SrSVGTag t) : SrSVGNode(t){};
  virtual void onDraw(canvas::SrCanvas*, SrSVGRenderContext& context) const = 0;
  bool HasEffectiveFill() const {
    return render_state_.fill &&
           render_state_.fill->type != SERVAL_PAINT_NONE &&
           render_state_.fill_opacity > 0.f;
  }
  bool HasEffectiveStroke() const {
    return render_state_.stroke &&
           render_state_.stroke->type != SERVAL_PAINT_NONE &&
           render_state_.stroke_width > 0.f &&
           render_state_.stroke_opacity > 0.f;
  }

  //  static void XformIdentity(float* xform);
  //  static void XformSetTranslation(float* xform, float tx, float ty);
  //  static void XformSetScale(float* xform, float sx, float sy);
  //  static void XformSetRotation(float* xform, float d);
  //  static void XformSetSkewX(float* xform, float d);
  //  static void XformSetSkewY(float* xform, float d);
  //  static void XformMultiply(float* t, const float* s);
  //  static void XformPreMultiply(float* t, const float* s);

 protected:
  SrSVGFillRule fill_rule_{SR_SVG_FILL};
  SrSVGRenderState render_state_{
      nullptr,     nullptr, 1.f,
      1.f,         1.f,     1.f,
      SR_SVG_FILL, nullptr, SR_SVG_VECTOR_EFFECT_NONE};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_
