// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "markdown/utils/markdown_node.h"

#include <vector>

#include "gtest/gtest.h"

namespace lynx::markdown {
namespace {

void ExpectChain(MarkdownNode* first, const std::vector<MarkdownNode*>& nodes) {
  if (nodes.empty()) {
    EXPECT_EQ(first, nullptr);
    return;
  }
  MarkdownNode* current = first;
  for (size_t i = 0; i < nodes.size(); i++) {
    EXPECT_EQ(current, nodes[i]);
    if (i == 0) {
      EXPECT_EQ(nodes[i]->GetPrevious(), nullptr);
    } else {
      EXPECT_EQ(nodes[i]->GetPrevious(), nodes[i - 1]);
    }
    if (i + 1 < nodes.size()) {
      EXPECT_EQ(nodes[i]->GetNext(), nodes[i + 1]);
    } else {
      EXPECT_EQ(nodes[i]->GetNext(), nullptr);
    }
    current = current->GetNext();
  }
  EXPECT_EQ(current, nullptr);
}

}  // namespace

TEST(MarkdownNodeTest, AppendChildHandlesEmptyAndNonEmpty) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;

  parent.AppendChild(&child1);
  EXPECT_EQ(parent.GetFirstChild(), &child1);
  EXPECT_EQ(parent.GetLastChild(), &child1);
  EXPECT_EQ(parent.GetChildCount(), 1);
  EXPECT_EQ(child1.GetParent(), &parent);
  EXPECT_EQ(child1.GetPrevious(), nullptr);
  EXPECT_EQ(child1.GetNext(), nullptr);

  parent.AppendChild(&child2);
  EXPECT_EQ(parent.GetFirstChild(), &child1);
  EXPECT_EQ(parent.GetLastChild(), &child2);
  EXPECT_EQ(parent.GetChildCount(), 2);
  EXPECT_EQ(child1.GetNext(), &child2);
  EXPECT_EQ(child2.GetPrevious(), &child1);
  EXPECT_EQ(child2.GetNext(), nullptr);
}

TEST(MarkdownNodeTest, PrependChildHandlesEmptyAndNonEmpty) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;

  parent.PrependChild(&child1);
  EXPECT_EQ(parent.GetFirstChild(), &child1);
  EXPECT_EQ(parent.GetLastChild(), &child1);
  EXPECT_EQ(parent.GetChildCount(), 1);
  EXPECT_EQ(child1.GetParent(), &parent);
  EXPECT_EQ(child1.GetPrevious(), nullptr);
  EXPECT_EQ(child1.GetNext(), nullptr);

  parent.PrependChild(&child2);
  EXPECT_EQ(parent.GetFirstChild(), &child2);
  EXPECT_EQ(parent.GetLastChild(), &child1);
  EXPECT_EQ(parent.GetChildCount(), 2);
  EXPECT_EQ(child2.GetNext(), &child1);
  EXPECT_EQ(child1.GetPrevious(), &child2);
  EXPECT_EQ(child2.GetPrevious(), nullptr);
}

TEST(MarkdownNodeTest, InsertBeforeHandlesFirstAndMiddle) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;
  MarkdownNode child3;
  MarkdownNode child0;

  parent.AppendChild(&child1);
  parent.AppendChild(&child3);
  parent.InsertBefore(&child2, &child3);

  ExpectChain(parent.GetFirstChild(), {&child1, &child2, &child3});
  EXPECT_EQ(parent.GetLastChild(), &child3);
  EXPECT_EQ(parent.GetChildCount(), 3);

  parent.InsertBefore(&child0, &child1);
  ExpectChain(parent.GetFirstChild(), {&child0, &child1, &child2, &child3});
  EXPECT_EQ(parent.GetLastChild(), &child3);
  EXPECT_EQ(parent.GetChildCount(), 4);
}

TEST(MarkdownNodeTest, InsertAfterHandlesLastMiddleAndNull) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;
  MarkdownNode child3;
  MarkdownNode child4;
  MarkdownNode child5;

  parent.AppendChild(&child1);
  parent.AppendChild(&child3);
  parent.InsertAfter(&child2, &child1);

  ExpectChain(parent.GetFirstChild(), {&child1, &child2, &child3});
  EXPECT_EQ(parent.GetLastChild(), &child3);
  EXPECT_EQ(parent.GetChildCount(), 3);

  parent.InsertAfter(&child4, &child3);
  ExpectChain(parent.GetFirstChild(), {&child1, &child2, &child3, &child4});
  EXPECT_EQ(parent.GetLastChild(), &child4);
  EXPECT_EQ(parent.GetChildCount(), 4);

  parent.InsertAfter(&child5, nullptr);
  ExpectChain(parent.GetFirstChild(),
              {&child1, &child2, &child3, &child4, &child5});
  EXPECT_EQ(parent.GetLastChild(), &child5);
  EXPECT_EQ(parent.GetChildCount(), 5);
}

TEST(MarkdownNodeTest, RemoveChildUpdatesLinksAndCount) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;
  MarkdownNode child3;

  parent.AppendChild(&child1);
  parent.AppendChild(&child2);
  parent.AppendChild(&child3);

  parent.RemoveChild(&child2);
  ExpectChain(parent.GetFirstChild(), {&child1, &child3});
  EXPECT_EQ(parent.GetLastChild(), &child3);
  EXPECT_EQ(parent.GetChildCount(), 2);
  EXPECT_EQ(child2.GetParent(), nullptr);
  EXPECT_EQ(child2.GetPrevious(), nullptr);
  EXPECT_EQ(child2.GetNext(), nullptr);

  parent.RemoveChild(&child1);
  ExpectChain(parent.GetFirstChild(), {&child3});
  EXPECT_EQ(parent.GetLastChild(), &child3);
  EXPECT_EQ(parent.GetChildCount(), 1);
  EXPECT_EQ(child1.GetParent(), nullptr);

  parent.RemoveChild(&child3);
  EXPECT_EQ(parent.GetFirstChild(), nullptr);
  EXPECT_EQ(parent.GetLastChild(), nullptr);
  EXPECT_EQ(parent.GetChildCount(), 0);
  EXPECT_EQ(child3.GetParent(), nullptr);
  EXPECT_EQ(child3.GetPrevious(), nullptr);
  EXPECT_EQ(child3.GetNext(), nullptr);
}

TEST(MarkdownNodeTest, InsertBeforeWithNullPrepends) {
  MarkdownNode parent;
  MarkdownNode child1;
  MarkdownNode child2;

  parent.AppendChild(&child1);
  parent.InsertBefore(&child2, nullptr);

  ExpectChain(parent.GetFirstChild(), {&child2, &child1});
  EXPECT_EQ(parent.GetLastChild(), &child1);
  EXPECT_EQ(parent.GetChildCount(), 2);
}

}  // namespace lynx::markdown
