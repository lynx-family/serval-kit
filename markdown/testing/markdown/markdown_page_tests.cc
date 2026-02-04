// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "gtest/gtest.h"
#include "markdown/element/markdown_page.h"
namespace lynx::markdown::testing {
TEST(MarkdownPageTest, ScrollState) {
  MarkdownPage page;
  auto region_ptr = std::make_unique<MarkdownPageParagraphRegion>();
  auto* region = region_ptr.get();
  page.AddRegion(std::move(region_ptr));
  region->scroll_x_ = true;
  region->scroll_x_offset_ = 10;
  region->element_ = std::make_shared<MarkdownParagraphElement>();
  auto state = page.GetScrollState();
  ASSERT_EQ(state.size(), 1.f);
  ASSERT_EQ(state.front().scroll_offset_, 10.f);
  region->scroll_x_offset_ = 0;
  page.ApplyScrollState(state);
  ASSERT_EQ(region->scroll_x_offset_, 10.f);
  state.emplace_back(ScrollState{2, MarkdownElementType::kParagraph, 20});
  page.ApplyScrollState(state);
}
}  // namespace lynx::markdown::testing
