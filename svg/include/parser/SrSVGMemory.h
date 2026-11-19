// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SVG_INCLUDE_PARSER_SRSVGMEMORY_H_
#define SVG_INCLUDE_PARSER_SRSVGMEMORY_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// A single fallible allocation boundary, also used by no-exception builds.
void* SrSVGAlloc(size_t size);
void* SrSVGCalloc(size_t count, size_t size);
void* SrSVGRealloc(void* data, size_t size);
void SrSVGFree(void* data);
bool SrSVGAllocationSize(size_t count, size_t size, size_t* bytes);

typedef struct SrSVGBuffer {
  void* data;
  size_t size;      // bytes in use
  size_t capacity;  // allocated bytes
} SrSVGBuffer;

bool SrSVGBufferReserve(SrSVGBuffer* buffer, size_t bytes);
bool SrSVGBufferAppend(SrSVGBuffer* buffer, const void* data, size_t bytes);
void SrSVGBufferRelease(SrSVGBuffer* buffer);
void* SrSVGBufferDetach(SrSVGBuffer* buffer);

#ifdef __cplusplus
}
#endif
#endif  // SVG_INCLUDE_PARSER_SRSVGMEMORY_H_
