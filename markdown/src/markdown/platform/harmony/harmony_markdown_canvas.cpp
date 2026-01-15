// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"

#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>
namespace lynx::markdown {
void HarmonyMarkdownCanvas::ClipRoundRect(float left, float top, float right,
                                          float bottom, float radiusX,
                                          float radiusY, bool doAntiAlias) {
  auto rect = OH_Drawing_RectCreate(left, top, right, bottom);
  auto round_rect = OH_Drawing_RoundRectCreate(rect, radiusX, radiusY);
  OH_Drawing_CanvasClipRoundRect(canvas_, round_rect, INTERSECT, doAntiAlias);
  OH_Drawing_RectDestroy(rect);
  OH_Drawing_RoundRectDestroy(round_rect);
}
}  // namespace lynx::markdown
