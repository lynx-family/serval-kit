// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGUSE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGUSE_H_

#include <memory>
#include <string>

#include "SrSVGShape.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGUse : public SrSVGNode {
 public:
  static SrSVGUse* Make() { return new SrSVGUse(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) override;
  bool OnPrepareToRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const override;
  bool HasChildren() const override;
  void AppendChild(SrSVGNodeBase*) override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 private:
  void renderRealNode(SrSVGNodeBase* node, canvas::SrCanvas* canvas,
                      SrSVGRenderContext& context);

 private:
  SrSVGUse() : SrSVGNode(SrSVGTag::kUse) {}
  std::string href_;
  SrSVGLength x_{0}, y_{0}, width_{0}, height_{0};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGUSE_H_
