#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -eo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TEST_BUILD_DIR="${SERVAL_MARKER_TEST_BUILD_DIR:-${TMPDIR:-/tmp}/serval-svg-marker-tests}"
mkdir -p "${TEST_BUILD_DIR}"
cd "${REPO_ROOT}"
source tools/envsetup.sh
if [[ "${SERVAL_MARKER_SKIP_SYNC:-0}" != 1 ]]; then
  ./hab sync . --target testing
fi
GTEST_ROOT="${REPO_ROOT}/markdown/third_party/googletest/googletest"
CC="${CC:-clang}"
CXX="${CXX:-clang++}"
COMMON_FLAGS=(-g -O1 -DSR_SVG_MIN_LOG_LEVEL=6 -I"${REPO_ROOT}/svg/include")
if [[ "${SERVAL_MARKER_SANITIZE:-0}" == 1 ]]; then
  COMMON_FLAGS+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  export ASAN_OPTIONS="detect_stack_use_after_return=1${ASAN_OPTIONS:+:${ASAN_OPTIONS}}"
fi
OBJECTS=()
for SOURCE in svg/src/element/*.c svg/src/parser/*.c svg/src/utils/*.c; do
  OBJECT="${TEST_BUILD_DIR}/$(basename "${SOURCE}").o"
  "${CC}" -std=c11 "${COMMON_FLAGS[@]}" -c "${SOURCE}" -o "${OBJECT}"
  OBJECTS+=("${OBJECT}")
done
"${CXX}" -std=c++17 "${COMMON_FLAGS[@]}" \
  -I"${GTEST_ROOT}/include" -I"${GTEST_ROOT}" \
  svg/src/element/*.cc svg/src/parser/*.cc svg/src/utils/*.cc \
  svg/testing/marker_tests.cc "${GTEST_ROOT}/src/gtest-all.cc" \
  "${GTEST_ROOT}/src/gtest_main.cc" "${OBJECTS[@]}" -pthread \
  -o "${TEST_BUILD_DIR}/marker_tests"
"${TEST_BUILD_DIR}/marker_tests" "$@"
