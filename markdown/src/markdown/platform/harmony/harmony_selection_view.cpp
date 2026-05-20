// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <utility>

#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/view/markdown_selection_view.h"

namespace serval::markdown {
std::shared_ptr<MarkdownPlatformView>
NativeServalMarkdownView::CreateSelectionHighlightSubView(
    const uint32_t color) {
  const auto view = CreateCustomSubView();
  auto highlight = std::make_shared<MarkdownSelectionHighlight>();
  highlight->SetColor(color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(highlight));
  return view;
}
std::shared_ptr<MarkdownPlatformView>
NativeServalMarkdownView::CreateSelectionHandleSubView(SelectionHandleType type,
                                                       float size,
                                                       uint32_t color) {
  const auto view = CreateCustomSubView();
  auto selection_handle =
      std::make_shared<MarkdownSelectionHandle>(size, type, color);
  view->GetCustomViewHandle()->AttachDrawable(std::move(selection_handle));
  auto harmony_view = static_cast<HarmonyView*>(view.get());
  harmony_view->SetClipByParent(false);
  return view;
}
}  // namespace serval::markdown
