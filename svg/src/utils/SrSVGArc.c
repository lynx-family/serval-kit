// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "utils/SrSVGArc.h"

#include <float.h>
#include <math.h>
#include <string.h>

// The ellipse axes include the radii and rotation. Calculate intermediate
// points relative to the authored start, so adding and subtracting a distant
// center cannot erase the displacement of a short arc.
static bool arc_cubics(SrSVGArc* result, const float args[9], double axis_xx,
                       double axis_xy, double axis_yx, double axis_yy,
                       double start_cos, double start_sin, double sweep) {
  // Keep each cubic Bézier segment strictly below 45 degrees.
  const double subdivisions = floor(fabs(sweep) / (M_PI / 4)) + 1;
  if (!isfinite(subdivisions) || subdivisions < 1 ||
      subdivisions > SR_SVG_ARC_MAX_CUBICS)
    return false;
  const int segments = (int)subdivisions;
  const double step = sweep / segments;
  const double half_tan = tan(step / 2);
  const double alpha = sin(step) * (sqrt(4 + 3 * half_tan * half_tan) - 1) / 3;

  const double start_x = args[0], start_y = args[1];
  double x = start_x, y = start_y;
  double tangent_x = -axis_xx * start_sin + axis_yx * start_cos;
  double tangent_y = -axis_xy * start_sin + axis_yy * start_cos;
  for (int i = 0; i < segments; ++i) {
    const double angle = (i + 1) * step;
    const double sin_angle = sin(angle);
    const double sin_half_angle = sin(angle / 2);
    const double cos_minus_one = -2 * sin_half_angle * sin_half_angle;
    const double delta_cos = start_cos * cos_minus_one - start_sin * sin_angle;
    const double delta_sin = start_sin * cos_minus_one + start_cos * sin_angle;
    const double end_cos = start_cos + delta_cos;
    const double end_sin = start_sin + delta_sin;
    const double next_tangent_x = -axis_xx * end_sin + axis_yx * end_cos;
    const double next_tangent_y = -axis_xy * end_sin + axis_yy * end_cos;

    const double end_x =
        i + 1 == segments ? args[7]
                          : start_x + axis_xx * delta_cos + axis_yx * delta_sin;
    const double end_y =
        i + 1 == segments ? args[8]
                          : start_y + axis_xy * delta_cos + axis_yy * delta_sin;
    const double values[] = {x + alpha * tangent_x,
                             y + alpha * tangent_y,
                             end_x - alpha * next_tangent_x,
                             end_y - alpha * next_tangent_y,
                             end_x,
                             end_y};
    for (int j = 0; j < 6; ++j)
      if (!isfinite(values[j]) || fabs(values[j]) > FLT_MAX)
        return false;
    memcpy(result->cubics[result->count++], values, sizeof(values));
    x = end_x;
    y = end_y;
    tangent_x = next_tangent_x;
    tangent_y = next_tangent_y;
  }
  return true;
}

bool SrSVGArcToCubics(const float args[9], SrSVGArc* out) {
  if (!args || !out)
    return false;
  for (int i = 0; i < 9; ++i)
    if (!isfinite(args[i]))
      return false;

  SrSVGArc result = {0};
  if (args[0] == args[7] && args[1] == args[8]) {
    *out = result;
    return true;
  }
  double rx = fabs(args[2]), ry = fabs(args[3]);
  if (rx == 0 || ry == 0) {
    result.line = true;
    *out = result;
    return true;
  }

  // In the rotated coordinate system centered at the endpoint midpoint, the
  // endpoints are opposite. Normalize their half-chord by the ellipse radii.
  const double theta = fmod(args[4], 360.0) * (M_PI / 180);
  const double cos_theta = cos(theta), sin_theta = sin(theta);
  const double half_dx = ((double)args[0] - args[7]) / 2;
  const double half_dy = ((double)args[1] - args[8]) / 2;
  const double chord_x = (cos_theta * half_dx + sin_theta * half_dy) / rx;
  const double chord_y = (-sin_theta * half_dx + cos_theta * half_dy) / ry;
  const double norm = hypot(chord_x, chord_y);
  const double unit_x = chord_x / norm;
  const double unit_y = chord_y / norm;

  // A chord longer than the unit diameter requires the same scale on both
  // axes. The resulting arc is exactly a half ellipse.
  double half_chord = norm;
  if (norm > 1) {
    rx *= norm;
    ry *= norm;
    half_chord = 1;
  }

  const bool large = fabsf(args[5]) > 1e-6f;
  const bool clockwise = fabsf(args[6]) > 1e-6f;
  const double side = large == clockwise ? -1 : 1;
  const double center_offset = side * sqrt((1 - half_chord) * (1 + half_chord));
  const double start_cos = half_chord * unit_x - center_offset * unit_y;
  const double start_sin = half_chord * unit_y + center_offset * unit_x;
  const double minor_sweep = 2 * asin(half_chord);
  const double sweep =
      (clockwise ? 1 : -1) * (large ? 2 * M_PI - minor_sweep : minor_sweep);

  if (!arc_cubics(&result, args, rx * cos_theta, rx * sin_theta,
                  -ry * sin_theta, ry * cos_theta, start_cos, start_sin,
                  sweep)) {
    // Keep the complete segment representable by a float-based path backend.
    result = (SrSVGArc){.line = true};
  }
  *out = result;
  return true;
}
