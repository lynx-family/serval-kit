// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/style/markdown_gradient.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "markdown/draw/markdown_path.h"
#include "markdown/element/markdown_context.h"
#include "markdown/parser/markdown_resource_loader.h"
#include "markdown/style/markdown_color.h"
#include "markdown/utils/markdown_string_utils.h"

namespace serval::markdown {

namespace {

constexpr std::string_view kLinearPrefix = "linear-gradient(";
constexpr std::string_view kUrlPrefix = "url(";
constexpr size_t kInvalidIndex = static_cast<size_t>(-1);
constexpr float kMinColorStop = 0.f;
constexpr float kMaxColorStop = 1.f;

struct ColorStop {
  uint32_t color{0};
  bool has_position{false};
  float position{0};
};

enum class ParseStatus {
  kNotFound,
  kSuccess,
  kInvalid,
};

bool ExtractColorStop(std::string_view value, ColorStop* stop) {
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  std::string_view color_part;
  std::string_view remainder;
  if (trimmed[0] == '#') {
    size_t end = 0;
    while (end < trimmed.size() && trimmed[end] != ' ' &&
           trimmed[end] != '\t' && trimmed[end] != '\n' &&
           trimmed[end] != '\r' && trimmed[end] != '\f') {
      end++;
    }
    color_part = trimmed.substr(0, end);
    remainder = trimmed.substr(end);
  } else {
    const auto open = trimmed.find('(');
    const auto first_space = trimmed.find_first_of(" \t\n\r\f");
    if (open != std::string_view::npos &&
        (first_space == std::string_view::npos || open < first_space)) {
      const auto close = FindMatchingParenthesis(trimmed, open);
      if (close == std::string_view::npos) {
        return false;
      }
      color_part = trimmed.substr(0, close + 1);
      remainder = trimmed.substr(close + 1);
    } else {
      const auto end =
          first_space == std::string_view::npos ? trimmed.size() : first_space;
      color_part = trimmed.substr(0, end);
      remainder = trimmed.substr(end);
    }
  }

  if (!MarkdownColor::Parse(color_part, &stop->color)) {
    return false;
  }

  remainder = Trim(remainder);
  if (remainder.empty()) {
    stop->has_position = false;
    return true;
  }

  float position = 0;
  if (!remainder.empty() && remainder.back() == '%') {
    if (!StringToFloat(remainder.substr(0, remainder.size() - 1), position,
                       true)) {
      return false;
    }
    position /= 100.f;
  } else {
    if (!StringToFloat(remainder, position, true)) {
      return false;
    }
  }
  stop->has_position = true;
  stop->position = position;
  return true;
}

void ClampColorStopsAtFront(std::vector<uint32_t>* colors,
                            std::vector<float>* stops,
                            uint32_t first_positive_index) {
  const float prev_stop = (*stops)[first_positive_index - 1];
  const uint32_t prev_color = (*colors)[first_positive_index - 1];
  const float current_stop = (*stops)[first_positive_index];
  const uint32_t current_color = (*colors)[first_positive_index];
  (*colors)[first_positive_index - 1] = MarkdownColor::Interpolate(
      prev_color, current_color, prev_stop, current_stop, kMinColorStop);
  (*stops)[first_positive_index - 1] = kMinColorStop;
  if (first_positive_index > 1) {
    colors->erase(colors->begin(), colors->begin() + first_positive_index - 1);
    stops->erase(stops->begin(), stops->begin() + first_positive_index - 1);
  }
}

void ClampColorStopsAtBack(std::vector<uint32_t>* colors,
                           std::vector<float>* stops, uint32_t tail_index) {
  const float prev_stop = (*stops)[tail_index - 1];
  const uint32_t prev_color = (*colors)[tail_index - 1];
  const float current_stop = (*stops)[tail_index];
  const uint32_t current_color = (*colors)[tail_index];
  (*colors)[tail_index] = MarkdownColor::Interpolate(
      prev_color, current_color, prev_stop, current_stop, kMaxColorStop);
  (*stops)[tail_index] = kMaxColorStop;
  if (tail_index + 1 < stops->size()) {
    colors->erase(colors->begin() + tail_index + 1, colors->end());
    stops->erase(stops->begin() + tail_index + 1, stops->end());
  }
}

void ClampColorStops(std::vector<uint32_t>* colors, std::vector<float>* stops) {
  if (stops->size() < 2) {
    return;
  }
  if (stops->front() < kMinColorStop) {
    const auto it = std::find_if(stops->begin(), stops->end(), [](float value) {
      return value >= kMinColorStop;
    });
    if (it != stops->begin() && it != stops->end()) {
      ClampColorStopsAtFront(colors, stops,
                             static_cast<uint32_t>(it - stops->begin()));
    }
  }
  if (!stops->empty() && stops->back() > kMaxColorStop) {
    const auto it = std::find_if(stops->begin(), stops->end(), [](float value) {
      return value >= kMaxColorStop;
    });
    if (it != stops->begin() && it != stops->end()) {
      ClampColorStopsAtBack(colors, stops,
                            static_cast<uint32_t>(it - stops->begin()));
    }
  }
}

bool ParseColorStopList(const std::vector<std::string_view>& args,
                        size_t start_index, std::vector<uint32_t>* colors,
                        std::vector<float>* stops) {
  size_t position_begin_index = kInvalidIndex;
  float position_begin_value = kMinColorStop;
  for (size_t index = start_index; index < args.size(); index++) {
    ColorStop stop;
    if (!ExtractColorStop(args[index], &stop)) {
      return false;
    }
    colors->emplace_back(stop.color);
    if (!stop.has_position) {
      if (position_begin_index == kInvalidIndex) {
        position_begin_index = colors->size() - 1;
      }
      continue;
    }
    if (position_begin_index != kInvalidIndex) {
      const auto current_index = colors->size() - 1;
      if (position_begin_index > 0) {
        position_begin_value = (*stops)[position_begin_index - 1];
      } else {
        position_begin_index++;
        stops->emplace_back(kMinColorStop);
      }
      const float step =
          (stop.position - position_begin_value) /
          static_cast<float>(current_index - position_begin_index + 1);
      for (size_t fill_index = position_begin_index; fill_index < current_index;
           fill_index++) {
        stops->emplace_back(
            position_begin_value +
            static_cast<float>(fill_index - position_begin_index + 1) * step);
      }
    }
    stops->emplace_back(stop.position);
    position_begin_index = kInvalidIndex;
  }

  const int32_t fill_step =
      static_cast<int32_t>(colors->size() - stops->size());
  if (!stops->empty() && fill_step > 0) {
    const float begin_value = stops->back();
    const float step_value = (kMaxColorStop - begin_value) / fill_step;
    for (int32_t i = 1; i < fill_step; i++) {
      stops->emplace_back(begin_value + step_value * i);
    }
    stops->emplace_back(kMaxColorStop);
  }

  ClampColorStops(colors, stops);
  return colors->size() >= 2 &&
         (stops->empty() || stops->size() == colors->size());
}

bool ParseAngle(std::string_view value, float* angle) {
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  auto parse_angle_with_unit = [&](std::string_view suffix,
                                   float factor) -> bool {
    if (trimmed.size() < suffix.size() ||
        trimmed.substr(trimmed.size() - suffix.size()) != suffix) {
      return false;
    }
    float number = 0;
    if (!StringToFloat(trimmed.substr(0, trimmed.size() - suffix.size()),
                       number, true)) {
      return false;
    }
    *angle = number * factor;
    return true;
  };
  if (parse_angle_with_unit("deg", 1.f)) {
    return true;
  }
  if (parse_angle_with_unit("grad", 360.f / 400.f)) {
    return true;
  }
  if (parse_angle_with_unit("rad", 180.f / static_cast<float>(M_PI))) {
    return true;
  }
  if (parse_angle_with_unit("turn", 360.f)) {
    return true;
  }
  if (trimmed == "0") {
    *angle = 0;
    return true;
  }
  return false;
}

bool MatchesDirectionKey(std::string_view value, const char* key) {
  size_t key_index = 0;
  for (const char c : value) {
    if (c == ' ' || c == '\t') {
      continue;
    }
    if (key[key_index] == '\0' || c != key[key_index]) {
      return false;
    }
    key_index++;
  }
  return key[key_index] == '\0';
}

ParseStatus ParseLinearPrelude(std::string_view value, float* angle,
                               MarkdownLinearGradientDirection* direction) {
  const auto trimmed = Trim(value);
  if (trimmed.empty()) {
    return ParseStatus::kNotFound;
  }
  if (ParseAngle(trimmed, angle)) {
    *direction = MarkdownLinearGradientDirection::kAngle;
    return ParseStatus::kSuccess;
  }
  if (!BeginsWith(trimmed, "to")) {
    return ParseStatus::kNotFound;
  }
  struct Direction {
    const char* key;
    float angle;
    MarkdownLinearGradientDirection direction;
  };
  static constexpr Direction kDirections[] = {
      {"top", 0.f, MarkdownLinearGradientDirection::kToTop},
      {"bottom", 180.f, MarkdownLinearGradientDirection::kToBottom},
      {"left", 270.f, MarkdownLinearGradientDirection::kToLeft},
      {"right", 90.f, MarkdownLinearGradientDirection::kToRight},
      {"topleft", 315.f, MarkdownLinearGradientDirection::kToTopLeft},
      {"lefttop", 315.f, MarkdownLinearGradientDirection::kToTopLeft},
      {"topright", 45.f, MarkdownLinearGradientDirection::kToTopRight},
      {"righttop", 45.f, MarkdownLinearGradientDirection::kToTopRight},
      {"bottomleft", 225.f, MarkdownLinearGradientDirection::kToBottomLeft},
      {"leftbottom", 225.f, MarkdownLinearGradientDirection::kToBottomLeft},
      {"bottomright", 135.f, MarkdownLinearGradientDirection::kToBottomRight},
      {"rightbottom", 135.f, MarkdownLinearGradientDirection::kToBottomRight},
  };
  const auto key = trimmed.substr(2);
  for (const auto& candidate : kDirections) {
    if (MatchesDirectionKey(key, candidate.key)) {
      *angle = candidate.angle;
      *direction = candidate.direction;
      return ParseStatus::kSuccess;
    }
  }
  return ParseStatus::kInvalid;
}

bool ParseLinearGradient(std::string_view args, float* angle,
                         MarkdownLinearGradientDirection* direction,
                         std::vector<uint32_t>* colors,
                         std::vector<float>* stops) {
  const auto parts = SplitTopLevel(args, ',');
  if (parts.size() < 2) {
    return false;
  }
  *angle = 180.f;
  *direction = MarkdownLinearGradientDirection::kToBottom;
  size_t color_start_index = 0;
  const auto prelude = ParseLinearPrelude(parts[0], angle, direction);
  if (prelude == ParseStatus::kInvalid) {
    return false;
  }
  if (prelude == ParseStatus::kSuccess) {
    color_start_index = 1;
  }

  return ParseColorStopList(parts, color_start_index, colors, stops);
}

std::string ExtractUrl(std::string_view value) {
  const auto trimmed = Trim(value);
  if (trimmed.empty() || !BeginsWith(trimmed, kUrlPrefix) ||
      trimmed.back() != ')') {
    return "";
  }
  auto content = Trim(trimmed.substr(kUrlPrefix.size(),
                                     trimmed.size() - kUrlPrefix.size() - 1));
  if (content.size() >= 2 &&
      ((content.front() == '"' && content.back() == '"') ||
       (content.front() == '\'' && content.back() == '\''))) {
    content = content.substr(1, content.size() - 2);
  }
  return std::string(Trim(content));
}

float SafeLength(float value) {
  return value > 0 ? value : 1.f;
}

float ToRadians(float angle) {
  return angle * static_cast<float>(M_PI) / 180.f;
}

void UpdateGradientPoints(float width, float height, float angle,
                          MarkdownLinearGradientDirection direction,
                          PointF* start, PointF* end) {
  const float safe_width = SafeLength(width);
  const float safe_height = SafeLength(height);
  const float mul = 2.f * safe_width * safe_height /
                    (safe_width * safe_width + safe_height * safe_height);
  switch (direction) {
    case MarkdownLinearGradientDirection::kToTop:
      *start = {.x_ = 0, .y_ = safe_height};
      *end = {.x_ = 0, .y_ = 0};
      return;
    case MarkdownLinearGradientDirection::kToBottom:
      *start = {.x_ = 0, .y_ = 0};
      *end = {.x_ = 0, .y_ = safe_height};
      return;
    case MarkdownLinearGradientDirection::kToLeft:
      *start = {.x_ = safe_width, .y_ = 0};
      *end = {.x_ = 0, .y_ = 0};
      return;
    case MarkdownLinearGradientDirection::kToRight:
      *start = {.x_ = 0, .y_ = 0};
      *end = {.x_ = safe_width, .y_ = 0};
      return;
    case MarkdownLinearGradientDirection::kToTopRight:
      *start = {.x_ = safe_width - safe_height * mul, .y_ = safe_width * mul};
      *end = {.x_ = safe_width, .y_ = 0};
      return;
    case MarkdownLinearGradientDirection::kToTopLeft:
      *start = {.x_ = safe_height * mul, .y_ = safe_width * mul};
      *end = {.x_ = 0, .y_ = 0};
      return;
    case MarkdownLinearGradientDirection::kToBottomRight:
      *start = {.x_ = 0, .y_ = 0};
      *end = {.x_ = safe_height * mul, .y_ = safe_width * mul};
      return;
    case MarkdownLinearGradientDirection::kToBottomLeft:
      *start = {.x_ = safe_width, .y_ = 0};
      *end = {.x_ = safe_width - safe_height * mul, .y_ = safe_width * mul};
      return;
    case MarkdownLinearGradientDirection::kNone:
    case MarkdownLinearGradientDirection::kAngle:
      break;
  }

  PointF center{.x_ = safe_width * 0.5f, .y_ = safe_height * 0.5f};
  PointF corner{};
  const float radians = ToRadians(angle);
  const float sin_value = std::sin(radians);
  const float cos_value = std::cos(radians);
  const float tan_value = std::tan(radians);
  if (sin_value >= 0 && cos_value >= 0) {
    corner = {.x_ = safe_width, .y_ = 0};
  } else if (sin_value >= 0 && cos_value < 0) {
    corner = {.x_ = safe_width, .y_ = safe_height};
  } else if (sin_value < 0 && cos_value < 0) {
    corner = {.x_ = 0, .y_ = safe_height};
  } else {
    corner = {.x_ = 0, .y_ = 0};
  }
  const float tmp =
      center.y_ - corner.y_ - tan_value * center.x_ + tan_value * corner.x_;
  end->x_ = center.x_ + sin_value * tmp / (sin_value * tan_value + cos_value);
  end->y_ = center.y_ - tmp / (tan_value * tan_value + 1.f);
  start->x_ = 2.f * center.x_ - end->x_;
  start->y_ = 2.f * center.y_ - end->y_;
}

}  // namespace

MarkdownLinearGradientDrawable::MarkdownLinearGradientDrawable(
    MarkdownContext* context, float angle,
    MarkdownLinearGradientDirection direction, std::vector<uint32_t> colors,
    std::vector<float> stops)
    : angle_(angle), direction_(direction), context_(context) {
  gradient_.colors = std::move(colors);
  gradient_.stops = std::move(stops);
}

MarkdownBackgroundImageDrawable::MarkdownBackgroundImageDrawable(
    MarkdownContext* context, MarkdownResourceLoader* loader, std::string url,
    std::shared_ptr<MarkdownDrawable> image)
    : loader_(loader),
      url_(std::move(url)),
      image_(std::move(image)),
      context_(context) {}

void MarkdownLinearGradientDrawable::DrawOnRect(tttext::ICanvasHelper* canvas,
                                                RectF rect,
                                                tttext::Painter* painter) {
  auto* canvas_extend =
      context_ == nullptr ? nullptr : context_->GetMarkdownCanvasExtend(canvas);
  if (canvas_extend == nullptr) {
    return;
  }
  tttext::Painter default_painter;
  if (painter == nullptr) {
    default_painter.SetFillColor(tttext::TTColor::WHITE);
    painter = &default_painter;
  }
  if (rect.GetWidth() == 0 && rect.GetHeight() == 0) {
    return;
  }
  auto gradient = MakeGradientForBounds(rect);
  canvas_extend->DrawLinearGradientOnRect(&gradient, rect, painter);
}

void MarkdownLinearGradientDrawable::DrawOnPath(tttext::ICanvasHelper* canvas,
                                                MarkdownPath* path,
                                                RectF bounds,
                                                tttext::Painter* painter) {
  if (path == nullptr || painter == nullptr) {
    return;
  }
  auto* canvas_extend =
      context_ == nullptr ? nullptr : context_->GetMarkdownCanvasExtend(canvas);
  if (canvas_extend == nullptr) {
    return;
  }
  auto gradient = MakeGradientForBounds(bounds);
  canvas_extend->DrawLinearGradientOnPath(&gradient, path, bounds, painter);
}

MeasureResult MarkdownLinearGradientDrawable::OnMeasure(MeasureSpec spec) {
  const float width = spec.width_mode_ == tttext::LayoutMode::kIndefinite
                          ? MeasureSpec::LAYOUT_MAX_SIZE
                          : std::max(spec.width_, 0.f);
  const float height = spec.height_mode_ == tttext::LayoutMode::kIndefinite
                           ? MeasureSpec::LAYOUT_MAX_SIZE
                           : std::max(spec.height_, 0.f);
  return {.width_ = width, .height_ = height, .baseline_ = height};
}

void MarkdownBackgroundImageDrawable::DrawOnRect(tttext::ICanvasHelper* canvas,
                                                 RectF rect,
                                                 tttext::Painter* painter) {
  (void)painter;
  auto image = EnsureImage(rect.GetWidth(), rect.GetHeight());
  if (image == nullptr) {
    return;
  }
  image->Draw(canvas, rect.GetLeft(), rect.GetTop());
}

void MarkdownBackgroundImageDrawable::DrawOnPath(tttext::ICanvasHelper* canvas,
                                                 MarkdownPath* path,
                                                 RectF bounds,
                                                 tttext::Painter* painter) {
  (void)painter;
  if (path == nullptr) {
    return;
  }
  auto image = EnsureImage(bounds.GetWidth(), bounds.GetHeight());
  if (image == nullptr) {
    return;
  }
  auto* canvas_extend =
      context_ == nullptr ? nullptr : context_->GetMarkdownCanvasExtend(canvas);
  if (canvas_extend == nullptr) {
    return;
  }
  canvas->Save();
  canvas_extend->ClipPath(path);
  image->Draw(canvas, bounds.GetLeft(), bounds.GetTop());
  canvas->Restore();
}

MeasureResult MarkdownBackgroundImageDrawable::OnMeasure(MeasureSpec spec) {
  const float width = spec.width_mode_ == tttext::LayoutMode::kIndefinite
                          ? MeasureSpec::LAYOUT_MAX_SIZE
                          : std::max(spec.width_, 0.f);
  const float height = spec.height_mode_ == tttext::LayoutMode::kIndefinite
                           ? MeasureSpec::LAYOUT_MAX_SIZE
                           : std::max(spec.height_, 0.f);
  auto image = EnsureImage(width, height);
  if (image == nullptr) {
    return {.width_ = width, .height_ = height, .baseline_ = height};
  }
  const auto result =
      image->Measure({.width_ = width,
                      .width_mode_ = tttext::LayoutMode::kDefinite,
                      .height_ = height,
                      .height_mode_ = tttext::LayoutMode::kDefinite});
  return {.width_ = result.width_,
          .height_ = result.height_,
          .baseline_ = result.height_};
}

std::shared_ptr<MarkdownDrawable> MarkdownBackgroundImageDrawable::EnsureImage(
    float width, float height) {
  if (image_ != nullptr && measured_width_ == width &&
      measured_height_ == height) {
    return image_;
  }
  if (loader_ == nullptr || url_.empty()) {
    return image_;
  }
  auto image =
      loader_->LoadImage(url_.c_str(), width, height, width, height, 0);
  if (image == nullptr) {
    return image_;
  }
  image_ = std::move(image);
  measured_width_ = width;
  measured_height_ = height;
  return image_;
}

MarkdownLinearGradient MarkdownLinearGradientDrawable::MakeGradientForBounds(
    RectF bounds) const {
  MarkdownLinearGradient gradient = gradient_;
  UpdateGradientPoints(bounds.GetWidth(), bounds.GetHeight(), angle_,
                       direction_, &gradient.start, &gradient.end);
  gradient.start.x_ += bounds.GetLeft();
  gradient.start.y_ += bounds.GetTop();
  gradient.end.x_ += bounds.GetLeft();
  gradient.end.y_ += bounds.GetTop();
  return gradient;
}

bool IsGradientValue(std::string_view value) {
  const auto trimmed = Trim(value);
  return BeginsWith(trimmed, kLinearPrefix);
}

std::shared_ptr<MarkdownBackgroundDrawable> ParseGradientValue(
    std::string_view value, const MarkdownLengthContext& length_context,
    MarkdownContext* context) {
  (void)length_context;
  const auto lower = ToLower(Trim(value));
  const std::string_view trimmed = lower;
  if (trimmed.empty() || trimmed.back() != ')') {
    return nullptr;
  }

  if (!BeginsWith(trimmed, kLinearPrefix)) {
    return nullptr;
  }
  const auto args = trimmed.substr(kLinearPrefix.size(),
                                   trimmed.size() - kLinearPrefix.size() - 1);

  float angle = 180.f;
  auto direction = MarkdownLinearGradientDirection::kToBottom;
  std::vector<uint32_t> colors;
  std::vector<float> stops;
  if (!ParseLinearGradient(args, &angle, &direction, &colors, &stops)) {
    return nullptr;
  }
  return std::make_shared<MarkdownLinearGradientDrawable>(
      context, angle, direction, std::move(colors), std::move(stops));
}

std::shared_ptr<MarkdownBackgroundDrawable> ParseBackgroundDrawableValue(
    std::string_view value, MarkdownResourceLoader* loader,
    const MarkdownLengthContext& length_context, MarkdownContext* context) {
  const auto url = ExtractUrl(value);
  if (!url.empty()) {
    if (loader == nullptr) {
      return nullptr;
    }
    return std::make_shared<MarkdownBackgroundImageDrawable>(context, loader,
                                                             url, nullptr);
  }
  return ParseGradientValue(value, length_context, context);
}

}  // namespace serval::markdown
