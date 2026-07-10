// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "app/mac_app.h"

#include <utility>

#ifndef MARKDOWN_DEMO_FONT_ROOT
#define MARKDOWN_DEMO_FONT_ROOT ""
#endif

#ifndef MARKDOWN_DEMO_CASES_ROOT
#define MARKDOWN_DEMO_CASES_ROOT ""
#endif

int main(int, const char**) {
  serval::markdown::example::SkityDemoConfig config;
  config.font_root = MARKDOWN_DEMO_FONT_ROOT;
  config.cases_root = MARKDOWN_DEMO_CASES_ROOT;
  config.window_title = "Serval Markdown macOS Demo";
  serval::markdown::example::MacApp app(std::move(config));
  app.Run();
  return 0;
}
