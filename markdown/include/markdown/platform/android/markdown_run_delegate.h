// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_

#include <textra/i_canvas_helper.h>

#include "markdown/element/markdown_drawable.h"
#include "markdown/utils/markdown_definition.h"
#include "markdown/utils/markdown_platform.h"

namespace serval::markdown {

class MarkdownRunDelegate final : public MarkdownDrawable {
 public:
  MarkdownRunDelegate(int id, float actual_width, float actual_height,
                      float desire_width, float desire_height, float max_width,
                      float max_height, float radius)
      : id_(id), radius_(radius) {
    if (actual_width > 0 && actual_height > 0) {
      if (desire_width > 0 && desire_height > 0) {
        width_ = desire_width;
        height_ = desire_height;
      } else if (desire_width > 0) {
        width_ = desire_width;
        height_ = desire_width * actual_height / actual_width;
      } else if (desire_height > 0) {
        height_ = desire_height;
        width_ = desire_height * actual_width / actual_height;
      } else {
        width_ = actual_width;
        height_ = actual_height;
      }
    } else {
      width_ = desire_width > 0 ? desire_width : 0;
      height_ = desire_height > 0 ? desire_height : 0;
    }
    if (max_width > 0 && width_ > max_width) {
      if (width_ > 0) {
        height_ *= max_width / width_;
      }
      width_ = max_width;
    }
    if (max_height > 0 && height_ > max_height) {
      if (height_ > 0) {
        width_ *= max_height / height_;
      }
      height_ = max_height;
    }
  }
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
  MeasureResult OnMeasure(MeasureSpec spec) override {
    return {.width_ = width_, .height_ = height_, .baseline_ = height_};
  }
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_RUN_DELEGATE_H_
