// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "utils/SrSVGPatternUtils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "parser/SrSVGTraversalState.h"

namespace serval {
namespace svg {
namespace element {

PatternTileBudget::PatternTileBudget(const SrSVGRenderContext& context, float x,
                                     float y, float right, float bottom,
                                     float width, float height)
    : traversal_(context.traversal_state) {
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(right) ||
      !std::isfinite(bottom) || !std::isfinite(width) ||
      !std::isfinite(height) || width <= 0.f || height <= 0.f || x >= right ||
      y >= bottom)
    return;
  const double columns = std::ceil((static_cast<double>(right) - x) / width);
  const double rows = std::ceil((static_cast<double>(bottom) - y) / height);
  // Reject a grid whose float increments may stop advancing, or whose work
  // exceeds the per-render budget. Retain normal float placement and rounding.
  if (x + width <= x || right + width <= right || y + height <= y ||
      bottom + height <= bottom ||
      columns * rows > parser::SrSVGTraversalState::kMaxPatternTiles)
    return;
  remaining_ = parser::SrSVGTraversalState::kMaxPatternTiles;
}

bool PatternTileBudget::HasRemaining() const {
  auto* state = static_cast<parser::SrSVGTraversalState*>(traversal_);
  return remaining_ &&
         (!state || (state->HasRenderBudget() &&
                     state->pattern_tiles <
                         parser::SrSVGTraversalState::kMaxPatternTiles));
}

void PatternTileBudget::Consume() {
  --remaining_;
  if (auto* state = static_cast<parser::SrSVGTraversalState*>(traversal_))
    state->TakePatternTile();
}

bool IsIdentityTransform(const float* xform) {
  return std::fabs(xform[0] - 1.f) < 1e-6f && std::fabs(xform[1]) < 1e-6f &&
         std::fabs(xform[2]) < 1e-6f && std::fabs(xform[3] - 1.f) < 1e-6f &&
         std::fabs(xform[4]) < 1e-6f && std::fabs(xform[5]) < 1e-6f;
}

bool ComposeAffineTransform(const float* left, const float* right, float* out) {
  float combined[6];
  std::memcpy(combined, left, sizeof(combined));
  xform_multiply(combined, right);
  if (!std::all_of(combined, combined + 6,
                   [](float x) { return std::isfinite(x); }))
    return false;
  std::memcpy(out, combined, sizeof(combined));
  return true;
}

bool InvertAffineTransform(const float* xform, float* inverse) {
  const double a = xform[0], b = xform[1], c = xform[2], d = xform[3],
               x = xform[4], y = xform[5];
  const double det = a * d - b * c;
  if (!std::isfinite(det) || std::fabs(det) < 1e-6)
    return false;
  const float det_float = xform[0] * xform[3] - xform[1] * xform[2];
  if (std::isfinite(det_float) && std::fabs(det_float) >= 1e-6f) {
    const float inv_det = 1.f / det_float;
    const float candidate[] = {
        xform[3] * inv_det,
        -xform[1] * inv_det,
        -xform[2] * inv_det,
        xform[0] * inv_det,
        (xform[2] * xform[5] - xform[3] * xform[4]) * inv_det,
        (xform[1] * xform[4] - xform[0] * xform[5]) * inv_det};
    if (std::all_of(candidate, candidate + 6,
                    [](float n) { return std::isfinite(n); })) {
      std::memcpy(inverse, candidate, sizeof(candidate));
      return true;
    }
  }
  const double candidate[] = {d / det,
                              -b / det,
                              -c / det,
                              a / det,
                              (c * y - d * x) / det,
                              (b * x - a * y) / det};
  if (!std::all_of(candidate, candidate + 6, [](double n) {
        return std::isfinite(n) &&
               std::fabs(n) <= std::numeric_limits<float>::max();
      }))
    return false;
  std::copy(candidate, candidate + 6, inverse);
  return true;
}

void MapPoint(const float* xform, float x, float y, float* out_x,
              float* out_y) {
  *out_x = xform[0] * x + xform[2] * y + xform[4];
  *out_y = xform[1] * x + xform[3] * y + xform[5];
}

bool TryMapBounds(const SrSVGBox& box, const float* xform, SrSVGBox* out) {
  float points[8];
  MapPoint(xform, box.left, box.top, &points[0], &points[1]);
  MapPoint(xform, box.left + box.width, box.top, &points[2], &points[3]);
  MapPoint(xform, box.left + box.width, box.top + box.height, &points[4],
           &points[5]);
  MapPoint(xform, box.left, box.top + box.height, &points[6], &points[7]);

  if (!std::all_of(points, points + 8,
                   [](float n) { return std::isfinite(n); }))
    return false;
  float min_x = points[0];
  float max_x = points[0];
  float min_y = points[1];
  float max_y = points[1];
  for (int i = 2; i < 8; i += 2) {
    min_x = std::min(min_x, points[i]);
    max_x = std::max(max_x, points[i]);
    min_y = std::min(min_y, points[i + 1]);
    max_y = std::max(max_y, points[i + 1]);
  }
  const float width = max_x - min_x;
  const float height = max_y - min_y;
  if (!std::isfinite(width) || !std::isfinite(height))
    return false;
  *out = {min_x, min_y, width, height};
  return true;
}

SrSVGBox MapBounds(const SrSVGBox& box, const float* xform) {
  SrSVGBox result{};
  TryMapBounds(box, xform, &result);
  return result;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
