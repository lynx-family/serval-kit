// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown.tttext;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;

public interface IDrawerCallback {
  void drawRunDelegate(IRunDelegate delegate, Rect rect);
  void drawRunDelegateOnPath(Canvas canvas, IRunDelegate delegate, Path path,
                             Paint paint);
}
