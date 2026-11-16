// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PLATFORM_SKITY_SRSKITYFILTERGRAPH_H_
#define SVG_INCLUDE_PLATFORM_SKITY_SRSKITYFILTERGRAPH_H_

#include <memory>
#include <vector>

#include "canvas/SrCanvas.h"
#include "skity/recorder/display_list.hpp"
#include "skity/render/canvas.hpp"

namespace serval {
namespace svg {
namespace skity {

// Compiles names to immutable input indices before drawing. The accepted graph
// has bounded replay work; a shared intermediate never starts an unbounded DAG
// expansion. Unsupported models retain the caller's existing source fallback.
class SrSkityFilterGraph {
 public:
  struct Cost {
    size_t source_replays{1};
    size_t visits{0};
    size_t depth{0};
    double workspace_bytes{0};
  };
  static std::unique_ptr<SrSkityFilterGraph> Compile(
      const canvas::SrFilterModel& model, const ::skity::Matrix& transform,
      float content_scale = 1.f);
  static bool CombineCosts(const Cost& outer, const Cost& inner,
                           Cost* combined);
  const Cost& cost() const { return cost_; }
  void Draw(::skity::Canvas* target, const ::skity::DisplayList& source) const;

 private:
  struct Node {
    canvas::SrFilterPrimitiveModel primitive;
    int first{-1};
    int second{-1};
    bool alpha_only{false};
    size_t visits{1};
    size_t sources{1};
    size_t depth{2};
  };
  void DrawInput(int input, ::skity::Canvas* target,
                 const ::skity::DisplayList& source) const;
  void DrawLayer(int input, ::skity::Canvas* target,
                 const ::skity::DisplayList& source,
                 const ::skity::Rect& region,
                 const ::skity::Paint& paint) const;
  canvas::SrFilterModel model_;
  std::vector<Node> nodes_;
  Cost cost_;
};

}  // namespace skity
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_SKITY_SRSKITYFILTERGRAPH_H_
