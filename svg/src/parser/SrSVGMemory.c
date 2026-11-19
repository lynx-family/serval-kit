// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "parser/SrSVGMemory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef SR_SVG_ALLOC_MALLOC
#define SR_SVG_ALLOC_MALLOC malloc
#define SR_SVG_ALLOC_CALLOC calloc
#define SR_SVG_ALLOC_REALLOC realloc
#define SR_SVG_ALLOC_FREE free
#endif

bool SrSVGAllocationSize(size_t count, size_t size, size_t* bytes) {
  if (size && count > SIZE_MAX / size)
    return false;
  *bytes = count * size;
  return true;
}

void* SrSVGAlloc(size_t size) {
  return size ? SR_SVG_ALLOC_MALLOC(size) : NULL;
}
void* SrSVGCalloc(size_t count, size_t size) {
  size_t bytes;
  return SrSVGAllocationSize(count, size, &bytes) && bytes
             ? SR_SVG_ALLOC_CALLOC(count, size)
             : NULL;
}
void* SrSVGRealloc(void* data, size_t size) {
  if (!size) {
    SR_SVG_ALLOC_FREE(data);
    return NULL;
  }
  return SR_SVG_ALLOC_REALLOC(data, size);
}
void SrSVGFree(void* data) {
  SR_SVG_ALLOC_FREE(data);
}

bool SrSVGBufferReserve(SrSVGBuffer* buffer, size_t bytes) {
  if (bytes <= buffer->capacity)
    return true;
  size_t capacity = buffer->capacity ? buffer->capacity : 32;
  while (capacity < bytes) {
    if (capacity > SIZE_MAX / 2) {
      capacity = bytes;
      break;
    }
    capacity *= 2;
  }
  void* data = SrSVGRealloc(buffer->data, capacity);
  if (!data)
    return false;
  buffer->data = data;
  buffer->capacity = capacity;
  return true;
}

bool SrSVGBufferAppend(SrSVGBuffer* buffer, const void* data, size_t bytes) {
  // Preserve a source slice of this buffer across a moving realloc.
  const uintptr_t source = (uintptr_t)data, base = (uintptr_t)buffer->data;
  bool alias = bytes && buffer->data && source >= base &&
               source - base < buffer->capacity;
  size_t offset = alias ? source - base : 0;
  if ((alias && (offset > buffer->size || bytes > buffer->size - offset)) ||
      bytes > SIZE_MAX - buffer->size ||
      !SrSVGBufferReserve(buffer, buffer->size + bytes))
    return false;
  if (bytes)
    memmove((char*)buffer->data + buffer->size,
            alias ? (char*)buffer->data + offset : data, bytes);
  buffer->size += bytes;
  return true;
}

void SrSVGBufferRelease(SrSVGBuffer* buffer) {
  SrSVGFree(buffer->data);
  *buffer = (SrSVGBuffer){0};
}
void* SrSVGBufferDetach(SrSVGBuffer* buffer) {
  void* data = buffer->data;
  *buffer = (SrSVGBuffer){0};
  return data;
}
