// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_LIST_ITEM_H_
#define MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_LIST_ITEM_H_

#include <memory>
#include <utility>

#include "markdown/element/markdown_drawable.h"
#include "markdown/layout/markdown_layout_node.h"
#include "markdown/style/markdown_style.h"

namespace serval::markdown {

class MarkdownLayoutListItem : public MarkdownLayoutNode {
 public:
  explicit MarkdownLayoutListItem(MarkdownContext* context);
  ~MarkdownLayoutListItem() override;

  void SetMarker(std::shared_ptr<MarkdownDrawable> marker) {
    marker_ = std::move(marker);
  }
  const std::shared_ptr<MarkdownDrawable>& GetMarker() const { return marker_; }
  void SetMarkerAlign(MarkdownVerticalAlign align) { marker_align_ = align; }
  MarkdownVerticalAlign GetMarkerAlign() const { return marker_align_; }

 protected:
  LayoutResult OnLayout(LayoutParams params) override;
  void OnRender(RenderParams params) override;

 private:
  std::shared_ptr<MarkdownDrawable> marker_;
  MeasureResult marker_measure_result_{};
  PointF marker_offset_{};
  MarkdownVerticalAlign marker_align_{MarkdownVerticalAlign::kBaseline};
};

}  // namespace serval::markdown

#endif  // MARKDOWN_INCLUDE_MARKDOWN_LAYOUT_MARKDOWN_LAYOUT_LIST_ITEM_H_
