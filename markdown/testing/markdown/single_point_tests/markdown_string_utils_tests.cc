// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gtest/gtest.h"
#include "markdown/utils/markdown_string_utils.h"

namespace serval::markdown {

TEST(MarkdownStringUtilsTest, TrimAndToLower) {
  EXPECT_EQ(Trim(" \tHello World\n"), "Hello World");
  EXPECT_EQ(ToLower("LiNeAr-GrAdIeNt"), "linear-gradient");
}

TEST(MarkdownStringUtilsTest, StringToFloatAcceptsStringView) {
  float value = 0;
  EXPECT_TRUE(StringToFloat(std::string_view("12.5"), value, true));
  EXPECT_FLOAT_EQ(value, 12.5f);

  EXPECT_FALSE(StringToFloat(std::string_view("nan"), value, true));
}

TEST(MarkdownStringUtilsTest, FindMatchingParenthesisHandlesNestedContent) {
  constexpr std::string_view value = "func(rgba(255, 0, 0, 0.5), test)";
  const auto open = value.find('(');
  ASSERT_NE(open, std::string_view::npos);
  EXPECT_EQ(FindMatchingParenthesis(value, open), value.size() - 1);
  EXPECT_EQ(FindMatchingParenthesis("(unclosed", 0), std::string_view::npos);
}

TEST(MarkdownStringUtilsTest, SplitTopLevelSkipsNestedDelimiters) {
  const auto parts =
      SplitTopLevel("rgba(255, 0, 0, 0.5), url(test,a.png), blue", ',');
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "rgba(255, 0, 0, 0.5)");
  EXPECT_EQ(parts[1], "url(test,a.png)");
  EXPECT_EQ(parts[2], "blue");

  EXPECT_TRUE(SplitTopLevel("blue,,red", ',').empty());
}

}  // namespace serval::markdown
