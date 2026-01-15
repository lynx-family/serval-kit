// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.markdown;

import android.view.View;
public class Constants {
  public static final int LAYOUT_MODE_INDEFINITE = 0;
  public static final int LAYOUT_MODE_DEFINITE = 1;
  public static final int LAYOUT_MODE_AT_MOST = 2;

  public static int ConvertLayoutMode(int mode) {
    switch (mode) {
      case View.MeasureSpec.EXACTLY:
        return LAYOUT_MODE_DEFINITE;
      case View.MeasureSpec.AT_MOST:
        return LAYOUT_MODE_AT_MOST;
      case View.MeasureSpec.UNSPECIFIED:
      default:
        return LAYOUT_MODE_INDEFINITE;
    }
  }
  public static int ConvertToMeasureMode(int mode) {
    switch (mode) {
      case LAYOUT_MODE_AT_MOST:
        return View.MeasureSpec.AT_MOST;
      case LAYOUT_MODE_DEFINITE:
        return View.MeasureSpec.EXACTLY;
      case LAYOUT_MODE_INDEFINITE:
      default:
        return View.MeasureSpec.UNSPECIFIED;
    }
  }
}
