// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGFilterPrimitives.h"
#include <cstring>
#include <vector>
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGFilterPrimitive::ParseAndSetAttribute(const char* name,
                                                const char* value) {
  if (strcmp(name, "result") == 0) {
    result_ = value;
  } else if (strcmp(name, "in") == 0) {
    in_ = value;
  } else if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
  } else {
    return SrSVGNode::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeGaussianBlur::ParseAndSetAttribute(const char* name,
                                               const char* value) {
  if (strcmp(name, "stdDeviation") == 0) {
    const char* ptr = value;
    char it[64];
    float args[2];
    int count = 0;
    
    while (*ptr && count < 2) {
      // skip delimiters
      while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
      if (!*ptr) break;
      
      ptr = SrSVGNode::ParseNumber(ptr, it, 64);
      args[count++] = SrSVGNode::Atof(it);
    }
    
    if (count == 1) {
      std_deviation_x_ = args[0];
      std_deviation_y_ = args[0];
    } else if (count == 2) {
      std_deviation_x_ = args[0];
      std_deviation_y_ = args[1];
    }
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeOffset::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "dx") == 0) {
    dx_ = Atof(value);
  } else if (strcmp(name, "dy") == 0) {
    dy_ = Atof(value);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeColorMatrix::ParseAndSetAttribute(const char* name,
                                              const char* value) {
  if (strcmp(name, "type") == 0) {
    type_ = value;
  } else if (strcmp(name, "values") == 0) {
    values_.clear();
    const char* ptr = value;
    char it[64];
    while (*ptr) {
      // skip delimiters
      while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
      if (!*ptr) break;
      
      ptr = SrSVGNode::ParseNumber(ptr, it, 64);
      values_.push_back(Atof(it));
    }
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeComposite::ParseAndSetAttribute(const char* name,
                                            const char* value) {
  if (strcmp(name, "in2") == 0) {
    in2_ = value;
  } else if (strcmp(name, "operator") == 0) {
    operator_ = value;
  } else if (strcmp(name, "k1") == 0) {
    k1_ = Atof(value);
  } else if (strcmp(name, "k2") == 0) {
    k2_ = Atof(value);
  } else if (strcmp(name, "k3") == 0) {
    k3_ = Atof(value);
  } else if (strcmp(name, "k4") == 0) {
    k4_ = Atof(value);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeBlend::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "in2") == 0) {
    in2_ = value;
  } else if (strcmp(name, "mode") == 0) {
    mode_ = value;
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

bool SrSVGFeFlood::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "flood-color") == 0) {
    if (flood_color_) release_serval_paint(flood_color_);
    flood_color_ = make_serval_paint(value);
  } else if (strcmp(name, "flood-opacity") == 0) {
    flood_opacity_ = Atof(value);
  } else {
    return SrSVGFilterPrimitive::ParseAndSetAttribute(name, value);
  }
  return true;
}

SrSVGFeFlood::~SrSVGFeFlood() {
  if (flood_color_) release_serval_paint(flood_color_);
}

}  // namespace element
}  // namespace svg
}  // namespace serval
