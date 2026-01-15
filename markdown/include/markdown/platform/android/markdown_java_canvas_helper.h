// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_JAVA_CANVAS_HELPER_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_JAVA_CANVAS_HELPER_H_

#include <textra/i_canvas_helper.h>
#include <textra/platform/java/buffer_output_stream.h>
#include <textra/platform/java/java_canvas_helper.h>

#include <memory>

#include "markdown/utils/markdown_definition.h"
enum class MarkdownCanvasOpExtend : int8_t {
  kClipPath,
  kDrawPath,
  kDrawDelegateOnPath,
};
class MarkdownJavaCanvasHelper : public tttext::JavaCanvasHelper {
  static constexpr int8_t kCanvasOPExtend = -1;

 public:
  MarkdownJavaCanvasHelper();
  ~MarkdownJavaCanvasHelper() override;

  void DrawRunDelegate(const tttext::RunDelegate* delegate, float left,
                       float top, float right, float bottom,
                       tttext::Painter* painter) override;

 public:
  void WritePoint(lynx::markdown::PointF point) {
    stream_->WriteFloat(point.x_);
    stream_->WriteFloat(point.y_);
  }
  void WriteRect(lynx::markdown::RectF rect) {
    stream_->WriteFloat(rect.GetLeft());
    stream_->WriteFloat(rect.GetTop());
    stream_->WriteFloat(rect.GetRight());
    stream_->WriteFloat(rect.GetBottom());
  }

 private:
  BufferOutputStream* stream_;
};

#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_ANDROID_MARKDOWN_JAVA_CANVAS_HELPER_H_
