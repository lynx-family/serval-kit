#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

source tools/envsetup.sh
check_args=()
if [[ $# -gt 0 ]]; then
  check_args=("$1")
fi
python3 tools_shared/git_lynx.py check "${check_args[@]}" --checkers file-type,cpplint,java-lint,commit-message,coding-style,android-check-style,copyright
