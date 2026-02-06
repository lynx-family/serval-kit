// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_node.h"
namespace lynx::markdown {
void MarkdownNode::AppendChild(MarkdownNode* child) {
  child->parent_ = this;
  child->next_ = nullptr;
  if (!last_child_) {
    child->previous_ = nullptr;
    first_child_ = child;
    last_child_ = child;
    child_count_++;
    return;
  }
  last_child_->next_ = child;
  child->previous_ = last_child_;
  last_child_ = child;
  child_count_++;
}
void MarkdownNode::PrependChild(MarkdownNode* child) {
  child->parent_ = this;
  child->previous_ = nullptr;
  if (!first_child_) {
    child->next_ = nullptr;
    first_child_ = child;
    last_child_ = child;
    child_count_++;
    return;
  }
  first_child_->previous_ = child;
  child->next_ = first_child_;
  first_child_ = child;
  child_count_++;
}
void MarkdownNode::InsertBefore(MarkdownNode* child, MarkdownNode* sibling) {
  if (!sibling || sibling == first_child_) {
    PrependChild(child);
    return;
  }
  child->parent_ = this;
  child->next_ = sibling;
  child->previous_ = sibling->previous_;
  sibling->previous_->next_ = child;
  sibling->previous_ = child;
  child_count_++;
}
void MarkdownNode::InsertAfter(MarkdownNode* child, MarkdownNode* sibling) {
  if (!sibling || sibling == last_child_) {
    AppendChild(child);
    return;
  }
  child->parent_ = this;
  child->previous_ = sibling;
  child->next_ = sibling->next_;
  sibling->next_->previous_ = child;
  sibling->next_ = child;
  child_count_++;
}
void MarkdownNode::RemoveChild(MarkdownNode* child) {
  child->parent_ = nullptr;
  if (child == first_child_) {
    first_child_ = child->next_;
  } else {
    child->previous_->next_ = child->next_;
  }
  if (child == last_child_) {
    last_child_ = child->previous_;
  } else {
    child->next_->previous_ = child->previous_;
  }
  child->previous_ = nullptr;
  child->next_ = nullptr;
  child_count_--;
}

}  // namespace lynx::markdown
