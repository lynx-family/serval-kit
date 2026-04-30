// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>

#include "gtest/gtest.h"
#include "markdown/draw/markdown_path.h"
#include "markdown/style/markdown_color.h"
#include "markdown/style/markdown_gradient.h"
#include "markdown/style/markdown_style_value.h"
#include "testing/markdown/markdown_tests_platform.h"
#include "testing/markdown/mock_markdown_canvas.h"
#include "testing/markdown/mock_markdown_resource_loader.h"
namespace serval::markdown::testing {

namespace {

class DeferredImageMarkdownResourceLoader final
    : public MockMarkdownResourceLoader {
 public:
  std::shared_ptr<MarkdownDrawable> LoadImage(const char* src,
                                              float desire_width,
                                              float desire_height,
                                              float max_width, float max_height,
                                              float radius) override {
    load_image_count_++;
    if (!image_ready_) {
      return nullptr;
    }
    return MockMarkdownResourceLoader::LoadImage(
        src, desire_width, desire_height, max_width, max_height, radius);
  }

  bool image_ready_{false};
  int load_image_count_{0};
};

constexpr MarkdownLengthContext MakeGradientTestContext() {
  return {
      .screen_width_ = 1000,
      .screen_height_ = 2000,
      .font_size_ = 20,
      .root_font_size_ = 10,
      .base_length_ = 100,
  };
}

}  // namespace

TEST(MarkdownColorTest, ParseCssFormats) {
  uint32_t color = 0;
  EXPECT_TRUE(MarkdownColor::Parse("#abc", &color));
  EXPECT_EQ(color, 0xffaabbccu);

  EXPECT_TRUE(MarkdownColor::Parse("0000ff", &color));
  EXPECT_EQ(color, 0xff0000ffu);

  EXPECT_TRUE(MarkdownColor::Parse("rgba(255, 245, 157, 0.5)", &color));
  EXPECT_EQ(color, 0x80fff59du);

  EXPECT_TRUE(MarkdownColor::Parse("hsla(240, 100%, 50%, 0.25)", &color));
  EXPECT_EQ(color, 0x400000ffu);

  EXPECT_TRUE(MarkdownColor::Parse("purple", &color));
  EXPECT_EQ(color, 0xff800080u);

  EXPECT_FALSE(MarkdownColor::Parse("not-a-color", &color));
}

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
  const auto ctx = MakeGradientTestContext();
  auto drawable =
      ParseGradientValue("linear-gradient(to right, red 25%, blue 75%)", ctx);
  ASSERT_NE(drawable, nullptr);
  auto linear =
      std::static_pointer_cast<MarkdownLinearGradientDrawable>(drawable);
  ASSERT_NE(linear, nullptr);
  EXPECT_FLOAT_EQ(linear->GetAngle(), 90);
  EXPECT_EQ(linear->GetDirection(), MarkdownLinearGradientDirection::kToRight);

  const auto& gradient = linear->GetGradient();
  ASSERT_EQ(gradient.colors.size(), 2u);
  EXPECT_EQ(gradient.colors[0], 0xffff0000u);
  EXPECT_EQ(gradient.colors[1], 0xff0000ffu);
  ASSERT_EQ(gradient.stops.size(), 2u);
  EXPECT_FLOAT_EQ(gradient.stops[0], 0.25f);
  EXPECT_FLOAT_EQ(gradient.stops[1], 0.75f);

  const auto measure =
      linear->Measure({.width_ = 100,
                       .width_mode_ = tttext::LayoutMode::kDefinite,
                       .height_ = 50,
                       .height_mode_ = tttext::LayoutMode::kDefinite});
  EXPECT_FLOAT_EQ(measure.width_, 100);
  EXPECT_FLOAT_EQ(measure.height_, 50);
}

TEST(MarkdownGradientTest, ParseLinearGradientClampStops) {
  const auto ctx = MakeGradientTestContext();
  auto drawable = ParseGradientValue(
      "linear-gradient(to left, red -10%, blue 10%, green)", ctx);
  ASSERT_NE(drawable, nullptr);
  auto linear =
      std::static_pointer_cast<MarkdownLinearGradientDrawable>(drawable);
  ASSERT_NE(linear, nullptr);

  const auto& gradient = linear->GetGradient();
  ASSERT_EQ(gradient.colors.size(), 3u);
  ASSERT_EQ(gradient.stops.size(), 3u);
  EXPECT_EQ(gradient.colors[0], 0xff800080u);
  EXPECT_EQ(gradient.colors[1], 0xff0000ffu);
  EXPECT_EQ(gradient.colors[2], 0xff008000u);
  EXPECT_FLOAT_EQ(gradient.stops[0], 0);
  EXPECT_FLOAT_EQ(gradient.stops[1], 0.1f);
  EXPECT_FLOAT_EQ(gradient.stops[2], 1);
}

TEST(MarkdownGradientTest, ParseLinearGradientWithSharedColorParser) {
  const auto ctx = MakeGradientTestContext();
  auto drawable = ParseGradientValue(
      "linear-gradient(to right, rgba(255, 245, 157, 0.5), #00f, purple)", ctx);
  ASSERT_NE(drawable, nullptr);
  auto linear =
      std::static_pointer_cast<MarkdownLinearGradientDrawable>(drawable);
  ASSERT_NE(linear, nullptr);

  const auto& gradient = linear->GetGradient();
  ASSERT_EQ(gradient.colors.size(), 3u);
  EXPECT_EQ(gradient.colors[0], 0x80fff59du);
  EXPECT_EQ(gradient.colors[1], 0xff0000ffu);
  EXPECT_EQ(gradient.colors[2], 0xff800080u);
}

TEST(MarkdownGradientTest, DrawLinearGradientUsesDrawBounds) {
  const auto ctx = MakeGradientTestContext();
  auto markdown_context = CreateTestMarkdownSharedContext();
  auto drawable = ParseGradientValue(
      "linear-gradient(45deg, red 0%, blue 100%)", ctx, markdown_context.get());
  ASSERT_NE(drawable, nullptr);
  auto linear =
      std::static_pointer_cast<MarkdownLinearGradientDrawable>(drawable);
  ASSERT_NE(linear, nullptr);

  linear->Measure({.width_ = 100,
                   .width_mode_ = tttext::LayoutMode::kDefinite,
                   .height_ = 100,
                   .height_mode_ = tttext::LayoutMode::kDefinite});

  MockMarkdownCanvas canvas(nullptr, nullptr);
  linear->DrawOnRect(&canvas, RectF::MakeLTWH(10, 20, 100, 100));
  const auto& ops = canvas.GetJson();
  ASSERT_EQ(ops.Size(), 1u);
  ASSERT_TRUE(ops[0].HasMember("gradient"));
  const auto& gradient = ops[0]["gradient"];
  EXPECT_NEAR(gradient["start"]["x"].GetFloat(), 10, 0.001);
  EXPECT_NEAR(gradient["start"]["y"].GetFloat(), 120, 0.001);
  EXPECT_NEAR(gradient["end"]["x"].GetFloat(), 110, 0.001);
  EXPECT_NEAR(gradient["end"]["y"].GetFloat(), 20, 0.001);
}

TEST(MarkdownGradientTest, ParseBackgroundImageUrl) {
  const auto ctx = MakeGradientTestContext();
  MockMarkdownResourceLoader loader;
  auto drawable = ParseBackgroundDrawableValue("url(test.png)", &loader, ctx);
  ASSERT_NE(drawable, nullptr);
  auto image =
      std::static_pointer_cast<MarkdownBackgroundImageDrawable>(drawable);
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->GetUrl(), "test.png");
}

TEST(MarkdownGradientTest, ParseBackgroundImageUrlAllowsDeferredLoad) {
  const auto ctx = MakeGradientTestContext();
  DeferredImageMarkdownResourceLoader loader;
  auto drawable = ParseBackgroundDrawableValue("url(test.png)", &loader, ctx);
  ASSERT_NE(drawable, nullptr);
  EXPECT_EQ(loader.load_image_count_, 0);

  auto image =
      std::static_pointer_cast<MarkdownBackgroundImageDrawable>(drawable);
  ASSERT_NE(image, nullptr);

  const MeasureSpec spec{
      .width_ = 120,
      .width_mode_ = tttext::LayoutMode::kDefinite,
      .height_ = 40,
      .height_mode_ = tttext::LayoutMode::kDefinite,
  };
  const auto first = image->Measure(spec);
  EXPECT_EQ(loader.load_image_count_, 1);
  EXPECT_FLOAT_EQ(first.width_, 120);
  EXPECT_FLOAT_EQ(first.height_, 40);

  loader.image_ready_ = true;
  image->Measure(spec);
  EXPECT_EQ(loader.load_image_count_, 2);
}

TEST(MarkdownGradientTest, RejectRadialGradient) {
  const auto ctx = MakeGradientTestContext();
  auto value = ParseGradientValue(
      "radial-gradient(ellipse 10px 5px at top, red, transparent)", ctx);
  EXPECT_EQ(value, nullptr);
}

TEST(MarkdownGradientTest, RejectConicGradient) {
  const auto ctx = MakeGradientTestContext();
  auto value = ParseGradientValue(
      "conic-gradient(from 50deg at top right, red 0%, blue 90%)", ctx);
  EXPECT_EQ(value, nullptr);
}

}  // namespace serval::markdown::testing
