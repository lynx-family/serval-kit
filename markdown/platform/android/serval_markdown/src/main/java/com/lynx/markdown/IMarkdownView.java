// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.markdown;

public interface IMarkdownView {
  void requestLayout();
  void requestDraw();
  void measure(float width, int widthMode, float height, int heightMode);
  void align(float left, float top);

  long getSize();
  long getPosition();

  void setSize(float width, float height);
  void setPosition(float left, float top);
  void setVisibility(boolean visible);

  ICustomView getCustomViewHandle();
  IMainView getMainViewHandle();
}
