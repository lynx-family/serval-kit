// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_NODE_
#define MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_NODE_
#include <cstdint>
#include "markdown/utils/markdown_marco.h"
namespace lynx::markdown {
class L_EXPORT MarkdownNode {
 public:
  MarkdownNode() = default;
  virtual ~MarkdownNode() = default;
  void PrependChild(MarkdownNode* child);
  void AppendChild(MarkdownNode* child);
  void InsertBefore(MarkdownNode* child, MarkdownNode* sibling);
  void InsertAfter(MarkdownNode* child, MarkdownNode* sibling);
  void RemoveChild(MarkdownNode* child);
  MarkdownNode* GetParent() const { return parent_; }
  MarkdownNode* GetPrevious() const { return previous_; }
  MarkdownNode* GetNext() const { return next_; }
  MarkdownNode* GetFirstChild() const { return first_child_; }
  MarkdownNode* GetLastChild() const { return last_child_; }
  int32_t GetChildCount() const { return child_count_; }

 protected:
  MarkdownNode* parent_{nullptr};
  MarkdownNode* previous_{nullptr};
  MarkdownNode* next_{nullptr};
  MarkdownNode* first_child_{nullptr};
  MarkdownNode* last_child_{nullptr};
  int32_t child_count_{0};
};
}  // namespace lynx::markdown
#endif  //MARKDOWN_INCLUDE_MARKDOWN_UTILS_MARKDOWN_NODE_
