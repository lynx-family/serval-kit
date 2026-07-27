// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "skity_demo/skity_demo_resource_loader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "skity_demo/skity_demo_font_manager.h"

namespace serval::markdown::example {
namespace {

class DemoImageDrawable final : public MarkdownDrawable {
 public:
  DemoImageDrawable(float desire_width, float desire_height, float max_width,
                    float max_height, float border_radius)
      : desire_width_(desire_width),
        desire_height_(desire_height),
        max_width_(max_width),
        max_height_(max_height),
        border_radius_(border_radius) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    const auto right = x + measure_result_.width_;
    const auto bottom = y + measure_result_.height_;
    const auto radius = std::clamp(
        border_radius_, 0.f,
        std::min(measure_result_.width_, measure_result_.height_) * 0.5f);
    if (radius > 0) {
      auto fill_painter = canvas->CreatePainter();
      fill_painter->SetFillColor(0xffe5e7eb);
      canvas->DrawRoundRect(x, y, right, bottom, radius, fill_painter.get());
    } else {
      canvas->FillRect(x, y, right, bottom, 0xffe5e7eb);
    }
    auto painter = canvas->CreatePainter();
    painter->SetStrokeColor(0xffcbd5e1);
    painter->SetStrokeWidth(1);
    if (radius > 0) {
      canvas->DrawRoundRect(x, y, right, bottom, radius, painter.get());
    } else {
      canvas->DrawRect(x, y, right, bottom, painter.get());
    }
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    constexpr float kFallbackWidth = 160.f;
    constexpr float kFallbackHeight = 90.f;
    float width = desire_width_ > 0 ? desire_width_ : kFallbackWidth;
    float height = desire_height_ > 0 ? desire_height_ : kFallbackHeight;

    if (max_width_ > 0) {
      width = std::min(width, max_width_);
    }
    if (max_height_ > 0) {
      height = std::min(height, max_height_);
    }
    if (spec.width_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.width_ > 0) {
      width = std::min(width, spec.width_);
    }
    if (spec.height_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.height_ > 0) {
      height = std::min(height, spec.height_);
    }

    width = std::max(1.f, width);
    height = std::max(1.f, height);
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }

 private:
  float desire_width_{0};
  float desire_height_{0};
  float max_width_{0};
  float max_height_{0};
  float border_radius_{0};
};

class DemoInlineViewDrawable final : public MarkdownDrawable {
 public:
  DemoInlineViewDrawable(std::string id_selector, float max_width,
                         float max_height)
      : id_selector_(std::move(id_selector)),
        max_width_(max_width),
        max_height_(max_height) {}

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    const uint32_t fill_color =
        id_selector_ == "block" ? 0xffdbeafe : 0xffccfbf1;
    const uint32_t stroke_color =
        id_selector_ == "block" ? 0xff60a5fa : 0xff14b8a6;
    canvas->FillRect(x, y, x + measure_result_.width_,
                     y + measure_result_.height_, fill_color);
    auto painter = canvas->CreatePainter();
    painter->SetStrokeColor(stroke_color);
    painter->SetStrokeWidth(1);
    canvas->DrawRect(x, y, x + measure_result_.width_,
                     y + measure_result_.height_, painter.get());
  }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override {
    float width = id_selector_ == "block" ? max_width_ : max_width_ * 0.22f;
    float height = id_selector_ == "block" ? 72.f : 30.f;
    if (id_selector_.find("cursor") != std::string::npos) {
      width = 10.f;
      height = 10.f;
    }
    if (max_width_ > 0) {
      width = std::min(width, max_width_);
    }
    if (max_height_ > 0) {
      height = std::min(height, max_height_);
    }
    if (spec.width_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.width_ > 0) {
      width = std::min(width, spec.width_);
    }
    if (spec.height_mode_ != tttext::LayoutMode::kIndefinite &&
        spec.height_ > 0) {
      height = std::min(height, spec.height_);
    }
    width = std::max(1.f, width);
    height = std::max(1.f, height);
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }

 private:
  std::string id_selector_;
  float max_width_{0};
  float max_height_{0};
};

}  // namespace

SkityDemoResourceLoader::SkityDemoResourceLoader(
    SkityDemoFontManager* font_manager)
    : font_manager_(font_manager) {}

std::shared_ptr<MarkdownDrawable> SkityDemoResourceLoader::LoadImage(
    const char* src, float desire_width, float desire_height, float max_width,
    float max_height, float border_radius) {
  if (src == nullptr || std::string(src) == "invalid" ||
      std::string(src) == "http://invalid") {
    return nullptr;
  }
  return std::make_shared<DemoImageDrawable>(
      desire_width, desire_height, max_width, max_height, border_radius);
}

std::shared_ptr<MarkdownDrawable> SkityDemoResourceLoader::LoadInlineView(
    const char* id_selector, float max_width, float max_height) {
  return std::make_shared<DemoInlineViewDrawable>(
      id_selector == nullptr ? "" : id_selector, max_width, max_height);
}

void* SkityDemoResourceLoader::LoadFont(const char* family,
                                        MarkdownFontWeight weight) {
  (void)weight;
  if (font_manager_ == nullptr) {
    return nullptr;
  }
  return font_manager_->GetPlatformFont(
      family == nullptr || family[0] == '\0' ? "sans-serif" : family);
}

MarkdownReplacementView SkityDemoResourceLoader::LoadReplacementView(
    void* ud, int32_t id, float max_width, float max_height) {
  (void)ud;
  (void)id;
  (void)max_width;
  (void)max_height;
  return {};
}

}  // namespace serval::markdown::example
