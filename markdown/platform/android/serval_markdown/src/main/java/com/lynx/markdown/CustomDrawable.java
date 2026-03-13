// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Canvas;
public class CustomDrawable {
  public static void draw(long drawable, Canvas canvas,
                          MarkdownResourceManager resourceManager) {
    byte[] buffer = nativeDrawCustomDrawable(drawable);
    MarkdownAndroidCanvasHelper canvasHelper =
        new MarkdownAndroidCanvasHelper(canvas, resourceManager);
    canvasHelper.drawBuffer(buffer);
  }
  public native static long measure(long drawable, float width, int widthMode,
                                    float height, int heightMode);
  private native static byte[] nativeDrawCustomDrawable(long drawable);
}
