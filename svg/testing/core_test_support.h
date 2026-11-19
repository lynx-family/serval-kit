// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SVG_TESTING_CORE_TEST_SUPPORT_H_
#define SVG_TESTING_CORE_TEST_SUPPORT_H_
#include <stddef.h>
void SVGTestReset(size_t fail_at, bool persistent = false);
size_t SVGTestAllocations();
size_t SVGTestLive();
void SVGTestAllowNew();
#endif  // SVG_TESTING_CORE_TEST_SUPPORT_H_
