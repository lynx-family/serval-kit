// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.servalkit.demoMarkdown;

import android.content.Context;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.util.Log;
import android.view.ViewGroup;
import androidx.core.content.ContextCompat;
import com.lynx.markdown.*;
import java.util.ArrayList;
import java.util.HashMap;

public class CustomDemoView extends ViewGroup {
  public CustomCursorRunDelegate mCustomCursorRunDelegate;
  public static String mCustomCursorId = "custom-cursor-01";

  private boolean mEnableTextSelection;
  Drawable mSelectionLeftCursor;
  Drawable mSelectionRightCursor;

  float mMarkdownLayoutWidth;
  float mMarkdownLayoutHeight;

  public CustomDemoView(Context context) {
    super(context);

    mCustomCursorRunDelegate =
        new CustomCursorRunDelegate(50, 50, 0, mCustomCursorId);
  }

  @Override
  protected void onLayout(boolean b, int i, int i1, int i2, int i3) {}

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

    //you can do measureMarkdownLayout anywhere
    setMeasuredDimension((int)mMarkdownLayoutWidth, (int)mMarkdownLayoutHeight);
  }

  public void setContentComplete(boolean complete) {}

  public void enableTextSelection(boolean enable) {
    mEnableTextSelection = enable;
    if (mEnableTextSelection) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        mSelectionLeftCursor = ContextCompat.getDrawable(
            getContext(), com.lynx.servalkit.R.drawable
                              .lynx_text_select_handle_left_material);
        mSelectionRightCursor = ContextCompat.getDrawable(
            getContext(), com.lynx.servalkit.R.drawable
                              .lynx_text_select_handle_right_material);
      } else {
        mSelectionLeftCursor = ContextCompat.getDrawable(
            getContext(), com.lynx.servalkit.R.drawable
                              .lynx_text_select_handle_left_material);
        mSelectionRightCursor = ContextCompat.getDrawable(
            getContext(), com.lynx.servalkit.R.drawable
                              .lynx_text_select_handle_right_material);
      }
    }
  }

  // Should call it after measured.
  public int getMaxStep() { return 0; }

  // Get the parsed plain text for current markdown text.
  public String getParsedContent() { return ""; }
}
