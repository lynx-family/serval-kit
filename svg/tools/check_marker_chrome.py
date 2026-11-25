#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Verify or regenerate the marker pixel references using Chrome."""

import check_pattern_percent_chrome as reference

reference.MANIFEST = reference.ROOT / "svg/testing/marker_cases.json"
if __name__ == "__main__":
    reference.main(
        default_output=reference.ROOT / "build/marker-chrome",
        generator_command="python3 svg/tools/check_marker_chrome.py --update",
    )
