// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGStop.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "element/SrSVGAnimation.h"

namespace serval {
namespace svg {
namespace element {

namespace {

bool ParseNumberWithSuffix(const std::string& value, double* number,
                           std::string* suffix) {
  if (!number || !suffix) {
    return false;
  }
  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str()) {
    return false;
  }
  *number = parsed;
  *suffix = end ? end : "";
  return true;
}

std::string AddAnimatedScalarValue(const std::string& base,
                                   const std::string& addition) {
  double base_number = 0.0;
  double addition_number = 0.0;
  std::string base_suffix;
  std::string addition_suffix;
  if (!ParseNumberWithSuffix(base, &base_number, &base_suffix) ||
      !ParseNumberWithSuffix(addition, &addition_number, &addition_suffix) ||
      base_suffix != addition_suffix) {
    return "";
  }
  std::ostringstream stream;
  stream << base_number + addition_number << base_suffix;
  return stream.str();
}

}  // namespace

bool SrSVGStop::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "id") == 0) {
    id_ = value;
    return true;
  } else if (strcmp(name, "offset") == 0) {
    stop_.offset = make_serval_length(value);
    return true;
  } else if (strcmp(name, "stop-color") == 0) {
    stop_.stopColor = make_serval_color(value);
    return true;
  } else if (strcmp(name, "stop-opacity") == 0) {
    stop_.stopOpacity = make_serval_length(value);
    return true;
  } else if (strcmp(name, "style") == 0) {
    ParseStyle(value);
    return true;
  }
  return false;
}

void SrSVGStop::StoreAttribute(const char* name, const char* value) {
  if (name && value) {
    base_attributes_[name] = value;
  }
}

void SrSVGStop::AddAnimation(SrSVGAnimation* animation) {
  if (animation && std::find(animations_.begin(), animations_.end(),
                             animation) == animations_.end()) {
    animations_.push_back(animation);
  }
}

void SrSVGStop::ApplyAnimations(double seconds, const IDMapper* id_mapper) {
  std::unordered_map<std::string, std::string> presentation_values;
  for (auto* animation : animations_) {
    if (!animation) {
      continue;
    }
    const std::string target_attribute = animation->TargetAttributeName();
    if (target_attribute.empty()) {
      continue;
    }
    auto presentation_it = presentation_values.find(target_attribute);
    if (presentation_it == presentation_values.end()) {
      auto base_it = base_attributes_.find(target_attribute);
      presentation_it =
          presentation_values
              .emplace(target_attribute,
                       base_it == base_attributes_.end() ? "" : base_it->second)
              .first;
    }

    SrSVGAnimation::Effect effect;
    if (!animation->Evaluate(seconds, id_mapper, presentation_it->second,
                             &effect) ||
        effect.attribute.empty()) {
      continue;
    }
    if (presentation_values.find(effect.attribute) ==
        presentation_values.end()) {
      auto base_it = base_attributes_.find(effect.attribute);
      presentation_values.emplace(
          effect.attribute,
          base_it == base_attributes_.end() ? "" : base_it->second);
    }
    if (animated_attributes_.find(effect.attribute) ==
        animated_attributes_.end()) {
      auto base_it = base_attributes_.find(effect.attribute);
      animated_attributes_[effect.attribute] =
          base_it == base_attributes_.end()
              ? std::optional<std::string>{}
              : std::optional<std::string>{base_it->second};
    }
    std::string value = effect.value;
    if (effect.additive) {
      auto& presentation = presentation_values[effect.attribute];
      if (!presentation.empty()) {
        const std::string added =
            AddAnimatedScalarValue(presentation, effect.value);
        if (!added.empty()) {
          value = added;
        }
      }
    }
    presentation_values[effect.attribute] = value;
    ParseAndSetAttribute(effect.attribute.c_str(), value.c_str());
  }
}

void SrSVGStop::RestoreAnimatedAttributes() {
  for (const auto& entry : animated_attributes_) {
    RestoreAnimatedAttribute(entry.first, entry.second);
  }
  animated_attributes_.clear();
}

void SrSVGStop::RestoreAnimatedAttribute(
    const std::string& name, const std::optional<std::string>& base_value) {
  if (base_value.has_value()) {
    ParseAndSetAttribute(name.c_str(), base_value->c_str());
    return;
  }
  ClearAnimatedAttribute(name);
}

void SrSVGStop::ClearAnimatedAttribute(const std::string& name) {
  if (name == "offset") {
    stop_.offset = (SrSVGLength){.value = 0.f, .unit = SR_SVG_UNITS_NUMBER};
  } else if (name == "stop-opacity") {
    stop_.stopOpacity =
        (SrSVGLength){.value = 1.f, .unit = SR_SVG_UNITS_NUMBER};
  } else if (name == "stop-color") {
    stop_.stopColor = make_serval_color("black");
  }
}

float SrSVGStop::offset(SrSVGRenderContext& context) const {
  if (stop_.offset.unit == SR_SVG_UNITS_PERCENTAGE) {
    return convert_serval_length_to_float(&(stop_.offset), &context,
                                          SR_SVG_LENGTH_TYPE_NUMERIC);
  } else if (stop_.offset.unit == SR_SVG_UNITS_NUMBER) {
    return stop_.offset.value;
  }
  return 0.f;
}

float SrSVGStop::opacity(SrSVGRenderContext& context) const {
  if (stop_.stopOpacity.unit == SR_SVG_UNITS_PERCENTAGE) {
    return convert_serval_length_to_float(&(stop_.stopOpacity), &context,
                                          SR_SVG_LENGTH_TYPE_NUMERIC);
  } else if (stop_.stopOpacity.unit == SR_SVG_UNITS_NUMBER) {
    return stop_.stopOpacity.value;
  }
  return 1.f;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
