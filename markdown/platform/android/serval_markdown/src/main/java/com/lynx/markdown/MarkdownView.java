// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Keep;

@Keep
public class MarkdownView extends ViewGroup implements IMarkdownViewHandle {
  public MarkdownView(Context context) { super(context); }
  public MarkdownView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }
  public MarkdownView(Context context, AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right,
                          int bottom) {}

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int width = resolveSize(0, widthMeasureSpec);
    int height = resolveSize(0, heightMeasureSpec);
    setMeasuredDimension(width, height);
  }

  public void requestMeasure() { requestLayout(); }
  public void requestAlign() { requestLayout(); }
  public void requestDraw() { invalidate(); }

  public long measure(int width, int widthMode, int height, int heightMode) {
    int widthSpec =
        getMeasureSpec(width, Constants.ConvertToMeasureMode(widthMode));
    int heightSpec =
        getMeasureSpec(height, Constants.ConvertToMeasureMode(heightMode));
    measure(widthSpec, heightSpec);
    int resultWidth = getMeasuredWidth();
    int resultHeight = getMeasuredHeight();
    int baseline = getBaseline();
    if (baseline < 0)
      baseline = resultHeight;
    return MarkdownValuePack.packMeasureResult(resultWidth, resultHeight,
                                               baseline);
  }

  public void align(int left, int top) {
    int width = getMeasuredWidth();
    int height = getMeasuredHeight();
    layout(left, top, left + width, top + height);
  }

  public long getSize() {
    int width = getWidth();
    int height = getHeight();
    return MarkdownValuePack.packIntPair(width, height);
  }

  public long getPosition() {
    int left = getLeft();
    int top = getTop();
    return MarkdownValuePack.packIntPair(left, top);
  }

  public void setSize(int width, int height) {
    setRight(getLeft() + width);
    setBottom(getTop() + height);
  }

  public void setPosition(int left, int top) {
    int width = getWidth();
    int height = getHeight();
    setLeft(left);
    setTop(top);
    setRight(left + width);
    setBottom(top + height);
  }

  public void setVisibility(boolean visible) {
    super.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
  }

  public int getVerticalAlign() { return Constants.VERTICAL_ALIGN_BASELINE; }

  protected static int getMeasureSpec(int value, int mode) {
    int specMode = 0;
    switch (mode) {
      case Constants.LAYOUT_MODE_AT_MOST:
        specMode = View.MeasureSpec.AT_MOST;
        break;
      case Constants.LAYOUT_MODE_DEFINITE:
        specMode = View.MeasureSpec.EXACTLY;
        break;
      case Constants.LAYOUT_MODE_INDEFINITE:
      default:
        break;
    }
    return View.MeasureSpec.makeMeasureSpec(value, specMode);
  }
}
