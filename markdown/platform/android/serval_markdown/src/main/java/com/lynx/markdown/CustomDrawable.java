// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Canvas;
import com.lynx.markdown.tttext.IDrawerCallback;
import com.lynx.markdown.tttext.JavaResourceManager;
import com.lynx.markdown.tttext.MarkdownJavaCanvasHelper;
public class CustomDrawable {
  public static void draw(long drawable, Canvas canvas,
                          JavaResourceManager resourceManager,
                          IDrawerCallback drawerCallback) {
    byte[] buffer = nativeDrawCustomDrawable(drawable);
    MarkdownJavaCanvasHelper canvasHelper =
        new MarkdownJavaCanvasHelper(canvas, resourceManager, drawerCallback);
    canvasHelper.drawBuffer(buffer);
  }
  public native static long measure(long drawable, float width, int widthMode,
                                    float height, int heightMode);
  private native static byte[] nativeDrawCustomDrawable(long drawable);
}
