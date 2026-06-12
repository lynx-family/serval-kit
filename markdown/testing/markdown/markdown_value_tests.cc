// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <type_traits>

#include "gtest/gtest.h"
#include "markdown/utils/markdown_value.h"

namespace serval::markdown::testing {

TEST(MarkdownValueTest, HasVirtualDestructor) {
  static_assert(std::has_virtual_destructor<Value>::value,
                "Value must destroy derived storage through base pointers.");
  EXPECT_TRUE(std::has_virtual_destructor<Value>::value);
}

}  // namespace serval::markdown::testing
