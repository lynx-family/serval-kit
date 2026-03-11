// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/view/markdown_view_renderer.h"

#include "markdown/draw/markdown_drawer.h"
#include "markdown/draw/markdown_typewriter_drawer.h"
#include "markdown/element/markdown_document.h"
#include "markdown/view/markdown_platform_view.h"
#include "markdown/view/markdown_view_measurer.h"

namespace lynx::markdown {

void MarkdownViewRenderer::SetDocument(
    std::shared_ptr<MarkdownDocument> document) {
  document_ = std::move(document);
}
void MarkdownViewRenderer::Draw(tttext::ICanvasHelper* canvas, float left,
                                float top) const {
  if (document_ == nullptr) {
    return;
  }
  if (animation_type_ == MarkdownAnimationType::kNone) {
    MarkdownDrawer drawer(canvas);
    auto page = document_->GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
  }
  if (animation_type_ == MarkdownAnimationType::kTypewriter) {
    MarkdownCharTypewriterDrawer drawer(
        canvas, animation_step_, document_->GetResourceLoader(),
        document_->GetStyle().typewriter_cursor_, false, cursor_.get());
    auto page = document_->GetPage();
    if (page != nullptr) {
      drawer.DrawPage(*page);
    }
  }
}

}  // namespace lynx::markdown
