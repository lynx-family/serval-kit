# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
source tools/envsetup.sh
chmod +x markdown/tools/initial-markdown.sh
./markdown/tools/initial-markdown.sh
pushd markdown/example/harmony
ohpm install
hvigorw assembleApp --mode project -p product=default -p buildMode=debug --no-daemon
popd
