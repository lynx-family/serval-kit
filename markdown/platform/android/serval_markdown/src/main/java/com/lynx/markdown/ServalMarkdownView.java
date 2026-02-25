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
import android.util.DisplayMetrics;
import android.view.Choreographer;
import android.view.View;
import com.lynx.markdown.tttext.IDrawerCallback;
import com.lynx.markdown.tttext.IRunDelegate;
import com.lynx.markdown.tttext.JavaResourceManager;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
public class ServalMarkdownView
    extends CustomDrawView implements IDrawerCallback {
  protected long mInstance = 0;
  public ServalMarkdownView(Context context) {
    super(context);
    mInstance = nativeCreateInstance(new MarkdownViewHandle(this));
    mResourceManager = new JavaResourceManager();
    mDrawerCallback = this;
    updateDisplayMetrics();
    initialVSync();
  }
  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    if (mInstance != 0) {
      nativeDestroyInstance(mInstance);
    }
  }
  public CustomDrawView createCustomView() {
    CustomDrawView view = new CustomDrawView(getContext());
    view.mResourceManager = mResourceManager;
    view.mDrawerCallback = mDrawerCallback;
    return view;
  }
  public void removeSubView(View view) { removeView(view); }
  public void removeAllSubviews() { removeAllViews(); }
  public float[] getRectInScreen() { return null; }

  public void setContent(String content) {
    nativeSetContent(mInstance, content);
  }
  public void setStyle(HashMap<String, Object> style) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(style);
      byte[] buffer = writer.getBuffer();
      nativeSetStyle(mInstance, buffer);
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public void setAnimationType(int animationType) {
    setNumberProp(Constants.MARKDOWN_PROPS_ANIMATION_TYPE, animationType);
  }
  public void setAnimationVelocity(float velocity) {
    setNumberProp(Constants.MARKDOWN_PROPS_ANIMATION_VELOCITY, velocity);
  }
  public void setInitialAnimationStep(int initialStep) {
    setNumberProp(Constants.MARKDOWN_PROPS_INITIAL_ANIMATION_STEP, initialStep);
  }

  public void setBooleanProp(int key, boolean value) {
    setNumberProp(key, value ? 1 : 0);
  }
  public void setColorProp(int key, int value) { setNumberProp(key, value); }
  public void setNumberProp(int key, double value) {
    nativeSetNumberProp(mInstance, key, value);
  }
  public void setStringProp(int key, String value) {
    nativeSetStringProp(mInstance, key, value);
  }
  public void setArrayProp(int key, ArrayList<Object> object) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeArray(object);
    } catch (IOException e) {
      e.printStackTrace();
    }
    nativeSetValueProp(mInstance, key, writer.getBuffer());
  }

  public void setObjectProp(int key, HashMap<String, Object> object) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(object);
    } catch (IOException e) {
      e.printStackTrace();
    }
    nativeSetValueProp(mInstance, key, writer.getBuffer());
  }

  protected void updateDisplayMetrics() {
    DisplayMetrics metrics = getResources().getDisplayMetrics();
    if (metrics != null) {
      nativeSetDensity(metrics.density);
    }
  }
  protected void initialVSync() {
    Choreographer.getInstance().postFrameCallback(this::onVSync);
  }
  protected void onVSync(long time) {
    nativeOnVSync(mInstance, time / 1000000);
    Choreographer.getInstance().postFrameCallback(this::onVSync);
  }
  private native long nativeCreateInstance(MarkdownViewHandle handle);
  private native void nativeDestroyInstance(long instance);
  private native void nativeSetContent(long instance, String content);
  private native void nativeSetDensity(float density);
  private native void nativeSetStyle(long instance, byte[] buffer);
  private native void nativeOnVSync(long instance, long time);
  private native void nativeSetNumberProp(long instance, int key, double value);
  private native void nativeSetStringProp(long instance, int key, String value);
  private native void nativeSetValueProp(long instance, int key, byte[] value);
  @Override
  public void alignInlineView(MarkdownViewHandle view, Rect rect) {}
}
