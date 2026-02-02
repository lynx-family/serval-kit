// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Canvas;
import android.view.ViewGroup;
import com.lynx.markdown.tttext.IDrawerCallback;
import com.lynx.markdown.tttext.JavaResourceManager;
public class CustomDrawView extends ViewGroup {
  long mDrawable = 0;
  protected JavaResourceManager mResourceManager = null;
  protected IDrawerCallback mDrawerCallback = null;
  public CustomDrawView(Context context) {
    super(context);
    setWillNotDraw(false);
  }

  @Override
  protected void onLayout(boolean b, int left, int top, int right, int bottom) {
  }
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
    invalidate();
  }

  public void attachDrawable(long instance) { mDrawable = instance; }
  public long getDrawable() { return mDrawable; }
}
