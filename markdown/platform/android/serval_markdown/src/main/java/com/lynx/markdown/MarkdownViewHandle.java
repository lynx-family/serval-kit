// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.view.View;
public class MarkdownViewHandle {
  private final View mView;
  public MarkdownViewHandle(View view) {
    mView = view;
  }
  View getView() {
    return mView;
  }
  public void requestMeasure() {
    mView.requestLayout();
  }
  public void requestAlign() {
    mView.requestLayout();
  }
  public void requestDraw() {
    mView.invalidate();
  }
  public long measure(int width, int widthMode, int height, int heightMode) {
    int widthSpec = getMeasureSpec(width, Constants.ConvertToMeasureMode(widthMode));
    int heightSpec = getMeasureSpec(height, Constants.ConvertToMeasureMode(heightMode));
    mView.measure(widthSpec, heightSpec);
    int resultWidth = mView.getMeasuredWidth();
    int resultHeight = mView.getMeasuredHeight();
    return MarkdownValuePack.packIntPair(resultWidth, resultHeight);
  }
  public void align(int left, int top) {
    int width = mView.getMeasuredWidth();
    int height = mView.getMeasuredHeight();
    mView.layout(left, top, left + width, top + height);
  }
  public long getSize() {
    int width = mView.getWidth();
    int height = mView.getHeight();
    return MarkdownValuePack.packIntPair(width, height);
  }
  public long getPosition() {
    int left = mView.getLeft();
    int top = mView.getTop();
    return MarkdownValuePack.packIntPair(left, top);
  }
  public void setSize(int width, int height) {
    mView.setRight(mView.getLeft() + width);
    mView.setBottom(mView.getTop() + height);
  }
  public void setPosition(int left, int top) {
    int width = mView.getWidth();
    int height = mView.getHeight();
    mView.setLeft(left);
    mView.setTop(top);
    mView.setRight(left + width);
    mView.setBottom(top + height);
  }
  public void setVisibility(boolean visible) {
    mView.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
  }
  public void attachDrawable(long instance) {
    if (mView instanceof CustomDrawView) {
      ((CustomDrawView) mView).attachDrawable(instance);
    }
  }
  public MarkdownViewHandle createCustomDrawView() {
    if (mView instanceof ServalMarkdownView) {
      return new MarkdownViewHandle(((ServalMarkdownView) mView).createCustomView());
    }
    return null;
  }
  public void removeSubview(MarkdownViewHandle handle) {
    if (mView instanceof ServalMarkdownView) {
      ((ServalMarkdownView) mView).removeSubView(handle.getView());
    }
  }
  public void removeAllSubviews() {
    if (mView instanceof ServalMarkdownView) {
      ((ServalMarkdownView) mView).removeAllSubviews();
    }
  }
  public float[] getRectInScreen() {
    if (mView instanceof ServalMarkdownView) {
      return ((ServalMarkdownView) mView).getRectInScreen();
    }
    return null;
  }
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
