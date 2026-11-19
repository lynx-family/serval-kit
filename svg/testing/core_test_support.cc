// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core_test_support.h"
#include <cassert>
#include <cstdlib>
#include <new>
namespace {
size_t count, failure, live;
bool persistent_failure;
bool forbid_new;
bool Fail() {
  ++count;
  return failure &&
         (count == failure || (persistent_failure && count >= failure));
}
}  // namespace
void SVGTestReset(size_t fail_at, bool persistent) {
  assert(live == 0);
  forbid_new = true;
  count = 0;
  failure = fail_at;
  persistent_failure = persistent;
}
size_t SVGTestAllocations() {
  return count;
}
size_t SVGTestLive() {
  return live;
}
void SVGTestAllowNew() {
  forbid_new = false;
}
extern "C" void* svg_test_malloc(size_t n) {
  if (Fail())
    return nullptr;
  void* result = malloc(n);
  if (result)
    ++live;
  return result;
}
extern "C" void* svg_test_calloc(size_t n, size_t size) {
  if (Fail())
    return nullptr;
  void* result = calloc(n, size);
  if (result)
    ++live;
  return result;
}
extern "C" void* svg_test_realloc(void* p, size_t n) {
  if (Fail())
    return nullptr;
  void* result = realloc(p, n);
  if (result && !p)
    ++live;
  return result;
}
extern "C" void svg_test_free(void* p) {
  if (p) {
    assert(live);
    --live;
  }
  free(p);
}
// A parser accidentally falling back to STL growth must fail this test immediately.
void* operator new(size_t size) {
  if (forbid_new)
    abort();
  void* p = malloc(size ? size : 1);
  if (!p)
    abort();
  return p;
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* p) noexcept {
  free(p);
}
void operator delete[](void* p) noexcept {
  free(p);
}
void operator delete(void* p, size_t) noexcept {
  free(p);
}
void operator delete[](void* p, size_t) noexcept {
  free(p);
}
