// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Canvas;
import android.view.ViewGroup;
import com.lynx.markdown.tttext.IDrawerCallback;
import com.lynx.markdown.tttext.JavaResourceManager;
public class CustomDrawView
    extends ViewGroup implements IMarkdownView, ICustomView {
  int mLeft = 0;
  int mTop = 0;
  int mWidth = 0;
  int mHeight = 0;
  long mDrawable = 0;
  protected JavaResourceManager mResourceManager = null;
  protected IDrawerCallback mDrawerCallback = null;
  public CustomDrawView(Context context) {
    super(context);
    setWillNotDraw(false);
  }

  @Override
  protected void onLayout(boolean b, int i, int i1, int i2, int i3) {}
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    CustomDrawable.draw(mDrawable, canvas, mResourceManager, mDrawerCallback);
  }
  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    int width = MeasureSpec.getSize(widthMeasureSpec);
    int wMode = MeasureSpec.getMode(widthMeasureSpec);
    int height = MeasureSpec.getSize(heightMeasureSpec);
    int hMode = MeasureSpec.getMode(heightMeasureSpec);
    long size = CustomDrawable.measure(
        mDrawable, width, Constants.ConvertLayoutMode(wMode), height,
        Constants.ConvertLayoutMode(hMode));
    int resultWidth = MarkdownValuePack.unpackPairFirst(size);
    int resultHeight = MarkdownValuePack.unpackPairSecond(size);
    setMeasuredDimension(resultWidth, resultHeight);
  }
  @Override
  public void attachDrawable(long instance) {
    mDrawable = instance;
  }
  @Override
  public long getDrawable() {
    return mDrawable;
  }
  @Override
  public void requestDraw() {
    invalidate();
  }
  @Override
  public void measure(float width, int widthMode, float height,
                      int heightMode) {
    int widthSpec =
        getMeasureSpec((int)width, Constants.ConvertToMeasureMode(widthMode));
    int heightSpec =
        getMeasureSpec((int)height, Constants.ConvertToMeasureMode(heightMode));
    measure(widthSpec, heightSpec);
    mWidth = getMeasuredWidth();
    mHeight = getMeasuredHeight();
  }
  @Override
  public void align(float left, float top) {
    layout((int)left, (int)top, (int)left + mWidth, (int)top + mHeight);
    mLeft = (int)left;
    mTop = (int)top;
  }
  @Override
  public long getSize() {
    return MarkdownValuePack.packIntPair(getWidth(), getHeight());
  }
  @Override
  public long getPosition() {
    return MarkdownValuePack.packIntPair(getLeft(), getTop());
  }
  @Override
  public void setSize(float width, float height) {
    mWidth = (int)width;
    mHeight = (int)height;
    updateViewRect();
  }
  @Override
  public void setPosition(float left, float top) {
    mLeft = (int)left;
    mTop = (int)top;
    updateViewRect();
  }
  @Override
  public void setVisibility(boolean visible) {
    setVisibility(visible ? VISIBLE : INVISIBLE);
  }
  @Override
  public ICustomView getCustomViewHandle() {
    return this;
  }
  @Override
  public IMainView getMainViewHandle() {
    return null;
  }

  protected void updateViewRect() {
    setLeft(mLeft);
    setTop(mTop);
    setRight(mLeft + mWidth);
    setBottom(mTop + mHeight);
  }
  protected int getMeasureSpec(int value, int mode) {
    int specMode = 0;
    switch (mode) {
      case Constants.LAYOUT_MODE_AT_MOST:
        specMode = MeasureSpec.AT_MOST;
        break;
      case Constants.LAYOUT_MODE_DEFINITE:
        specMode = MeasureSpec.EXACTLY;
        break;
      case Constants.LAYOUT_MODE_INDEFINITE:
      default:
        break;
    }
    return MeasureSpec.makeMeasureSpec(value, specMode);
  }
}
