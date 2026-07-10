// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_INPUT_H_
#define MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_INPUT_H_

namespace serval::markdown::example {

enum class SkityDemoKey {
  kUnknown,
  kSpace,
  kR,
  kN,
  kP,
  kLeft,
  kRight,
  kUp,
  kDown,
  kPageUp,
  kPageDown,
  kHome,
  kEnd,
};

enum class SkityDemoKeyAction { kPress, kRelease };
enum class SkityDemoPointerAction { kDown, kUp, kMove };
enum class SkityDemoPointerButton { kPrimary, kOther };

}  // namespace serval::markdown::example

#endif  // MARKDOWN_EXAMPLE_SKITY_DEMO_SKITY_DEMO_INPUT_H_
