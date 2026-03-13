// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

public interface IMarkdownExposureListener {
  void onLinkAppear(String url, String content);
  void onLinkDisappear(String url, String content);
  void onImageAppear(String url);
  void onImageDisappear(String url);
}
