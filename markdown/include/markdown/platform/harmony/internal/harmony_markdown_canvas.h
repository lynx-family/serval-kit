// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
#define MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
#include <native_drawing/drawing_types.h>

#include "markdown/draw/markdown_canvas.h"
#include "textra/platform/ark_graphics/ag_canvas_helper.h"
namespace lynx::markdown {
class HarmonyMarkdownCanvas : public ttoffice::tttext::AGCanvasHelper,
                              public MarkdownCanvasExtend {
 public:
  explicit HarmonyMarkdownCanvas(OH_Drawing_Canvas* canvas)
      : ttoffice::tttext::AGCanvasHelper(canvas), canvas_(canvas) {}
  ~HarmonyMarkdownCanvas() override = default;
  void ClipPath(MarkdownPath* path) override;
  void DrawMarkdownPath(MarkdownPath* path, tttext::Painter* painter) override;
  void DrawDelegateOnPath(tttext::RunDelegate* run_delegate, MarkdownPath* path,
                          tttext::Painter* painter) override;

 private:
  OH_Drawing_Canvas* canvas_;
};
}  // namespace lynx::markdown
#endif  // MARKDOWN_INCLUDE_MARKDOWN_PLATFORM_HARMONY_INTERNAL_HARMONY_MARKDOWN_CANVAS_H_
