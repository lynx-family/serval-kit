# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
source tools/envsetup.sh
pushd markdown/example/ios/ServalMarkdownDemo
bundle install
bundle exec pod install
xcodebuild -workspace ServalMarkdownDemo.xcworkspace -scheme ServalMarkdownDemo -configuration Release -arch arm64 -sdk iphoneos SYMROOT=$(pwd)/build CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
popd
