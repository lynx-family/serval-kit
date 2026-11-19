// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_TESTING_ALLOCATION_HOOKS_H_
#define SVG_TESTING_ALLOCATION_HOOKS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
void* svg_test_malloc(size_t size);
void* svg_test_calloc(size_t count, size_t size);
void* svg_test_realloc(void* ptr, size_t size);
void svg_test_free(void* ptr);
#ifdef __cplusplus
}
#endif

// This header is force-included only when compiling the parser allocator tests.
#define SR_SVG_ALLOC_MALLOC svg_test_malloc
#define SR_SVG_ALLOC_CALLOC svg_test_calloc
#define SR_SVG_ALLOC_REALLOC svg_test_realloc
#define SR_SVG_ALLOC_FREE svg_test_free

#endif  // SVG_TESTING_ALLOCATION_HOOKS_H_
