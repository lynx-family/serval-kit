// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_
#define SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_

#include <memory>
#include <vector>

#include "SrSVGNode.h"
#include "SrSVGTypes.h"
#include "canvas/SrCanvas.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGContainer : public SrSVGNode {
 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void AppendChild(SrSVGNodeBase*) override;
  std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory,
      SrSVGRenderContext* context) const override;

 protected:
  explicit SrSVGContainer(SrSVGTag t) : SrSVGNode(t){};
  ~SrSVGContainer() override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  [[nodiscard]] bool HasChildren() const final;
  std::vector<SrSVGNodeBase*> children_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_
