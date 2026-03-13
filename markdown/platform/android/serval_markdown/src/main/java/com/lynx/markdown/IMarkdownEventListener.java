// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

public interface IMarkdownEventListener {
  void onParseEnd();
  void onTextOverflow(int overflow);
  void onDrawStart();
  void onDrawEnd();
  void onAnimationStep(int animationStep, int maxAnimationStep);
  void onLinkClicked(String url, String content);
  void onImageClicked(String url);
  void onSelectionChanged(int startIndex, int endIndex, int handle, int state);
}
