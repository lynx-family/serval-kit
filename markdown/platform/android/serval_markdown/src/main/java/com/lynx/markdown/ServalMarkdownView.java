// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;
import com.lynx.markdown.tttext.IDrawerCallback;
import com.lynx.markdown.tttext.IRunDelegate;
import com.lynx.markdown.tttext.JavaResourceManager;
public class ServalMarkdownView
    extends CustomDrawView implements IMainView, IDrawerCallback {
  protected long mInstance = 0;
  public ServalMarkdownView(Context context) {
    super(context);
    mInstance = nativeCreateInstance();
    mResourceManager = new JavaResourceManager();
    mDrawerCallback = this;
  }
  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    if (mInstance != 0) {
      nativeDestroyInstance(mInstance);
    }
  }
  @Override
  public IMarkdownView createCustomView() {
    CustomDrawView view = new CustomDrawView(getContext());
    view.mResourceManager = mResourceManager;
    view.mDrawerCallback = mDrawerCallback;
    return view;
  }
  @Override
  public void removeSubView(IMarkdownView view) {
    if (view instanceof View) {
      removeView((View)view);
    }
  }
  @Override
  public void removeAllSubviews() {
    removeAllViews();
  }
  @Override
  public float[] getRectInScreen() {
    return null;
  }
  @Override
  public void measure(float width, int widthMode, float height,
                      int heightMode) {
    int widthSpec = getMeasureSpec((int)width, widthMode);
    int heightSpec = getMeasureSpec((int)height, heightMode);
    measure(widthSpec, heightSpec);
  }
  @Override
  public IMainView getMainViewHandle() {
    return this;
  }

  public void setContent(String content) {
    nativeSetContent(mInstance, content);
  }

  private native long nativeCreateInstance();
  private native void nativeDestroyInstance(long instance);
  private native void nativeSetContent(long instance, String content);
  @Override
  public void drawRunDelegate(IRunDelegate delegate, Rect rect) {}
  @Override
  public void drawRunDelegateOnPath(Canvas canvas, IRunDelegate delegate,
                                    Path path, Paint paint) {}
}
