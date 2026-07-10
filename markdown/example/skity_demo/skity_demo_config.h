// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_CONFIG_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_CONFIG_H_

#include <cstdint>
#include <filesystem>
#include <string>

#include "skity/skity.hpp"

namespace serval::markdown::example {

constexpr float kSkityDemoWidth = 720.f;
constexpr float kSkityDemoHeight = 100000.f;

struct SkityDemoConfig {
  std::filesystem::path font_root;
  std::filesystem::path cases_root;
  int32_t initial_width{900};
  int32_t initial_height{720};
  std::string window_title{"Serval Markdown Skity Demo"};
  skity::Vec4 clear_color{0.965f, 0.976f, 0.992f, 1.f};
};

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_CONFIG_H_
