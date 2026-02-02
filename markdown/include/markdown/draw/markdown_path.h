// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_MARKDOWN_CANVAS_MARKDOWN_PATH_H_
#define THIRD_PARTY_MARKDOWN_CANVAS_MARKDOWN_PATH_H_
#include "markdown/utils/markdown_definition.h"
namespace lynx::markdown {
class MarkdownPath {
 public:
  MarkdownPath() = default;
  ~MarkdownPath() = default;
  struct Arc {
    PointF center_;
    float radius_;
    float start_angle_;
    float end_angle_;
  };
  struct RoundRect {
    RectF rect_;
    float radius_x_;
    float radius_y_;
  };
  struct Cubic {
    PointF control_1_;
    PointF control_2_;
    PointF end_;
  };
  struct Quad {
    PointF control_;
    PointF end_;
  };
  void AddArc(const Arc& arc) {
    path_ops_.emplace_back(PathOp{.op_ = kArc, .data_ = {.arc_ = arc}});
  }
  void AddOval(const RectF& oval_rect) {
    path_ops_.emplace_back(PathOp{.op_ = kOval, .data_ = {.rect_ = oval_rect}});
  }
  void AddRect(const RectF& rect) {
    path_ops_.emplace_back(PathOp{.op_ = kRect, .data_ = {.rect_ = rect}});
  }
  void AddRoundRect(const RoundRect& round_rect) {
    path_ops_.emplace_back(
        PathOp{.op_ = kRoundRect, .data_ = {.round_rect_ = round_rect}});
  }
  void MoveTo(const PointF point) {
    path_ops_.emplace_back(PathOp{.op_ = kMoveTo, .data_ = {.point_ = point}});
  }
  void LineTo(const PointF point) {
    path_ops_.emplace_back(PathOp{.op_ = kLineTo, .data_ = {.point_ = point}});
  }
  void CubicTo(const Cubic& cubic) {
    path_ops_.emplace_back(PathOp{.op_ = kCubicTo, .data_ = {.cubic_ = cubic}});
  }
  void QuadTo(const Quad quad) {
    path_ops_.emplace_back(PathOp{.op_ = kQuadTo, .data_ = {.quad_ = quad}});
  }
  enum PathOpType : uint8_t {
    kArc,
    kOval,
    kRect,
    kRoundRect,
    kMoveTo,
    kLineTo,
    kCubicTo,
    kQuadTo,
  };
  struct PathOp {
    PathOpType op_;
    union {
      PointF point_;
      Arc arc_;
      RectF rect_;
      RoundRect round_rect_;
      Cubic cubic_;
      Quad quad_;
    } data_;
  };
  std::vector<PathOp> path_ops_;
};
}  // namespace lynx::markdown
#endif  // THIRD_PARTY_MARKDOWN_CANVAS_MARKDOWN_PATH_H_
