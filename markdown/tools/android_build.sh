# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
source tools/envsetup.sh
pushd markdown/example/android
./gradlew :app:assembleRelease
popd
