// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DRAWABLE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DRAWABLE_H_
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_textlayout_headers.h"
namespace lynx::markdown {
class MarkdownDrawable : public tttext::RunDelegate {
 public:
  MarkdownDrawable() = default;
  ~MarkdownDrawable() override = default;

  void Layout() override { Measure(MeasureSpec{}); }
  float GetAdvance() const override { return GetWidth(); }
  float GetAscent() const override { return -GetBaseLine(); }
  float GetDescent() const override { return GetHeight() - GetBaseLine(); }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    Draw(canvas, x, y, x + GetWidth(), y + GetHeight());
  }

  virtual SizeF Measure(MeasureSpec spec) = 0;
  virtual void Align() {}
  virtual void Draw(tttext::ICanvasHelper* canvas, float left, float top,
                    float right, float bottom) = 0;
  virtual float GetWidth() const = 0;
  virtual float GetHeight() const = 0;
  virtual float GetBaseLine() const { return GetHeight(); }
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_ELEMENT_MARKDOWN_DRAWABLE_H_
