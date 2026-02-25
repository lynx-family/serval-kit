// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.markdown;

import android.view.View;
public class Constants {
  public static final int LAYOUT_MODE_INDEFINITE = 0;
  public static final int LAYOUT_MODE_DEFINITE = 1;
  public static final int LAYOUT_MODE_AT_MOST = 2;

  public static final int VERTICAL_ALIGN_TOP = 0;
  public static final int VERTICAL_ALIGN_CENTER = 1;
  public static final int VERTICAL_ALIGN_BASELINE = 2;
  public static final int VERTICAL_ALIGN_BOTTOM = 3;
  public static final int MARKDOWN_PROPS_ANIMATION_TYPE = 0;
  public static final int MARKDOWN_PROPS_ANIMATION_VELOCITY = 1;
  public static final int MARKDOWN_PROPS_TEXT_MAXLINE = 2;
  public static final int MARKDOWN_PROPS_CONTENT_COMPLETE = 3;
  public static final int MARKDOWN_PROPS_TYPEWRITER_DYNAMIC_HEIGHT = 4;
  public static final int MARKDOWN_PROPS_INITIAL_ANIMATION_STEP = 5;
  public static final int MARKDOWN_PROPS_MARKDOWN_MAX_HEIGHT = 6;
  public static final int MARKDOWN_PROPS_CONTENT_RANGE_START = 7;
  public static final int MARKDOWN_PROPS_CONTENT_RANGE_END = 8;
  public static final int MARKDOWN_PROPS_EXPOSURE_TAGS = 9;
  public static final int MARKDOWN_PROPS_ANIMATION_FRAME_RATE = 10;
  public static final int MARKDOWN_PROPS_TYPEWRITER_HEIGHT_TRANSITION_DURATION =
      11;
  public static final int MARKDOWN_PROPS_ALLOW_BREAK_AROUND_PUNCTUATION = 12;
  public static final int MARKDOWN_PROPS_ENABLE_TEXT_SELECTION = 13;
  public static final int MARKDOWN_PROPS_SELECTION_HIGHLIGHT_COLOR = 14;
  public static final int MARKDOWN_PROPS_SELECTION_HANDLE_COLOR = 15;
  public static final int MARKDOWN_PROPS_SELECTION_HANDLE_SIZE = 16;
  public static final int MARKDOWN_PROPS_SELECTION_HANDLE_TOUCH_MARGIN = 17;
  public static final int MARKDOWN_PROPS_MARKDOWN_EFFECT = 18;
  public static final int MARKDOWN_PROPS_TEXT_MARK_ATTACHMENTS = 19;

  public static final int ANIMATION_TYPE_NONE = 0;
  public static final int ANIMATION_TYPE_TYPEWRITER = 1;

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
