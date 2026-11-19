// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SVG_INCLUDE_CANVAS_SRCANVASTRANSFORMSTACK_H_
#define SVG_INCLUDE_CANVAS_SRCANVASTRANSFORMSTACK_H_

#include <array>
#include "parser/SrSVGArray.h"

namespace serval::svg::canvas {

class SrCanvasTransformStack {
 public:
  using Matrix = std::array<float, 6>;

  // Plain saves cannot report failure. Record their matrix on the first change.
  void Save() { ++depth_; }

  bool BeforeTransform(const Matrix& current) {
    return depth_ == 0 ||
           (!frames_.empty() && frames_.back().depth == depth_) ||
           frames_.push_back({depth_, current});
  }

  template <typename BeginLayer>
  bool TrySaveLayer(const Matrix& current, BeginLayer begin) {
    if (!frames_.reserve(frames_.size() + 1) || !begin())
      return false;
    ++depth_;
    frames_.push_back({depth_, current});  // Capacity reserved before begin().
    return true;
  }

  void Restore(Matrix& current) {
    if (!depth_)
      return;
    if (!frames_.empty() && frames_.back().depth == depth_) {
      current = frames_.back().matrix;
      frames_.pop_back();
    }
    --depth_;
  }

  void Clear() {
    frames_.clear();
    depth_ = 0;
  }

 private:
  struct Frame {
    size_t depth;
    Matrix matrix;
  };
  parser::SrSVGArray<Frame> frames_;
  size_t depth_{0};
};

}  // namespace serval::svg::canvas
#endif  // SVG_INCLUDE_CANVAS_SRCANVASTRANSFORMSTACK_H_
