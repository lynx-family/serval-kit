// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import androidx.annotation.Keep;

@Keep
public interface IMarkdownViewHandle {
  void requestDraw();
  long measure(int width, int widthMode, int height, int heightMode);
  void align(int left, int top);
  // Runtime geometry update on the render/UI thread.
  void setBounds(int left, int top, int width, int height);
  void setVisibility(boolean visible);
  int getVerticalAlign();
  // Offset in pixels for VERTICAL_ALIGN_LENGTH; positive values move the view upward.
  default float getVerticalAlignLength() { return 0; }
}
