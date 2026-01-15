// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_

#include <cstdint>
#include <memory>

#include "SrSVGNode.h"
#include "SrSVGTypes.h"
#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGShape : public SrSVGNode {
 protected:
  // todo(renzhongyue): add an independent file for constants declaration.
  static const uint8_t kRenderTypeFlagStroke;
  static const uint8_t kRenderTypeFlagFill;
  // mask for fill-rule. bit is set if fill-rule is evenodd. default to nonzero
  static const uint8_t kRenderTypeFillRule;

 public:
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;
  void AppendChild(SrSVGNodeBase* node) override;
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  void OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) final;
  explicit SrSVGShape(SrSVGTag t) : SrSVGNode(t){};
  virtual void onDraw(canvas::SrCanvas*, SrSVGRenderContext& context) const = 0;

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
  SrSVGRenderState render_state_{nullptr, nullptr, 1.f,        1.f,
                                 1.f,     1.f,     SR_SVG_FILL};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGSHAPE_H_
