#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SKITY_SOURCE_DIR="${SKITY_SOURCE_DIR:-${REPO_ROOT}/third_party/skity/skity}"
SKITY_BUILD_DIR="${SKITY_BUILD_DIR:-${REPO_ROOT}/build/svg-pattern-percent-skity/dependency}"
TEST_BUILD_DIR="${REPO_ROOT}/build/svg-pattern-percent-skity/runner"
SERVAL_CMAKE_BIN="${SERVAL_CMAKE_BIN:-cmake}"
SERVAL_BUILD_JOBS="${SERVAL_BUILD_JOBS:-4}"
TEST_MANIFEST="${REPO_ROOT}/svg/testing/pattern_percent_cases.json"

usage() {
  cat <<'USAGE'
Run Chrome-reference SVG pattern probes through SrSkityCanvas's software canvas.
Requires macOS, Xcode command-line tools, CMake, and a populated Skity checkout.

Usage: run_skity_pattern_percent_tests.sh [options]
  --skity-source PATH  Skity source with CMakeLists.txt and include/skity
  --skity-build PATH   Dependency build directory (configured and built if needed)
  --build-dir PATH     Test runner build/output directory
  --cmake PATH         CMake executable (or set SERVAL_CMAKE_BIN)
  --manifest PATH      Shared SVG/RGBA manifest (defaults to pattern cases)
  --help              Show this help

Results and per-case PNG files are written beneath the test build's results/.
All manifest cases run, including images; a failed probe returns a nonzero status.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skity-source) SKITY_SOURCE_DIR="$2"; shift 2 ;;
    --skity-build) SKITY_BUILD_DIR="$2"; shift 2 ;;
    --build-dir) TEST_BUILD_DIR="$2"; shift 2 ;;
    --cmake) SERVAL_CMAKE_BIN="$2"; shift 2 ;;
    --manifest) TEST_MANIFEST="$2"; shift 2 ;;
    --help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done

if [[ "$(uname -s)" != Darwin ]]; then
  echo "The Skity pixel runner currently requires macOS ImageIO." >&2
  exit 2
fi
if [[ ! -f "${SKITY_SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "Pass --skity-source with a populated Skity source checkout." >&2
  exit 2
fi
SKITY_SOURCE_DIR="$(cd "${SKITY_SOURCE_DIR}" && pwd)"
mkdir -p "${SKITY_BUILD_DIR}" "${TEST_BUILD_DIR}"
SKITY_BUILD_DIR="$(cd "${SKITY_BUILD_DIR}" && pwd)"
TEST_BUILD_DIR="$(cd "${TEST_BUILD_DIR}" && pwd)"

cd "${REPO_ROOT}"
source tools/envsetup.sh
./hab sync . --target testing
"${SERVAL_CMAKE_BIN}" -S "${SKITY_SOURCE_DIR}" -B "${SKITY_BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug -DSKITY_HW_RENDERER=ON -DSKITY_SW_RENDERER=ON \
  -DSKITY_GL_BACKEND=OFF -DSKITY_MTL_BACKEND=ON -DSKITY_CT_FONT=ON \
  -DSKITY_CODEC_MODULE=OFF -DSKITY_IO_MODULE=OFF -DSKITY_CAPI_MODULE=OFF \
  -DSKITY_EXAMPLE=OFF -DSKITY_TEST=OFF -DSKITY_BUILD_INSTALL=OFF
"${SERVAL_CMAKE_BIN}" --build "${SKITY_BUILD_DIR}" --target skity --parallel "${SERVAL_BUILD_JOBS}"
"${SERVAL_CMAKE_BIN}" -S "${REPO_ROOT}/svg/testing/skity" -B "${TEST_BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug -DSKITY_SOURCE_DIR="${SKITY_SOURCE_DIR}" \
  -DSKITY_BUILD_DIR="${SKITY_BUILD_DIR}"
"${SERVAL_CMAKE_BIN}" --build "${TEST_BUILD_DIR}" --parallel "${SERVAL_BUILD_JOBS}"
"${TEST_BUILD_DIR}/pattern_percent_runner" \
  "${TEST_MANIFEST}" \
  "${REPO_ROOT}/svg/test_cases" "${TEST_BUILD_DIR}/results"
