// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"

#include "markdown/view/markdown_view_measurer.h"

namespace lynx::markdown {

TEST(MarkdownViewMeasurerTest, MeasurePlainTextBasic) {
  MarkdownViewMeasurer measurer;
  measurer.SetSourceType(SourceType::kPlainText);
  measurer.SetContent("hello\n");
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;
  auto size = measurer.Measure(spec);
  EXPECT_GT(size.width_, 0);
  EXPECT_GT(size.height_, 0);
  EXPECT_LE(size.width_, 200);
}

TEST(MarkdownViewMeasurerTest, ContentRangeAffectsHeight) {
  MarkdownViewMeasurer measurer;
  measurer.SetSourceType(SourceType::kMarkdown);
  const std::string content = "line1\n\nline2";
  measurer.SetContent(content);
  MeasureSpec spec;
  spec.width_ = 200;
  spec.width_mode_ = tttext::LayoutMode::kDefinite;
  spec.height_ = MeasureSpec::LAYOUT_MAX_SIZE;
  spec.height_mode_ = tttext::LayoutMode::kIndefinite;
  auto full = measurer.Measure(spec);

  measurer.SetContentRange({0, 5});
  auto partial = measurer.Measure(spec);

  EXPECT_GT(full.height_, 0);
  EXPECT_GT(partial.height_, 0);
  EXPECT_LE(partial.height_, full.height_);
}

}  // namespace lynx::markdown
