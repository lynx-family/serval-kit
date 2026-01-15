// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
#include <native_drawing/drawing_types.h>
#include "markdown/draw/markdown_canvas.h"
namespace lynx::markdown {
class HarmonyMarkdownCanvas : public MarkdownCanvas {
 public:
  HarmonyMarkdownCanvas(tttext::ICanvasHelper* tttext_canvas,
                        OH_Drawing_Canvas* canvas)
      : MarkdownCanvas(tttext_canvas), canvas_(canvas) {}
  ~HarmonyMarkdownCanvas() override = default;
  void ClipRoundRect(float left, float top, float right, float bottom,
                     float radiusX, float radiusY, bool doAntiAlias) override;

 private:
  OH_Drawing_Canvas* canvas_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
