// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.view.View;
import androidx.annotation.Keep;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/** Owns and measures a Markdown document before it is attached to a view. */
@Keep
public final class MarkdownMeasurer {

  public interface RequestMeasureCallback {
    void requestMeasure();
  }

  private long mInstance;
  private final MarkdownResourceManager mResourceManager;
  private IResourceLoader mLoader;
  private IMarkdownEventListener mEventListener;
  private IMarkdownExposureListener mExposureListener;
  private RequestMeasureCallback mRequestMeasureCallback;
  private WeakReference<ServalMarkdownView> mView;
  private boolean mAnimationPaused = false;
  private long mCurrentTimeMs = 0;
  private long mPauseStartTimeMs = 0;
  private long mTotalPausedDurationMs = 0;

  public MarkdownMeasurer(Context context) {
    Markdown.ensureInitialized();
    updateDensity(context);
    mResourceManager = new MarkdownResourceManager();
    mInstance = nativeCreateInstance();
  }

  public void destroy() {
    if (mInstance == 0) {
      return;
    }
    nativeDestroyInstance(mInstance);
    mInstance = 0;
    mAnimationPaused = false;
    mCurrentTimeMs = 0;
    mPauseStartTimeMs = 0;
    mTotalPausedDurationMs = 0;
  }

  long getNativeInstance() { return mInstance; }
  MarkdownResourceManager getResourceManager() { return mResourceManager; }

  static void updateDensity(Context context) {
    DisplayMetrics metrics =
        context == null
            ? android.content.res.Resources.getSystem().getDisplayMetrics()
            : context.getResources().getDisplayMetrics();
    if (metrics != null) {
      nativeSetDensity(metrics.density);
    }
  }

  public long measure(int widthMeasureSpec, int heightMeasureSpec) {
    if (mInstance == 0) {
      return MarkdownValuePack.packMeasureResult(0, 0, 0);
    }
    return nativeMeasureInstance(
        mInstance, View.MeasureSpec.getSize(widthMeasureSpec),
        Constants.ConvertLayoutMode(View.MeasureSpec.getMode(widthMeasureSpec)),
        View.MeasureSpec.getSize(heightMeasureSpec),
        Constants.ConvertLayoutMode(
            View.MeasureSpec.getMode(heightMeasureSpec)));
  }

  void align(int left, int top) {
    if (mInstance != 0) {
      nativeAlignInstance(mInstance, left, top);
    }
  }

  /**
   * Measures Markdown using the legacy one-shot API.
   *
   * @param markdown Markdown source text; {@code null} is treated as empty.
   * @param style the same style map accepted by {@link ServalMarkdownView}.
   * @param maxWidth maximum layout width in pixels.
   * @param maxLines values less than or equal to zero mean unlimited.
   */
  public static MeasureResult measure(String markdown,
                                      HashMap<String, Object> style,
                                      float maxWidth, int maxLines) {
    if (Float.isNaN(maxWidth) || Float.isInfinite(maxWidth) || maxWidth < 0) {
      throw new IllegalArgumentException(
          "maxWidth must be finite and non-negative");
    }
    Markdown.ensureInitialized();
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(style);
    } catch (IOException e) {
      throw new IllegalStateException("Failed to serialize Markdown style", e);
    }
    float density =
        android.content.res.Resources.getSystem().getDisplayMetrics().density;
    return readMeasureResult(
        nativeMeasureLegacy(markdown == null ? "" : markdown,
                            writer.getBuffer(), maxWidth, maxLines, density));
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
  public void setRequestMeasureCallback(RequestMeasureCallback callback) {
    mRequestMeasureCallback = callback;
  }

  public void setContent(String content) {
    if (mInstance != 0) {
      nativeSetContent(mInstance, content == null ? "" : content);
    }
  }
  public void markDirty() {
    if (mInstance != 0) {
      nativeMarkDirty(mInstance);
    }
  }
  public void setTextSelection(int start, int end) {
    if (mInstance != 0) {
      nativeSetTextSelection(mInstance, start, end);
    }
  }
  public void setStyle(HashMap<String, Object> style) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(style);
      if (mInstance != 0) {
        nativeSetStyle(mInstance, writer.getBuffer());
      }
    } catch (IOException e) {
      throw new IllegalStateException("Failed to serialize Markdown style", e);
    }
  }
  public void setAnimationType(int type) {
    setNumberProp(Constants.MARKDOWN_PROPS_ANIMATION_TYPE, type);
  }
  public void setAnimationVelocity(float velocity) {
    setNumberProp(Constants.MARKDOWN_PROPS_ANIMATION_VELOCITY, velocity);
  }
  public void setInitialAnimationStep(int step) {
    setNumberProp(Constants.MARKDOWN_PROPS_INITIAL_ANIMATION_STEP, step);
  }
  public int getAnimationStep() {
    return mInstance == 0 ? 0 : nativeGetAnimationStep(mInstance);
  }
  public void setAnimationStep(int step) {
    if (mInstance != 0) {
      nativeSetAnimationStep(mInstance, step);
    }
  }
  public void pauseAnimation() {
    if (mAnimationPaused) {
      return;
    }
    mAnimationPaused = true;
    mPauseStartTimeMs = mCurrentTimeMs;
  }
  public void resumeAnimation() { resumeAnimation(-1); }
  public void resumeAnimation(int animationStep) {
    if (animationStep != -1) {
      setAnimationStep(animationStep);
    }
    if (!mAnimationPaused) {
      return;
    }
    mAnimationPaused = false;
    if (mPauseStartTimeMs > 0 && mCurrentTimeMs > mPauseStartTimeMs) {
      mTotalPausedDurationMs += mCurrentTimeMs - mPauseStartTimeMs;
    }
  }
  public void onLayoutFrame(long frameTimeNanos) {
    mCurrentTimeMs = frameTimeNanos / 1000000;
    if (!mAnimationPaused && mInstance != 0) {
      nativeOnLayoutFrame(mInstance, mCurrentTimeMs - mTotalPausedDurationMs);
    }
  }
  public void setBooleanProp(int key, boolean value) {
    setNumberProp(key, value ? 1 : 0);
  }
  public void setColorProp(int key, int value) { setNumberProp(key, value); }
  public void setNumberProp(int key, double value) {
    if (mInstance != 0) {
      nativeSetNumberProp(mInstance, key, value);
    }
  }
  public void setStringProp(int key, String value) {
    if (mInstance != 0) {
      nativeSetStringProp(mInstance, key, value);
    }
  }
  public void setArrayProp(int key, ArrayList<Object> value) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeArray(value);
      if (mInstance != 0) {
        nativeSetValueProp(mInstance, key, writer.getBuffer());
      }
    } catch (IOException e) {
      throw new IllegalStateException("Failed to serialize Markdown value", e);
    }
  }
  public void setObjectProp(int key, HashMap<String, Object> value) {
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(value);
      if (mInstance != 0) {
        nativeSetValueProp(mInstance, key, writer.getBuffer());
      }
    } catch (IOException e) {
      throw new IllegalStateException("Failed to serialize Markdown value", e);
    }
  }
  public void onFontLoaded(String family, int weight, int style) {
    if (mInstance != 0 && family != null) {
      nativeOnFontLoaded(mInstance, family, weight, style);
      requestMeasure();
    }
  }
  public void onImageLoaded(String url) {
    if (mInstance != 0 && url != null) {
      nativeOnImageLoaded(mInstance, url);
      requestMeasure();
    }
  }

  int loadImage(String source) {
    if (mLoader == null)
      return 0;
    Drawable drawable = mLoader.loadImage(source);
    return drawable == null ? 0 : mResourceManager.add(drawable);
  }
  long getImageSize(int id) {
    Drawable drawable = mResourceManager.getRunDelegate(id);
    if (drawable == null)
      return MarkdownValuePack.packIntPair(0, 0);
    int width = Math.max(0, drawable.getBounds().width());
    int height = Math.max(0, drawable.getBounds().height());
    if (width == 0)
      width = Math.max(0, drawable.getIntrinsicWidth());
    if (height == 0)
      height = Math.max(0, drawable.getIntrinsicHeight());
    return MarkdownValuePack.packIntPair(width, height);
  }
  IMarkdownViewHandle loadInlineView(String id) {
    return mLoader == null ? null : mLoader.loadInlineView(id);
  }
  long loadFont(String family, int weight, int style) {
    if (mLoader == null)
      return 0;
    Typeface typeface = mLoader.loadFont(family, weight, style);
    return typeface == null
        ? 0
        : mResourceManager.add(typeface, family, weight, style).mIndex;
  }
  void onParseEnd() {
    if (mEventListener != null)
      mEventListener.onParseEnd();
  }
  void onTextOverflow(int overflow) {
    if (mEventListener != null)
      mEventListener.onTextOverflow(overflow);
  }
  void onDrawStart() {
    if (mEventListener != null)
      mEventListener.onDrawStart();
  }
  void onDrawEnd() {
    if (mEventListener != null)
      mEventListener.onDrawEnd();
  }
  void onAnimationStep(int step, int maxStep) {
    if (mEventListener != null)
      mEventListener.onAnimationStep(step, maxStep);
  }
  void onLinkClicked(String url, String content) {
    if (mEventListener != null)
      mEventListener.onLinkClicked(url, content);
  }
  void onImageClicked(String url) {
    if (mEventListener != null)
      mEventListener.onImageClicked(url);
  }
  void onSelectionChanged(int start, int end, int handle, int state) {
    if (mEventListener != null)
      mEventListener.onSelectionChanged(start, end, handle, state);
  }
  void onLinkAppear(String url, String content) {
    if (mExposureListener != null)
      mExposureListener.onLinkAppear(url, content);
  }
  void onLinkDisappear(String url, String content) {
    if (mExposureListener != null)
      mExposureListener.onLinkDisappear(url, content);
  }
  void onImageAppear(String url) {
    if (mExposureListener != null)
      mExposureListener.onImageAppear(url);
  }
  void onImageDisappear(String url) {
    if (mExposureListener != null)
      mExposureListener.onImageDisappear(url);
  }
  void setMarkdownView(ServalMarkdownView view) {
    mView = new WeakReference<>(view);
  }
  void requestMeasure() {
    if (mRequestMeasureCallback != null) {
      mRequestMeasureCallback.requestMeasure();
    } else if (mView != null) {
      ServalMarkdownView view = mView.get();
      if (view != null) {
        view.requestMeasure();
      }
    }
  }

  private static MeasureResult readMeasureResult(byte[] result) {
    try (NativeBufferInputStream stream = new NativeBufferInputStream(
             new ByteArrayInputStream(result == null ? new byte[0] : result))) {
      float width = stream.readFloat();
      float height = stream.readFloat();
      int lineCount = stream.readInt();
      if (lineCount < 0)
        throw new IOException("Invalid Markdown measure line count");
      String[] lines = new String[lineCount];
      for (int i = 0; i < lineCount; ++i)
        lines[i] = stream.readCString();
      return new MeasureResult(width, height, lines);
    } catch (IOException e) {
      throw new IllegalStateException("Failed to read Markdown measure result",
                                      e);
    }
  }
  @Keep
  public static final class MeasureResult {
    private final float mWidth;
    private final float mHeight;
    private final List<String> mLines;
    @Keep
    private MeasureResult(float width, float height, String[] lines) {
      mWidth = width;
      mHeight = height;
      mLines = Collections.unmodifiableList(Arrays.asList(lines));
    }
    public float getWidth() { return mWidth; }
    public float getHeight() { return mHeight; }
    public int getLineCount() { return mLines.size(); }
    public List<String> getLines() { return mLines; }
  }

  private static native byte[] nativeMeasureLegacy(String markdown,
                                                   byte[] style, float maxWidth,
                                                   int maxLines, float density);
  private static native void nativeSetDensity(float density);
  private native long nativeCreateInstance();
  private native void nativeDestroyInstance(long instance);
  private native long nativeMeasureInstance(long instance, float width,
                                            int widthMode, float height,
                                            int heightMode);
  private native void nativeAlignInstance(long instance, float left, float top);
  private native void nativeOnLayoutFrame(long instance, long timeMs);
  private native void nativeSetContent(long instance, String content);
  private native void nativeMarkDirty(long instance);
  private native void nativeSetTextSelection(long instance, int start, int end);
  private native void nativeSetStyle(long instance, byte[] buffer);
  private native int nativeGetAnimationStep(long instance);
  private native void nativeSetAnimationStep(long instance, int step);
  private native void nativeSetNumberProp(long instance, int key, double value);
  private native void nativeSetStringProp(long instance, int key, String value);
  private native void nativeSetValueProp(long instance, int key, byte[] value);
  private native void nativeOnFontLoaded(long instance, String family,
                                         int weight, int style);
  private native void nativeOnImageLoaded(long instance, String url);
  private native void nativeSetExposureListenerEnabled(long instance,
                                                       boolean enabled);
}
