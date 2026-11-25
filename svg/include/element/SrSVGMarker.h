// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGMARKER_H_
#define SVG_INCLUDE_ELEMENT_SRSVGMARKER_H_

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "element/SrSVGContainer.h"

namespace serval::svg::element {

struct SrSVGMarkerPosition {
  float x;
  float y;
  float angle;  // Degrees in the referencing shape's coordinate system.
};

// Uses the equivalent path, retaining command vertices rather than flattened
// curve samples. Exposed for geometry regression tests.
std::vector<SrSVGMarkerPosition> SrMarkerPositions(const SrPathData& path);
std::vector<SrSVGMarkerPosition> SrMarkerPositions(const SrSVGNodeBase& shape,
                                                   SrSVGRenderContext context);

class SrSVGMarker : public SrSVGContainer {
 public:
  struct ResolvedMarkers {
    std::array<SrSVGMarker*, 3> nodes{};
    // Keep whole-shape opacity composition for declared marker references,
    // including references whose definitions are missing or unsupported.
    bool has_references{false};
  };

  static SrSVGMarker* Make() { return new SrSVGMarker(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  static bool ParseReference(const char* value,
                             std::optional<std::string>* reference);
  static ResolvedMarkers ResolveMarkers(const SrSVGNode& shape,
                                        const SrSVGRenderContext& context);
  static void RenderMarkers(canvas::SrCanvas* canvas,
                            SrSVGRenderContext& context, const SrSVGNode& shape,
                            const SrSVGRenderState& state,
                            const ResolvedMarkers& markers);

 private:
  struct StrokeTransform {
    float current[6];
    float inverse[6];
  };

  SrSVGMarker() : SrSVGContainer(SrSVGTag::kMarker) {}
  void RenderInstance(canvas::SrCanvas* canvas, SrSVGRenderContext context,
                      const SrSVGMarkerPosition& position, bool is_start,
                      const SrSVGRenderState& state,
                      const StrokeTransform* stroke_transform);
  SrSVGLength width_{3.f, SR_SVG_UNITS_NUMBER};
  SrSVGLength height_{3.f, SR_SVG_UNITS_NUMBER};
  SrSVGLength ref_x_{0.f, SR_SVG_UNITS_NUMBER};
  SrSVGLength ref_y_{0.f, SR_SVG_UNITS_NUMBER};
  SrSVGBox view_box_{0.f, 0.f, 0.f, 0.f};
  bool has_view_box_{false};
  bool stroke_units_{true};
  bool auto_orient_{false};
  bool start_reverse_{false};
  bool clip_overflow_{true};
  float angle_{0.f};
  SrSVGPreserveAspectRatio aspect_{make_default_preserve_aspect_radio()};
};

}  // namespace serval::svg::element

#endif  // SVG_INCLUDE_ELEMENT_SRSVGMARKER_H_
