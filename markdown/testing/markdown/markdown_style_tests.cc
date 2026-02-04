// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "gtest/gtest.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/style/markdown_style_value.h"
namespace lynx::markdown::testing {

TEST(MarkdownLengthTest, Calculate) {
  constexpr MarkdownLengthContext ctx{
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 500,
  };
  auto length = MarkdownStyleValue::ParseValue("");
  EXPECT_EQ(length, nullptr);
  length = MarkdownStyleValue::ParseValue("100");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 100);
  length = MarkdownStyleValue::ParseValue("100px");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 100);
  length = MarkdownStyleValue::ParseValue("100em");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2000);
  length = MarkdownStyleValue::ParseValue("100rem");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 1000);
  length = MarkdownStyleValue::ParseValue("100%");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 500);
  length = MarkdownStyleValue::ParseValue("100vh");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2000);
  length = MarkdownStyleValue::ParseValue("100vw");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 1000);
  length = MarkdownStyleValue::ParseValue("calc(100)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 100);
  length = MarkdownStyleValue::ParseValue("calc(100px)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 100);
  length = MarkdownStyleValue::ParseValue("calc( 100em )");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2000);
  length = MarkdownStyleValue::ParseValue("calc( 100em +10px)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2010);
  length = MarkdownStyleValue::ParseValue("calc( 100em * 2)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 4000);
  length = MarkdownStyleValue::ParseValue("calc( 100em + 5vh)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2100);
  length = MarkdownStyleValue::ParseValue("calc(100em/2 )");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 1000);
  length = MarkdownStyleValue::ParseValue("calc( 50% - 50px )");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 200);
  length = MarkdownStyleValue::ParseValue("calc( 100em + 100em * 2 - 250em)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 1000);
  length = MarkdownStyleValue::ParseValue("calc((100em + 100em) * 2 - 250em)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 3000);
  length =
      MarkdownStyleValue::ParseValue("calc((100em + 100em) * 2 - 250em*2)");
  EXPECT_EQ(length->CalculateLengthValue(ctx), -2000);
  length =
      MarkdownStyleValue::ParseValue("calc(-((100em + 100em) * 2 - 250em*2))");
  EXPECT_EQ(length->CalculateLengthValue(ctx), 2000);
}

TEST(MarkdownPathTest, Create) {
  MarkdownPath path;
  MarkdownPath::Arc arc{};
  path.AddArc(arc);
  EXPECT_EQ(path.path_ops_.size(), 1ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kArc);
  EXPECT_EQ(path.path_ops_.back().data_.arc_.center_, arc.center_);
  const auto rect = RectF::MakeLTRB(10, 10, 100, 100);
  path.AddOval(rect);
  EXPECT_EQ(path.path_ops_.size(), 2ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kOval);
  EXPECT_EQ(path.path_ops_.back().data_.rect_, rect);
  MarkdownPath::RoundRect round_rect{};
  path.AddRoundRect(round_rect);
  EXPECT_EQ(path.path_ops_.size(), 3ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kRoundRect);
  EXPECT_EQ(path.path_ops_.back().data_.round_rect_.rect_, round_rect.rect_);
  PointF point{.x_ = 50, .y_ = 50};
  path.MoveTo(point);
  EXPECT_EQ(path.path_ops_.size(), 4ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kMoveTo);
  EXPECT_EQ(path.path_ops_.back().data_.point_, point);
  path.LineTo(point);
  EXPECT_EQ(path.path_ops_.size(), 5ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kLineTo);
  EXPECT_EQ(path.path_ops_.back().data_.point_, point);
  MarkdownPath::Cubic cubic{};
  path.CubicTo(cubic);
  EXPECT_EQ(path.path_ops_.size(), 6ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kCubicTo);
  MarkdownPath::Quad quad{{10, 10}, {20, 20}};
  path.QuadTo(quad);
  EXPECT_EQ(path.path_ops_.size(), 7ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kQuadTo);
  path.AddRect(rect);
  EXPECT_EQ(path.path_ops_.size(), 8ull);
  EXPECT_EQ(path.path_ops_.back().op_, MarkdownPath::PathOpType::kRect);
}

}  // namespace lynx::markdown::testing
