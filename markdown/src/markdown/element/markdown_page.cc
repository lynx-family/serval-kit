// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/element/markdown_page.h"
namespace lynx {
namespace markdown {
std::vector<ScrollState> MarkdownPage::GetScrollState() const {
  std::vector<ScrollState> state;
  for (uint32_t i = 0; i < regions_.size(); i++) {
    auto* region = regions_[i].get();
    if (region->scroll_x_) {
      state.emplace_back(ScrollState{i, region->element_->GetType(),
                                     region->scroll_x_offset_});
    }
  }
  return state;
}
void MarkdownPage::ApplyScrollState(const std::vector<ScrollState>& states) {
  for (auto& state : states) {
    if (state.index_ >= regions_.size()) {
      break;
    }
    auto* region = regions_[state.index_].get();
    if (region->scroll_x_ && region->element_->GetType() == state.type_) {
      region->scroll_x_offset_ = state.scroll_offset_;
    }
  }
}
}  // namespace markdown
}  // namespace lynx
