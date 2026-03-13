// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_

#include <textra/i_canvas_helper.h>

#include "markdown/element/markdown_drawable.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"

class MarkdownRunDelegate final : public lynx::markdown::MarkdownDrawable {
 public:
  MarkdownRunDelegate(int id, float width, float height, float radius)
      : id_(id), width_(width), height_(height), radius_(radius) {}
  ~MarkdownRunDelegate() override = default;

 public:
  int GetID() const { return id_; }
  float GetRadius() const { return radius_; }
  void Draw(tttext::ICanvasHelper* canvas, float x, float y) override {
    tttext::Painter painter;
    canvas->DrawRunDelegate(this, x, y, x + width_, y + height_, &painter);
  }

 private:
  int id_{0};
  float width_{0};
  float height_{0};
  float radius_{0};

 protected:
  lynx::markdown::MeasureResult OnMeasure(
      lynx::markdown::MeasureSpec spec) override {
    return {.width_ = width_, .height_ = height_, .baseline_ = height_};
  }
};
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
