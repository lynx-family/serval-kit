// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_
#define SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_

#include "element/SrSVGNode.h"
#include <string>

namespace serval {
namespace svg {
namespace element {

class SrSVGFilterPrimitive : public SrSVGNode {
 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 protected:
  explicit SrSVGFilterPrimitive(SrSVGTag t) : SrSVGNode(t) {}
  std::string result_;
  std::string in_;
  SrSVGLength x_{.value = 0.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength y_{.value = 0.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength width_{.value = 100.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
  SrSVGLength height_{.value = 100.0f, .unit = SR_SVG_UNITS_PERCENTAGE};
};

class SrSVGFeGaussianBlur : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeGaussianBlur* Make() { return new SrSVGFeGaussianBlur(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  float std_deviation_x() const { return std_deviation_x_; }
  float std_deviation_y() const { return std_deviation_y_; }

 private:
  SrSVGFeGaussianBlur() : SrSVGFilterPrimitive(SrSVGTag::kFeGaussianBlur) {}
  float std_deviation_x_{0.f};
  float std_deviation_y_{0.f};
};

class SrSVGFeOffset : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeOffset* Make() { return new SrSVGFeOffset(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  float dx() const { return dx_; }
  float dy() const { return dy_; }

 private:
  SrSVGFeOffset() : SrSVGFilterPrimitive(SrSVGTag::kFeOffset) {}
  float dx_{0.f};
  float dy_{0.f};
};

class SrSVGFeColorMatrix : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeColorMatrix* Make() { return new SrSVGFeColorMatrix(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  const std::vector<float>& values() const { return values_; }

 private:
  SrSVGFeColorMatrix() : SrSVGFilterPrimitive(SrSVGTag::kFeColorMatrix) {}
  // type defaults to matrix
  std::string type_{"matrix"};
  std::vector<float> values_;
};

class SrSVGFeComposite : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeComposite* Make() { return new SrSVGFeComposite(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGFeComposite() : SrSVGFilterPrimitive(SrSVGTag::kFeComposite) {}
  std::string in2_;
  std::string operator_{"over"};
  float k1_{0.f};
  float k2_{0.f};
  float k3_{0.f};
  float k4_{0.f};
};

class SrSVGFeBlend : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeBlend* Make() { return new SrSVGFeBlend(); }
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGFeBlend() : SrSVGFilterPrimitive(SrSVGTag::kFeBlend) {}
  std::string in2_;
  std::string mode_{"normal"};
};

class SrSVGFeFlood : public SrSVGFilterPrimitive {
 public:
  static SrSVGFeFlood* Make() { return new SrSVGFeFlood(); }
  ~SrSVGFeFlood() override;
  bool ParseAndSetAttribute(const char* name, const char* value) override;

 private:
  SrSVGFeFlood() : SrSVGFilterPrimitive(SrSVGTag::kFeFlood) {}
  SrSVGPaint* flood_color_{nullptr};
  float flood_opacity_{1.f};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_
