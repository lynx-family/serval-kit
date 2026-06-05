// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_
#define SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_

#include <optional>
#include <string>
#include <vector>
#include "element/SrSVGNode.h"
namespace serval {
namespace svg {
namespace element {

class SrSVGFilterPrimitive : public SrSVGNode {
 public:
  bool ParseAndSetAttribute(const char* name, const char* value) override;
  const std::string& result() const { return result_; }
  const std::string& input() const { return in_; }
  const std::optional<SrSVGLength>& x() const { return x_; }
  const std::optional<SrSVGLength>& y() const { return y_; }
  const std::optional<SrSVGLength>& width() const { return width_; }
  const std::optional<SrSVGLength>& height() const { return height_; }

 protected:
  explicit SrSVGFilterPrimitive(SrSVGTag t) : SrSVGNode(t) {}
  std::string result_;
  std::string in_;
  std::optional<SrSVGLength> x_;
  std::optional<SrSVGLength> y_;
  std::optional<SrSVGLength> width_;
  std::optional<SrSVGLength> height_;
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
  const std::string& type() const { return type_; }

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
  const std::string& input2() const { return in2_; }
  const std::string& composite_operator() const { return operator_; }
  float k1() const { return k1_; }
  float k2() const { return k2_; }
  float k3() const { return k3_; }
  float k4() const { return k4_; }

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
  const std::string& input2() const { return in2_; }
  const std::string& mode() const { return mode_; }

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
  const SrSVGPaint* flood_color() const { return flood_color_; }
  float flood_opacity() const { return flood_opacity_; }

 private:
  SrSVGFeFlood() : SrSVGFilterPrimitive(SrSVGTag::kFeFlood) {}
  SrSVGPaint* flood_color_{nullptr};
  float flood_opacity_{1.f};
};

}  // namespace element
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_ELEMENT_SRSVGFILTERPRIMITIVES_H_
