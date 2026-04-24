// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_
#define MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/element/markdown_drawable.h"
#include "markdown/style/markdown_style_value.h"

namespace serval::markdown {

class MarkdownResourceLoader;
class MarkdownContext;

enum class MarkdownGradientType : int32_t {
  kLinear = 2,
};

enum class MarkdownLinearGradientDirection : int32_t {
  kNone = 0,
  kToTop = 1,
  kToBottom = 2,
  kToLeft = 3,
  kToRight = 4,
  kToTopRight = 5,
  kToTopLeft = 6,
  kToBottomRight = 7,
  kToBottomLeft = 8,
  kAngle = 9,
};

class MarkdownLinearGradientDrawable final : public MarkdownBackgroundDrawable {
 public:
  MarkdownLinearGradientDrawable(MarkdownContext* context, float angle,
                                 MarkdownLinearGradientDirection direction,
                                 std::vector<uint32_t> colors,
                                 std::vector<float> stops);
  ~MarkdownLinearGradientDrawable() override = default;

  void DrawOnRect(tttext::ICanvasHelper* canvas, RectF rect,
                  tttext::Painter* painter = nullptr) override;
  void DrawOnPath(tttext::ICanvasHelper* canvas, MarkdownPath* path,
                  RectF bounds, tttext::Painter* painter) override;

  const MarkdownLinearGradient& GetGradient() const { return gradient_; }
  float GetAngle() const { return angle_; }
  MarkdownLinearGradientDirection GetDirection() const { return direction_; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 private:
  MarkdownLinearGradient MakeTranslatedGradient(float left, float top) const;
  void UpdatePoints(float width, float height);

  float angle_{0};
  MarkdownLinearGradientDirection direction_{
      MarkdownLinearGradientDirection::kNone};
  MarkdownLinearGradient gradient_{};
  MarkdownContext* context_{nullptr};
};

class MarkdownBackgroundImageDrawable final
    : public MarkdownBackgroundDrawable {
 public:
  MarkdownBackgroundImageDrawable(MarkdownContext* context,
                                  MarkdownResourceLoader* loader,
                                  std::string url,
                                  std::shared_ptr<MarkdownDrawable> image);
  ~MarkdownBackgroundImageDrawable() override = default;

  void DrawOnRect(tttext::ICanvasHelper* canvas, RectF rect,
                  tttext::Painter* painter = nullptr) override;
  void DrawOnPath(tttext::ICanvasHelper* canvas, MarkdownPath* path,
                  RectF bounds, tttext::Painter* painter) override;

  const std::string& GetUrl() const { return url_; }

 protected:
  MeasureResult OnMeasure(MeasureSpec spec) override;

 private:
  std::shared_ptr<MarkdownDrawable> EnsureImage(float width, float height);

  MarkdownResourceLoader* loader_{nullptr};
  std::string url_;
  std::shared_ptr<MarkdownDrawable> image_;
  MarkdownContext* context_{nullptr};
  float measured_width_{-1};
  float measured_height_{-1};
};

bool IsGradientValue(std::string_view value);

std::shared_ptr<MarkdownBackgroundDrawable> ParseGradientValue(
    std::string_view value, const MarkdownLengthContext& length_context,
    MarkdownContext* context = nullptr);

std::shared_ptr<MarkdownBackgroundDrawable> ParseBackgroundDrawableValue(
    std::string_view value, MarkdownResourceLoader* loader,
    const MarkdownLengthContext& length_context,
    MarkdownContext* context = nullptr);

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_STYLE_MARKDOWN_GRADIENT_H_
