#!/bin/bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

pushd markdown/third_party/lynx-textra
source tools/envsetup.sh
./tools/hab sync .
python3 ./tools/build_cmake_environment.py --gn-args "is_debug=false use_flutter_cxx=false"
popd
