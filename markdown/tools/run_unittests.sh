# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
cmake -S markdown -B markdown/build
cmake --build markdown/build -j --target markdown_tests
./markdown/build/testing/markdown/markdown_tests
