// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "gtest/gtest.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/style/markdown_gradient.h"
#include "markdown/style/markdown_style_value.h"
namespace serval::markdown::testing {

namespace {

Value* ArrayAt(Value* value, size_t index) {
  if (value == nullptr || value->GetType() != ValueType::kArray) {
    return nullptr;
  }
  auto& array = value->AsArray();
  if (index >= array.size()) {
    return nullptr;
  }
  return array[index].get();
}

}  // namespace

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

TEST(MarkdownGradientTest, ParseLinearGradientDirection) {
  const MarkdownLengthContext ctx{
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 100,
  };
  auto value =
      ParseGradientValue("linear-gradient(to right, red 25%, blue 75%)", ctx);
  ASSERT_NE(value, nullptr);
  ASSERT_EQ(value->GetType(), ValueType::kArray);
  ASSERT_EQ(value->AsArray().size(), 2u);
  EXPECT_EQ(ArrayAt(value.get(), 0)->GetInt(),
            static_cast<int32_t>(MarkdownGradientType::kLinear));

  auto* data = ArrayAt(value.get(), 1);
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(data->AsArray().size(), 4u);
  EXPECT_DOUBLE_EQ(ArrayAt(data, 0)->GetDouble(), 90);
  EXPECT_EQ(ArrayAt(data, 3)->GetInt(),
            static_cast<int32_t>(MarkdownLinearGradientDirection::kToRight));

  auto* colors = ArrayAt(data, 1);
  ASSERT_NE(colors, nullptr);
  ASSERT_EQ(colors->AsArray().size(), 2u);
  EXPECT_EQ(ArrayAt(colors, 0)->GetLong(), 0xffff0000);
  EXPECT_EQ(ArrayAt(colors, 1)->GetLong(), 0xff0000ff);

  auto* stops = ArrayAt(data, 2);
  ASSERT_NE(stops, nullptr);
  ASSERT_EQ(stops->AsArray().size(), 2u);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 0)->GetDouble(), 25);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 1)->GetDouble(), 75);
}

TEST(MarkdownGradientTest, ParseLinearGradientClampStops) {
  const MarkdownLengthContext ctx{
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 100,
  };
  auto value = ParseGradientValue(
      "linear-gradient(to left, red -10%, blue 10%, green)", ctx);
  ASSERT_NE(value, nullptr);

  auto* data = ArrayAt(value.get(), 1);
  auto* colors = ArrayAt(data, 1);
  auto* stops = ArrayAt(data, 2);
  ASSERT_NE(colors, nullptr);
  ASSERT_NE(stops, nullptr);
  ASSERT_EQ(colors->AsArray().size(), 3u);
  ASSERT_EQ(stops->AsArray().size(), 3u);
  EXPECT_EQ(ArrayAt(colors, 0)->GetLong(), 0xff800080);
  EXPECT_EQ(ArrayAt(colors, 1)->GetLong(), 0xff0000ff);
  EXPECT_EQ(ArrayAt(colors, 2)->GetLong(), 0xff008000);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 0)->GetDouble(), 0);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 1)->GetDouble(), 10);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 2)->GetDouble(), 100);
}

TEST(MarkdownGradientTest, ParseRadialGradient) {
  const MarkdownLengthContext ctx{
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 100,
  };
  auto value = ParseGradientValue(
      "radial-gradient(ellipse 10px 5px at top, red, transparent)", ctx);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(ArrayAt(value.get(), 0)->GetInt(),
            static_cast<int32_t>(MarkdownGradientType::kRadial));

  auto* data = ArrayAt(value.get(), 1);
  auto* shape = ArrayAt(data, 0);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->AsArray().size(), 14u);
  EXPECT_EQ(ArrayAt(shape, 0)->GetInt(),
            static_cast<int32_t>(MarkdownRadialGradientShapeType::kEllipse));
  EXPECT_EQ(ArrayAt(shape, 1)->GetInt(),
            static_cast<int32_t>(MarkdownRadialGradientSizeType::kLength));
  EXPECT_EQ(ArrayAt(shape, 2)->GetInt(),
            -static_cast<int32_t>(MarkdownBackgroundPositionType::kCenter));
  EXPECT_EQ(ArrayAt(shape, 3)->GetInt(),
            static_cast<int32_t>(MarkdownBackgroundPositionType::kCenter));
  EXPECT_EQ(ArrayAt(shape, 4)->GetInt(),
            -static_cast<int32_t>(MarkdownBackgroundPositionType::kTop));
  EXPECT_EQ(ArrayAt(shape, 5)->GetInt(),
            static_cast<int32_t>(MarkdownBackgroundPositionType::kTop));
  EXPECT_EQ(ArrayAt(shape, 6)->GetInt(),
            static_cast<int32_t>(StyleValuePattern::kPx));
  EXPECT_DOUBLE_EQ(ArrayAt(shape, 7)->GetDouble(), 10);
  EXPECT_EQ(ArrayAt(shape, 8)->GetInt(),
            static_cast<int32_t>(StyleValuePattern::kPx));
  EXPECT_DOUBLE_EQ(ArrayAt(shape, 9)->GetDouble(), 5);
  EXPECT_DOUBLE_EQ(ArrayAt(shape, 10)->GetDouble(), 10);
  EXPECT_EQ(ArrayAt(shape, 11)->GetInt(),
            static_cast<int32_t>(MarkdownPlatformLengthUnit::kNumber));
  EXPECT_DOUBLE_EQ(ArrayAt(shape, 12)->GetDouble(), 5);
  EXPECT_EQ(ArrayAt(shape, 13)->GetInt(),
            static_cast<int32_t>(MarkdownPlatformLengthUnit::kNumber));
}

TEST(MarkdownGradientTest, ParseConicGradient) {
  const MarkdownLengthContext ctx{
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 100,
  };
  auto value = ParseGradientValue(
      "conic-gradient(from 50deg at top right, red 0%, blue 90%)", ctx);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(ArrayAt(value.get(), 0)->GetInt(),
            static_cast<int32_t>(MarkdownGradientType::kConic));

  auto* data = ArrayAt(value.get(), 1);
  ASSERT_NE(data, nullptr);
  ASSERT_EQ(data->AsArray().size(), 4u);
  EXPECT_DOUBLE_EQ(ArrayAt(data, 0)->GetDouble(), 50);

  auto* center = ArrayAt(data, 1);
  ASSERT_NE(center, nullptr);
  ASSERT_EQ(center->AsArray().size(), 4u);
  EXPECT_DOUBLE_EQ(ArrayAt(center, 0)->GetDouble(), 100);
  EXPECT_EQ(ArrayAt(center, 1)->GetInt(),
            static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage));
  EXPECT_DOUBLE_EQ(ArrayAt(center, 2)->GetDouble(), 0);
  EXPECT_EQ(ArrayAt(center, 3)->GetInt(),
            static_cast<int32_t>(MarkdownPlatformLengthUnit::kPercentage));

  auto* stops = ArrayAt(data, 3);
  ASSERT_NE(stops, nullptr);
  ASSERT_EQ(stops->AsArray().size(), 2u);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 0)->GetDouble(), 0);
  EXPECT_DOUBLE_EQ(ArrayAt(stops, 1)->GetDouble(), 90);
}

}  // namespace serval::markdown::testing
