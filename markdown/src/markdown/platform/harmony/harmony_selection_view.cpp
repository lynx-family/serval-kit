// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <utility>

#include "markdown/platform/harmony/internal/harmony_view.h"
#include "markdown/view/markdown_selection_view.h"

namespace lynx::markdown {
MarkdownPlatformView* MarkdownSelectionHighlight::CreateView(
    MarkdownViewContainerHandle* parent, const uint32_t color) {
  const auto view = parent->CreateCustomSubView();
  auto highlight = std::make_unique<MarkdownSelectionHighlight>();
  highlight->SetColor(color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(highlight));
  return view;
}
MarkdownPlatformView* MarkdownSelectionHandle::CreateView(
    MarkdownViewContainerHandle* parent, SelectionHandleType type, float size,
    float margin, uint32_t color) {
  const auto view = parent->CreateCustomSubView();
  auto selection_handle =
      std::make_unique<MarkdownSelectionHandle>(size, margin, type, color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(selection_handle));
  auto harmony_view = static_cast<HarmonyView*>(view);
  harmony_view->EnablePanEvent(true, GESTURE_DIRECTION_ALL, PRIORITY);
  harmony_view->SetClipByParent(false);
  return view;
}
}  // namespace lynx::markdown
