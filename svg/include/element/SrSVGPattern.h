// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGPATTERN_H_
#define SVG_INCLUDE_ELEMENT_SRSVGPATTERN_H_

#include <optional>
#include <string>

#include "element/SrSVGContainer.h"
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

class SrSVGPattern : public SrSVGContainer {
 public:
  static SrSVGPattern* Make() { return new SrSVGPattern(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  void OnRender(canvas::SrCanvas*, SrSVGRenderContext&) override;
  void RenderContent(canvas::SrCanvas* canvas,
                     SrSVGRenderContext& context) const;

  const std::optional<SrSVGLength>& x() const { return x_; }
  const std::optional<SrSVGLength>& y() const { return y_; }
  const std::optional<SrSVGLength>& width() const { return width_; }
  const std::optional<SrSVGLength>& height() const { return height_; }
  SrSVGObjectBoundingBoxUnitType pattern_units() const {
    return pattern_units_;
  }
  bool has_pattern_units() const { return has_pattern_units_; }
  SrSVGObjectBoundingBoxUnitType pattern_content_units() const {
    return pattern_content_units_;
  }
  bool has_pattern_content_units() const { return has_pattern_content_units_; }
  const float (&pattern_transform() const)[6] { return pattern_transform_; }
  bool has_pattern_transform() const { return has_pattern_transform_; }
  const SrSVGBox& view_box() const { return view_box_; }
  bool has_view_box() const { return has_view_box_; }
  const SrSVGPreserveAspectRatio& preserve_aspect_ratio() const {
    return preserve_aspect_ratio_;
  }
  bool has_preserve_aspect_ratio() const { return has_preserve_aspect_ratio_; }
  const std::string& href() const { return href_; }
  bool has_href() const { return !href_.empty(); }

 private:
  SrSVGPattern() : SrSVGContainer(SrSVGTag::kPattern) {}

  std::optional<SrSVGLength> x_;
  std::optional<SrSVGLength> y_;
  std::optional<SrSVGLength> width_;
  std::optional<SrSVGLength> height_;

  SrSVGObjectBoundingBoxUnitType pattern_units_{
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX};
  SrSVGObjectBoundingBoxUnitType pattern_content_units_{
      SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE};
  bool has_pattern_units_{false};
  bool has_pattern_content_units_{false};

  float pattern_transform_[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  bool has_pattern_transform_{false};

  SrSVGBox view_box_{0, 0, 0, 0};
  SrSVGPreserveAspectRatio preserve_aspect_ratio_{
      make_default_preserve_aspect_radio()};
  bool has_view_box_{false};
  bool has_preserve_aspect_ratio_{false};
  std::string href_;
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGPATTERN_H_
