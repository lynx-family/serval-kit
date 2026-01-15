// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
#define MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
#include <string>
#include "markdown/style/markdown_style.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown::testing {
class MockImage : public tttext::RunDelegate {
 public:
  MockImage(const char* src, float desire_width, float desire_height,
            float max_width, float max_height)
      : src_(src) {
    float img_w = max_width * 0.1;
    float img_h = 20;
    if (desire_width > 0 && desire_height > 0) {
      width_ = desire_width;
      height_ = desire_height;
    } else if (desire_width > 0) {
      width_ = desire_width;
      height_ = desire_width * img_h / img_w;
    } else if (desire_height > 0) {
      height_ = desire_height;
      width_ = desire_height * img_w / img_h;
    } else {
      width_ = img_w;
      height_ = img_h;
    }
  }
  ~MockImage() override = default;

  float GetAscent() const override { return -height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  std::string src_;
  float width_;
  float height_;
};
class MockInlineView : public tttext::RunDelegate {
 public:
  MockInlineView(const char* id_selector, float max_width, float max_height)
      : id_(id_selector), width_(max_width * 0.2), height_(30) {}
  ~MockInlineView() override = default;

  float GetAscent() const override { return -height_; }
  float GetDescent() const override { return 0; }
  float GetAdvance() const override { return width_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override;

  std::string id_;
  float width_;
  float height_;
};
class MockBackgroundDrawable : public MarkdownDrawable {
 public:
  MockBackgroundDrawable(MarkdownBackgroundStylePart* background,
                         float border_radius, float font_size,
                         float root_font_size)
      : background_(*background),
        border_radius_(border_radius),
        font_size_(font_size),
        root_font_size_(root_font_size) {}
  ~MockBackgroundDrawable() override = default;
  void Draw(tttext::ICanvasHelper* canvas, float left, float top, float right,
            float bottom) override;

  MarkdownBackgroundStylePart background_;
  float border_radius_;
  float font_size_;
  float root_font_size_;
};
}  // namespace lynx::markdown::testing
#endif  // MARKDOWN_TESTING_MARKDOWN_MOCK_RUN_DELEGATE_H_
