// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/platform/harmony/internal/harmony_markdown_canvas.h"

#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>
namespace lynx::markdown {
void HarmonyMarkdownCanvas::ClipPath(MarkdownPath* path) {}
void HarmonyMarkdownCanvas::DrawMarkdownPath(MarkdownPath* path,
                                             tttext::Painter* painter) {}
void HarmonyMarkdownCanvas::DrawDelegateOnPath(
    tttext::RunDelegate* run_delegate, MarkdownPath* path,
    tttext::Painter* painter) {}
}  // namespace lynx::markdown
