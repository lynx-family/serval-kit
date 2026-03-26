// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.view.Choreographer;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.Keep;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

@Keep
public class ServalMarkdownView extends CustomDrawView {
  protected long mInstance = 0;
  protected IResourceLoader mLoader = null;
  protected IMarkdownEventListener mEventListener = null;
  protected IMarkdownExposureListener mExposureListener = null;
  private final GestureDetector mGestureDetector;
  private float mDownX = 0;
  private float mDownY = 0;
  private boolean mAnimationPaused = false;
  private long mCurrentTime = 0;
  private long mPauseStartTime = 0;
  private long mTotalPausedDurationMs = 0;

  public ServalMarkdownView(Context context) {
    super(context);
    Markdown.ensureInitialized();
    mInstance = nativeCreateInstance();
    mResourceManager = new MarkdownResourceManager();
    updateDisplayMetrics();
    initialVSync();
    setClipChildren(false);
    setClipToPadding(false);
    setClickable(true);
    setLongClickable(true);
    mGestureDetector = new GestureDetector(
        context, new GestureDetector.SimpleOnGestureListener() {
          @Override
          public boolean onDown(MotionEvent e) {
            return true;
          }

          @Override
          public void onLongPress(MotionEvent e) {
            if (mInstance == 0) {
              return;
            }
            boolean consumed =
                nativeDispatchLongPress(mInstance, e.getX(), e.getY());
            if (consumed) {
              disallowParentIntercept(true);
            }
          }

          @Override
          public boolean onSingleTapUp(MotionEvent e) {
            if (mInstance == 0) {
              return false;
            }
            return nativeDispatchTap(mInstance, e.getX(), e.getY());
          }
        });
  }
  public void destroy() {
    if (mInstance != 0) {
      nativeDestroyInstance(mInstance);
      mInstance = 0;
    }
    mAnimationPaused = false;
    mTotalPausedDurationMs = 0;
  }
  public void setResourceLoader(IResourceLoader loader) { mLoader = loader; }
  public void setEventListener(IMarkdownEventListener listener) {
    mEventListener = listener;
  }
  public void setExposureListener(IMarkdownExposureListener listener) {
    mExposureListener = listener;
    if (mInstance != 0) {
      nativeSetExposureListenerEnabled(mInstance, listener != null);
    }
  }
  protected CustomDrawView createCustomView() {
    CustomDrawView view = new CustomDrawView(getContext());
    view.mResourceManager = mResourceManager;
    addView(view);
    return view;
  }
  protected CustomDrawView createRegionView() {
    CustomDrawView view = new CustomDrawView(getContext());
    view.mResourceManager = mResourceManager;
    addView(view, 0);
    return view;
  }
  protected SelectionHandleView createSelectionHandleView(
      long nativePlatformView) {
    SelectionHandleView view =
        new SelectionHandleView(getContext(), nativePlatformView);
    view.mResourceManager = mResourceManager;
    addView(view);
    return view;
  }
  protected void removeSubView(View view) { removeView(view); }
  protected void removeAllSubviews() { removeAllViews(); }
  protected long getVisibleVerticalRangeInScreen() {
    if (getWidth() <= 0 || getHeight() <= 0) {
      return MarkdownValuePack.packIntPair(0, 0);
    }

    Rect globalVisible = new Rect();
    Point globalOffset = new Point();
    if (!getGlobalVisibleRect(globalVisible, globalOffset)) {
      return MarkdownValuePack.packIntPair(0, 0);
    }

    int top = globalVisible.top - globalOffset.y;
    int bottom = globalVisible.bottom - globalOffset.y;
    top = Math.max(0, top);
    bottom = Math.min(getHeight(), bottom);
    if (bottom <= top) {
      return MarkdownValuePack.packIntPair(0, 0);
    }
    return MarkdownValuePack.packIntPair(top, bottom);
  }

  public void setContent(String content) {
    nativeSetContent(mInstance, content);
  }
  public String getContent() {
    if (mInstance == 0) {
      return "";
    }
    return nativeGetDocumentContent(mInstance);
  }
  public String getContentID() {
    if (mInstance == 0) {
      return "";
    }
    return nativeGetContentID(mInstance);
  }
  public String getContent(int start, int end, int indexType) {
    if (mInstance == 0) {
      return "";
    }
    return nativeGetContent(mInstance, start, end, indexType);
  }
  public String getSelectedText() {
    if (mInstance == 0) {
      return "";
    }
    return nativeGetSelectedText(mInstance);
  }
  public String[] getAllImageUrl() {
    if (mInstance == 0) {
      return new String[0];
    }
    return nativeGetAllImageUrl(mInstance);
  }
  public String[] getLinkUrl() {
    if (mInstance == 0) {
      return new String[0];
    }
    return nativeGetLinkUrl(mInstance);
  }
  public String[] getLinkContent() {
    if (mInstance == 0) {
      return new String[0];
    }
    return nativeGetLinkContent(mInstance);
  }
  public ArrayList<RectF> getLinkBoundingRect() {
    ArrayList<RectF> result = new ArrayList<>();
    if (mInstance == 0) {
      return result;
    }
    float[] rects = nativeGetLinkBoundingRect(mInstance);
    for (int i = 0; i + 3 < rects.length; i += 4) {
      result.add(new RectF(rects[i], rects[i + 1], rects[i + 2], rects[i + 3]));
    }
    return result;
  }
  public long[] getSyntaxSourceRanges(String tag) {
    if (mInstance == 0) {
      return new long[0];
    }
    return nativeGetSyntaxSourceRanges(mInstance, tag);
  }
  public long getSelectedRange() {
    if (mInstance == 0) {
      return MarkdownValuePack.packIntPair(-1, -1);
    }
    return nativeGetSelectedRange(mInstance);
  }
  public ArrayList<RectF> getSelectedLineBoundingRect() {
    ArrayList<RectF> result = new ArrayList<>();
    if (mInstance == 0) {
      return result;
    }
    float[] rects = nativeGetSelectedLineBoundingRect(mInstance);
    for (int i = 0; i + 3 < rects.length; i += 4) {
      result.add(new RectF(rects[i], rects[i + 1], rects[i + 2], rects[i + 3]));
    }
    return result;
  }
  public long getSelectionHandlePosition() {
    if (mInstance == 0) {
      return MarkdownValuePack.packIntPair(-1, -1);
    }
    return nativeGetSelectionHandlePosition(mInstance);
  }
  public float getSelectionHandleRadius() {
    if (mInstance == 0) {
      return 0;
    }
    return nativeGetSelectionHandleRadius(mInstance);
  }
  public ArrayList<RectF> getTextBoundingRect(int start, int end,
                                              int indexType) {
    ArrayList<RectF> result = new ArrayList<>();
    if (mInstance == 0) {
      return result;
    }
    float[] rects = nativeGetTextBoundingRect(mInstance, start, end, indexType);
    for (int i = 0; i + 3 < rects.length; i += 4) {
      result.add(new RectF(rects[i], rects[i + 1], rects[i + 2], rects[i + 3]));
    }
    return result;
  }
  public int getCharIndexByPoint(float x, float y, int indexType) {
    if (mInstance == 0) {
      return -1;
    }
    return nativeGetCharIndexByPoint(mInstance, x, y, indexType);
  }
  public long getCharRangeByPoint(float x, float y, int indexType,
                                  int rangeType) {
    if (mInstance == 0) {
      return MarkdownValuePack.packIntPair(-1, -1);
    }
    return nativeGetCharRangeByPoint(mInstance, x, y, indexType, rangeType);
  }
  public void setTextSelection(int start, int end) {
    if (mInstance == 0) {
      return;
    }
    nativeSetTextSelection(mInstance, start, end);
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
  public int getAnimationStep() {
    if (mInstance == 0) {
      return 0;
    }
    return nativeGetAnimationStep(mInstance);
  }
  public void pauseAnimation() {
    if (mAnimationPaused) {
      return;
    }
    mAnimationPaused = true;
    mPauseStartTime = mCurrentTime;
  }
  public void resumeAnimation() { resumeAnimation(-1); }
  public void resumeAnimation(int animationStep) {
    if (animationStep != -1 && mInstance != 0) {
      nativeSetAnimationStep(mInstance, animationStep);
    }
    if (mAnimationPaused) {
      mAnimationPaused = false;
      if (mPauseStartTime > 0 && mCurrentTime > mPauseStartTime) {
        mTotalPausedDurationMs += mCurrentTime - mPauseStartTime;
      }
    }
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
    mCurrentTime = time / 1000000;
    if (!mAnimationPaused && mInstance != 0) {
      long adjustedTimeMs = mCurrentTime - mTotalPausedDurationMs;
      nativeOnVSync(mInstance, adjustedTimeMs);
    }
    Choreographer.getInstance().postFrameCallback(this::onVSync);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    boolean handledByDetector = mGestureDetector.onTouchEvent(event);
    if (mInstance == 0) {
      return handledByDetector || super.onTouchEvent(event);
    }
    int action = event.getActionMasked();
    float x = event.getX();
    float y = event.getY();
    int gestureEventType = 0;
    if (action == MotionEvent.ACTION_DOWN) {
      mDownX = x;
      mDownY = y;
      gestureEventType = 1;
    } else if (action == MotionEvent.ACTION_MOVE) {
      gestureEventType = 2;
    } else if (action == MotionEvent.ACTION_UP) {
      gestureEventType = 3;
    } else if (action == MotionEvent.ACTION_CANCEL) {
      gestureEventType = 4;
    }

    boolean panConsumed = false;
    if (gestureEventType != 0) {
      float motionX = x - mDownX;
      float motionY = y - mDownY;
      panConsumed = nativeDispatchPan(mInstance, x, y, motionX, motionY,
                                      gestureEventType);
      if (panConsumed && (gestureEventType == 1 || gestureEventType == 2)) {
        disallowParentIntercept(true);
      } else if (gestureEventType == 3 || gestureEventType == 4) {
        disallowParentIntercept(false);
      }
    }
    return true;
  }

  protected void disallowParentIntercept(boolean disallow) {
    if (getParent() != null) {
      getParent().requestDisallowInterceptTouchEvent(disallow);
    }
  }
  protected int loadImage(String source) {
    if (mLoader == null)
      return 0;
    Drawable drawable = mLoader.loadImage(source);
    if (drawable == null)
      return 0;
    return mResourceManager.add(drawable);
  }
  protected long getImageSize(int id) {
    Drawable drawable = mResourceManager.getRunDelegate(id);
    if (drawable == null) {
      return MarkdownValuePack.packIntPair(0, 0);
    }
    Rect bounds = drawable.getBounds();
    int width = bounds.width();
    int height = bounds.height();
    if (width <= 0 || height <= 0) {
      width = drawable.getIntrinsicWidth();
      height = drawable.getIntrinsicHeight();
    }
    if (width < 0) {
      width = 0;
    }
    if (height < 0) {
      height = 0;
    }
    return MarkdownValuePack.packIntPair(width, height);
  }
  protected IMarkdownViewHandle loadInlineView(String id) {
    if (mLoader == null)
      return null;
    return mLoader.loadInlineView(id);
  }
  protected long loadFont(String family, int weight, int style) {
    if (mLoader == null)
      return 0;
    Typeface typeface = mLoader.loadFont(family, weight, style);
    if (typeface == null)
      return 0;
    return mResourceManager.add(typeface, family, weight, style).mIndex;
  }

  protected void onParseEnd() {
    if (mEventListener != null)
      mEventListener.onParseEnd();
  }
  protected void onTextOverflow(int overflow) {
    if (mEventListener != null)
      mEventListener.onTextOverflow(overflow);
  }
  protected void onDrawStart() {
    if (mEventListener != null)
      mEventListener.onDrawStart();
  }
  protected void onDrawEnd() {
    if (mEventListener != null)
      mEventListener.onDrawEnd();
  }
  protected void onAnimationStep(int animationStep, int maxAnimationStep) {
    if (mEventListener != null)
      mEventListener.onAnimationStep(animationStep, maxAnimationStep);
  }
  protected void onLinkClicked(String url, String content) {
    if (mEventListener != null)
      mEventListener.onLinkClicked(url, content);
  }
  protected void onImageClicked(String url) {
    if (mEventListener != null)
      mEventListener.onImageClicked(url);
  }
  protected void onSelectionChanged(int startIndex, int endIndex, int handle,
                                    int state) {
    if (mEventListener != null)
      mEventListener.onSelectionChanged(startIndex, endIndex, handle, state);
  }

  protected void onLinkAppear(String url, String content) {
    if (mExposureListener != null)
      mExposureListener.onLinkAppear(url, content);
  }
  protected void onLinkDisappear(String url, String content) {
    if (mExposureListener != null)
      mExposureListener.onLinkDisappear(url, content);
  }
  protected void onImageAppear(String url) {
    if (mExposureListener != null)
      mExposureListener.onImageAppear(url);
  }
  protected void onImageDisappear(String url) {
    if (mExposureListener != null)
      mExposureListener.onImageDisappear(url);
  }
  private native long nativeCreateInstance();
  private native void nativeDestroyInstance(long instance);
  private native void nativeSetContent(long instance, String content);
  private native String nativeGetDocumentContent(long instance);
  private native String nativeGetContentID(long instance);
  private native String nativeGetContent(long instance, int start, int end,
                                         int indexType);
  private native String nativeGetSelectedText(long instance);
  private native String[] nativeGetAllImageUrl(long instance);
  private native String[] nativeGetLinkUrl(long instance);
  private native String[] nativeGetLinkContent(long instance);
  private native float[] nativeGetLinkBoundingRect(long instance);
  private native long[] nativeGetSyntaxSourceRanges(long instance, String tag);
  private native long nativeGetSelectedRange(long instance);
  private native float[] nativeGetSelectedLineBoundingRect(long instance);
  private native long nativeGetSelectionHandlePosition(long instance);
  private native float nativeGetSelectionHandleRadius(long instance);
  private native float[] nativeGetTextBoundingRect(long instance, int start,
                                                   int end, int indexType);
  private native int nativeGetCharIndexByPoint(long instance, float x, float y,
                                               int indexType);
  private native long nativeGetCharRangeByPoint(long instance, float x, float y,
                                                int indexType, int rangeType);
  private native void nativeSetTextSelection(long instance, int start, int end);
  private native void nativeSetDensity(float density);
  private native void nativeSetStyle(long instance, byte[] buffer);
  private native void nativeOnVSync(long instance, long time);
  private native int nativeGetAnimationStep(long instance);
  private native void nativeSetAnimationStep(long instance, int animationStep);
  private native void nativeSetNumberProp(long instance, int key, double value);
  private native void nativeSetStringProp(long instance, int key, String value);
  private native void nativeSetValueProp(long instance, int key, byte[] value);
  private native void nativeSetExposureListenerEnabled(long instance,
                                                       boolean enabled);
  private native boolean nativeDispatchTap(long instance, float x, float y);
  private native boolean nativeDispatchLongPress(long instance, float x,
                                                 float y);
  private native boolean nativeDispatchPan(long instance, float x, float y,
                                           float motionX, float motionY,
                                           int type);
}
