// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.servalkit.demoMarkdown;

import android.graphics.*;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class CustomCursorRunDelegate {
  private String mId;
  private Drawable mDrawable;
  public CustomCursorRunDelegate(int desireWidth, int desireHeight,
                                 int baseline, String id) {
    mId = id;
    mDrawable = new customTypingDotsDrawable();
  }
}
