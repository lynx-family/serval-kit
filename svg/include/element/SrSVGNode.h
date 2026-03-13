// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGNODE_H_
#define SVG_INCLUDE_ELEMENT_SRSVGNODE_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "SrSVGTypes.h"
#include "canvas/SrCanvas.h"

namespace serval::svg {

namespace canvas {
class SrCanvas;
}  // namespace canvas

namespace element {

enum class SrSVGTag {
  kCircle,
  kClipPath,
  kDefs,
  kEllipse,
  kG,
  kImage,
  kLine,
  kLinearGradient,
  kPath,
  kPattern,
  kPolygon,
  kPolyline,
  kRadialGradient,
  kRect,
  kStop,
  kSvg,
  kText,
  kTextLiteral,
  kTSpan,
  kUse
};

class SrSVGNodeBase {
 public:
  virtual ~SrSVGNodeBase() = default;
  void Render(canvas::SrCanvas* canvas, SrSVGRenderContext& context);
  virtual bool ParseAndSetAttribute(const char* name, const char* value) = 0;
  virtual void AppendChild(SrSVGNodeBase*) {}
  virtual std::unique_ptr<canvas::Path> AsPath(
      canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
    return nullptr;
  }
  virtual bool IsSVGNode() const { return false; }
  SrSVGTag Tag() const { return tag_; }

 protected:
  explicit SrSVGNodeBase(SrSVGTag tag) : tag_(tag) {}
  virtual bool HasChildren() const { return false; }
  virtual void OnRender(canvas::SrCanvas* canvas, SrSVGRenderContext& context) {
  }
  virtual bool OnPrepareToRender(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext& context) const {
    return false;
  }
  void ParseStyle(const char* str);
  bool ParseNameValue(const char* start, const char* end);

 public:
  std::optional<SrSVGColor> color_;
  std::optional<SrSVGColor> inherit_color_;

 protected:
  std::string id_;

 private:
  SrSVGTag tag_;
};

class SrSVGNode : public SrSVGNodeBase {
 public:
  static void ParseTransform(const char* str, float* xform);
  static int ParseMatrix(float* xform, const char* str);
  static int ParseTranslate(float* xform, const char* str);
  static int ParseScale(float* xform, const char* str);
  static int ParseRotate(float* xform, const char* str);
  static int ParseSkewX(float* xform, const char* str);
  static int ParseSkewY(float* xform, const char* str);
  static int ParseTransformArgs(const char* str, float* args, int maxNa,
                                int* na);
  static const char* ParseNumber(const char* s, char* it, int size);
  static double Atof(const char* s);

 public:
  ~SrSVGNode() override;

  bool ParseAndSetAttribute(const char* name, const char* value) override;

  bool IsSVGNode() const override { return true; }

 protected:
  explicit SrSVGNode(SrSVGTag tag) : SrSVGNodeBase(tag) {}

  bool OnPrepareToRender(canvas::SrCanvas* canvas,
                         SrSVGRenderContext& context) const override;

 private:
  void ParseStrokeDashArray(const char* value);

 public:
  static const float s_stroke_miter_limit;
  SrSVGPaint* fill_{nullptr};
  SrSVGPaint* stroke_{nullptr};
  SrSVGPaint* clip_path_{nullptr};
  std::optional<float> opacity_;
  std::optional<float> fill_opacity_;
  std::optional<float> stroke_opacity_;
  std::optional<SrSVGLength>
      stroke_width_;  //{.value = 1.0f, .unit = SR_SVG_UNITS_PX}

  SrSVGStrokeJoin stroke_join_{SR_SVG_STROKE_JOIN_MITER};

  SrSVGStrokeCap stroke_cap_{SR_SVG_STROKE_CAP_BUTT};

  float stoke_miter_limit_{s_stroke_miter_limit};

  float stroke_dash_offset_{0.f};

  std::vector<float> stroke_dash_array_{};

  //inherit
  SrSVGPaint* inherit_fill_paint_{nullptr};
  SrSVGPaint* inherit_stroke_paint_{nullptr};
  SrSVGPaint* inherit_clip_path_{nullptr};
  std::optional<float> inherit_opacity_;
  std::optional<float> inherit_fill_opacity_;
  std::optional<float> inherit_stroke_opacity_;
  std::optional<SrSVGLength> inherit_stroke_width_;
  float transform_[6]{1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
};

using IDMapper = std::unordered_map<std::string, SrSVGNodeBase*>;

}  // namespace element
}  // namespace serval::svg

#endif  // SVG_INCLUDE_ELEMENT_SRSVGNODE_H_
