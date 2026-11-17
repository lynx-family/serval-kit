#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TEST_BUILD_DIR="${REPO_ROOT}/build/ios-pattern-tests"
TEST_DESTINATION="${SERVAL_IOS_TEST_DESTINATION:-}"

if [[ "${1:-}" == "--help" ]]; then
  cat <<'HELP'
Usage: bash svg/tools/run_ios_pattern_tests.sh [xcodebuild-destination]

Requires Xcode with an iOS simulator runtime, Ruby, CocoaPods and xcodeproj.
Use the repository's Bundler environment, or put those Ruby tools on PATH.
An explicit destination may also be set with SERVAL_IOS_TEST_DESTINATION.
Otherwise a booted simulator, or the first available iPhone, is selected.
SERVAL_IOS_PATTERN_CASE_FILTER optionally selects fixture filenames by regex.

The shared Chrome manifest and SVG fixtures are copied into a generated XCTest
bundle under ignored build/ios-pattern-tests. Results include the rendered PNGs.
HELP
  exit 0
fi
if [[ $# -gt 0 ]]; then
  TEST_DESTINATION="$1"
fi

cd "${REPO_ROOT}"
source tools/envsetup.sh
./hab sync .

if [[ -z "${TEST_DESTINATION}" ]]; then
  TEST_DEVICE_ID="$(xcrun simctl list devices available --json | python3 -c '
import json, sys
devices = [device for runtime, values in json.load(sys.stdin)["devices"].items()
           if ".iOS-" in runtime for device in values if device.get("isAvailable")]
device = next((d for d in devices if d["state"] == "Booted"), None)
device = device or next((d for d in devices if "iPhone" in d["name"]), None)
if device is None:
    sys.exit("No available iOS simulator; install a runtime or pass a destination.")
print(device["udid"])
')"
  TEST_DESTINATION="platform=iOS Simulator,id=${TEST_DEVICE_ID}"
fi

ruby "${REPO_ROOT}/svg/testing/ios/generate_project.rb" "${TEST_BUILD_DIR}"
pod install --project-directory="${TEST_BUILD_DIR}"
TEST_RESULT_PATH="${TEST_BUILD_DIR}/results-$(date +%Y%m%d-%H%M%S).xcresult"
xcodebuild -workspace "${TEST_BUILD_DIR}/PatternTests.xcworkspace" \
  -scheme PatternTests -configuration Debug \
  -destination "${TEST_DESTINATION}" \
  -derivedDataPath "${TEST_BUILD_DIR}/DerivedData" \
  -resultBundlePath "${TEST_RESULT_PATH}" \
  IPHONEOS_DEPLOYMENT_TARGET=16.4 CODE_SIGNING_ALLOWED=NO \
  test | tee "${TEST_BUILD_DIR}/test.log"
