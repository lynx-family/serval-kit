// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_
#define SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_

#include <cstddef>
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
      canvas::PathFactory* path_factory, SrSVGRenderContext* context,
      bool include_transform = true) const override;

  const std::vector<SrSVGNodeBase*>& children() const { return children_; }
  size_t ChildCount() const;
  virtual bool RenderChildAt(canvas::SrCanvas* canvas,
                             SrSVGRenderContext& context, size_t index);
  virtual bool RenderChildPathAt(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context,
                                 const std::vector<size_t>& path,
                                 size_t depth = 0);

 protected:
  explicit SrSVGContainer(SrSVGTag t) : SrSVGNode(t){};
  ~SrSVGContainer() override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  [[nodiscard]] bool HasChildren() const final;
  void RenderChild(canvas::SrCanvas* canvas, SrSVGRenderContext& context,
                   SrSVGNodeBase* child);
  bool PrepareChild(SrSVGNodeBase* child);
  void RestoreChild(SrSVGNodeBase* child);

 protected:
  std::vector<SrSVGNodeBase*> children_;

 private:
  struct ChildRenderState {
    SrSVGPaint* fill_paint{nullptr};
    SrSVGPaint* stroke_paint{nullptr};
    SrSVGPaint* clip_path{nullptr};
    SrSVGPaint* mask{nullptr};
    std::optional<SrSVGLength> stroke_width;
    std::optional<float> fill_opacity;
    std::optional<float> stroke_opacity;
    std::optional<SrSVGColor> color;
  };
  std::vector<ChildRenderState> child_render_state_stack_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGCONTAINER_H_
