// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_RUN_DELEGATES_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_RUN_DELEGATES_H_
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "markdown/draw/markdown_canvas.h"
#include "markdown/layout/markdown_selection.h"
#include "markdown/style/markdown_style.h"
#include "markdown/style/markdown_style_value.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx {
namespace markdown {
class MarkdownUnorderedListMarkDelegate : public tttext::RunDelegate {
 public:
  MarkdownUnorderedListMarkDelegate(
      MarkdownMarkType type, const MarkdownUnorderedListMarkerStyle& style)
      : type_(type), style_(style) {
    height_ = style.size_.width_ + style.block_.margin_top_ +
              style.block_.margin_bottom_;
    width_ = style.size_.width_ + style.block_.margin_left_ +
             style.block_.margin_right_;
  }
  float GetAscent() const override { return -height_; }

  float GetDescent() const override { return 0; }

  float GetAdvance() const override { return width_; }

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    canvas->Save();
    canvas->Translate(x, y);
    canvas->Translate(style_.block_.margin_left_, style_.block_.margin_top_);
    auto painter = canvas->CreatePainter();
    painter->SetStrokeWidth(MarkdownScreenMetrics::DPToPx(1));
    switch (type_) {
      case MarkdownMarkType::kCircle:
        painter->SetFillColor(style_.marker_.color_);
        canvas->DrawCircle(style_.size_.width_ / 2, style_.size_.width_ / 2,
                           style_.size_.width_ / 2, painter.get());
        break;
      case MarkdownMarkType::kRing:
        painter->SetStrokeColor(style_.marker_.color_);
        canvas->DrawCircle(style_.size_.width_ / 2, style_.size_.width_ / 2,
                           style_.size_.width_ / 2 - 0.5, painter.get());
        break;
      case MarkdownMarkType::kSquare:
        painter->SetFillColor(style_.marker_.color_);
        canvas->DrawRect(0, 0, style_.size_.width_, style_.size_.width_,
                         painter.get());
        break;
      default:
        break;
    }
    canvas->Restore();
  }

 private:
  float width_;
  float height_;
  MarkdownMarkType type_;
  MarkdownUnorderedListMarkerStyle style_;
};

class MarkdownEmptySpaceDelegate : public tttext::RunDelegate {
 public:
  explicit MarkdownEmptySpaceDelegate(float space) : width_(space) {}
  float GetAscent() const override { return 0; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {}

 private:
  float width_;
};

class MarkdownRefDelegate : public tttext::RunDelegate {
 public:
  MarkdownRefDelegate(std::unique_ptr<tttext::Paragraph> paragraph,
                      MarkdownRefStyle style, float base_text_size)
      : paragraph_(std::move(paragraph)),
        style_(std::move(style)),
        base_text_height_(base_text_size * 0.9f) {}
  ~MarkdownRefDelegate() override = default;
  float GetAscent() const override { return -base_text_height_; }
  float GetDescent() const override { return height_ - base_text_height_; }
  float GetAdvance() const override {
    return width_ + style_.block_.margin_left_ + style_.block_.margin_right_;
  }
  bool Equals(RunDelegate* other) override { return this == other; }
  void Layout() override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 private:
  std::unique_ptr<tttext::Paragraph> paragraph_;
  std::unique_ptr<tttext::LayoutRegion> page_;
  bool layout_{false};
  float width_{};
  float height_{};
  float base_line_{};
  MarkdownRefStyle style_;
  float base_text_height_;
};

class MarkdownTextDelegate : public tttext::RunDelegate {
 public:
  MarkdownTextDelegate(std::unique_ptr<tttext::Paragraph> text, float width,
                       float height)
      : text_(std::move(text)), width_(width), height_(height) {
    MarkdownStyleInitializer::ResetBlockStyle(&block_style_);
  }
  MarkdownTextDelegate(std::unique_ptr<tttext::Paragraph> text,
                       MarkdownBlockStylePart block, float width, float height)
      : text_(std::move(text)),
        width_(width),
        height_(height),
        block_style_(block) {}

  float GetAscent() const override { return ascent_; }
  float GetDescent() const override { return descent_; }
  float GetAdvance() const override { return advance_; }

  void Layout() override;

  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 private:
  bool layout_{false};
  float ascent_{0};
  float descent_{0};
  float advance_{0};
  std::unique_ptr<tttext::Paragraph> text_;
  std::unique_ptr<tttext::LayoutRegion> page_;
  float width_;
  float height_;
  MarkdownBlockStylePart block_style_{};
};

enum class InlineBorderDirection {
  kLeft,
  kRight,
};
class MarkdownInlineBorderDelegate : public tttext::RunDelegate {
 public:
  MarkdownInlineBorderDelegate(InlineBorderDirection direction,
                               MarkdownBorderStylePart border_style,
                               MarkdownBlockStylePart block_style,
                               uint32_t background_color, uint32_t char_offset);
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  float GetAdvance() const override {
    return border_style_.border_width_ +
           (direction_ == InlineBorderDirection::kLeft
                ? (block_style_.margin_left_ + block_style_.padding_left_)
                : (block_style_.margin_right_ + block_style_.padding_right_));
  }
  float GetAscent() const override { return 0; }
  float GetDescent() const override { return 0; }
  void SetEnable(bool enable) { enable_ = enable; }
  void UpdateDrawRect(std::vector<RectF>&& rect);
  const std::vector<RectF>& GetDrawRect() const { return draw_rect_; }
  uint32_t GetCharOffset() const { return char_offset_; }
  void SetBackgroundDrawable(std::shared_ptr<MarkdownDrawable> drawable) {
    background_drawable_ = std::move(drawable);
  }
  void SetCharOffset(uint32_t char_offset) { char_offset_ = char_offset; }
  void SetRectType(MarkdownSelection::RectType rect_type) {
    border_rect_type_ = rect_type;
  }
  MarkdownSelection::RectType GetRectType() const { return border_rect_type_; }
  void DrawOnRects(tttext::ICanvasHelper* canvas, float x, float y,
                   const std::vector<RectF>& rects);

 private:
  bool enable_{true};
  InlineBorderDirection direction_;
  MarkdownBorderStylePart border_style_;
  MarkdownBlockStylePart block_style_;
  uint32_t background_color_;
  std::vector<RectF> draw_rect_;
  uint32_t char_offset_{0};
  std::shared_ptr<MarkdownDrawable> background_drawable_;
  MarkdownSelection::RectType border_rect_type_;
};

class BlockViewWrapper : public tttext::RunDelegate {
 public:
  BlockViewWrapper(const float max_width, const float indent,
                   std::shared_ptr<RunDelegate> delegate)
      : max_width_(max_width),
        indent_(indent),
        delegate_(std::move(delegate)) {}

  float GetAdvance() const override { return max_width_; }
  float GetAscent() const override { return delegate_->GetAscent(); }
  float GetDescent() const override { return delegate_->GetDescent(); }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    canvas->Save();
    float offset = (max_width_ - delegate_->GetAdvance()) / 2;
    canvas->Translate(x - indent_, 0);
    delegate_->Draw(canvas, offset, y);
    canvas->Restore();
  }
  void Layout() override { delegate_->Layout(); }

 protected:
  float max_width_;
  float indent_;
  std::shared_ptr<RunDelegate> delegate_;
};

class RoundRectImageWrapper : public tttext::RunDelegate {
 public:
  RoundRectImageWrapper(std::shared_ptr<RunDelegate> delegate, float radius)
      : delegate_(std::move(delegate)), radius_(radius) {}
  ~RoundRectImageWrapper() override = default;
  float GetAscent() const override { return delegate_->GetAscent(); }
  float GetDescent() const override { return delegate_->GetDescent(); }
  float GetAdvance() const override { return delegate_->GetAdvance(); }
  void Layout() override { delegate_->Layout(); }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 private:
  float radius_;
  std::shared_ptr<RunDelegate> delegate_;
};

class CircleDelegate : public tttext::RunDelegate {
 public:
  CircleDelegate(float radius, uint32_t color)
      : radius_(radius), color_(color) {}
  ~CircleDelegate() override = default;
  float GetAdvance() const override { return radius_ * 2; }
  float GetDescent() const override { return 0; }
  float GetAscent() const override { return -radius_ * 2; }
  void Layout() override {}
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

 private:
  float radius_{0};
  uint32_t color_{0};
};
class ImageWithCaption final : public tttext::RunDelegate {
 public:
  ImageWithCaption(
      std::shared_ptr<RunDelegate> image,
      std::unique_ptr<tttext::Paragraph> caption, const float max_width,
      const MarkdownCaptionPosition position = MarkdownCaptionPosition::kBottom,
      const MarkdownTextAlign align = MarkdownTextAlign::kCenter)
      : image_(std::move(image)),
        caption_(std::move(caption)),
        max_width_(max_width),
        caption_position_(position),
        align_(align) {}
  void Layout() override;
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;
  float GetAscent() const override { return -height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }

 private:
  std::shared_ptr<RunDelegate> image_;
  std::unique_ptr<tttext::Paragraph> caption_;
  std::unique_ptr<tttext::LayoutRegion> region_;
  float max_width_;
  MarkdownCaptionPosition caption_position_;
  MarkdownTextAlign align_;
  float width_{};
  float height_{};
  bool layout_{false};
};
}  // namespace markdown
}  // namespace lynx
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_RUN_DELEGATES_H_
