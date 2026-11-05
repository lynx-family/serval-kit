// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.view.Choreographer;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.UiThread;
import java.util.ArrayList;
import java.util.HashMap;

@Keep
public class ServalMarkdownView extends MarkdownGestureView {
  private static final Handler UI_THREAD_HANDLER =
      new Handler(Looper.getMainLooper());
  protected long mInstance = 0;
  protected MarkdownMeasurer mMeasurer = null;
  private boolean mOwnsMeasurer = false;
  private boolean mDisableInternalVSync = false;
  private boolean mInternalVSyncPosted = false;
  private boolean mExternalMeasure = false;
  private long mExternalMeasuredSize = 0;
  private final Choreographer.FrameCallback mInternalVSyncCallback =
      this::onVSync;

  public ServalMarkdownView(Context context) { this(context, true); }
  public ServalMarkdownView(Context context, boolean createMeasurer) {
    super(context, 0);
    Markdown.ensureInitialized();
    updateDisplayMetrics();
    mInstance = nativeCreateInstance();
    setNativeGestureTarget(mInstance);
    if (createMeasurer) {
      setMarkdownMeasurer(new MarkdownMeasurer(context));
      mOwnsMeasurer = true;
    }
    initialVSync();
    setClipChildren(false);
    setClipToPadding(false);
  }
  /** Releases this view and its internally created measurer. */
  public void destroy() {
    mDisableInternalVSync = true;
    clearInternalVSync();
    attachDrawable(0);
    if (mInstance != 0) {
      nativeDestroyInstance(mInstance);
      mInstance = 0;
      setNativeGestureTarget(0);
    }
    if (mMeasurer != null) {
      mMeasurer.clearMarkdownView(this);
      if (mOwnsMeasurer) {
        mMeasurer.destroy();
      }
    }
    mMeasurer = null;
    mOwnsMeasurer = false;
  }
  /**
   * Attaches a borrowed measurer. Callers must finish layout and renderer
   * operations before destroying either object. The caller remains responsible
   * for destroying the supplied measurer.
   */
  public void setMarkdownMeasurer(MarkdownMeasurer measurer) {
    if (mInstance == 0 || mMeasurer != null || measurer == null ||
        measurer.getNativeInstance() == 0) {
      return;
    }
    mResourceManager = measurer.getResourceManager();
    measurer.setMarkdownView(this);
    nativeSetMarkdownMeasurer(mInstance, measurer.getNativeInstance());
    mMeasurer = measurer;
  }
  public MarkdownMeasurer getMarkdownMeasurer() { return mMeasurer; }
  /**
   * UI-thread API. Enables host-managed document measurement and alignment.
   * Disabled by default.
   */
  public void setExternalMeasure(boolean external) {
    mExternalMeasure = external;
    requestMeasure();
  }
  /**
   * UI-thread API. Supplies the host's measured size in physical pixels, packed
   * as returned by {@link MarkdownMeasurer#measure(int, int)}.
   */
  public void setExternalMeasuredSize(long size) {
    mExternalMeasuredSize = size;
    requestMeasure();
  }
  /**
   * UI-thread API. Enables host-managed visible render regions. Disabled by
   * default. Disabling restores automatic calculation on the next frame.
   */
  public void setExternalRenderRegion(boolean external) {
    if (mInstance != 0) {
      nativeSetExternalRenderRegion(mInstance, external);
    }
  }
  /**
   * UI-thread API. Supplies the visible rectangle in view-local physical pixels
   * before rendering a frame with external render-region control enabled. The
   * rectangle is copied and retained across frames until replaced.
   */
  public void setRenderRegion(RectF region) {
    if (mInstance != 0) {
      nativeSetRenderRegion(mInstance, region.left, region.top, region.right,
                            region.bottom);
    }
  }
  public void setResourceLoader(IResourceLoader loader) {
    if (mMeasurer != null)
      mMeasurer.setResourceLoader(loader);
  }
  public void setEventListener(IMarkdownEventListener listener) {
    if (mMeasurer != null)
      mMeasurer.setEventListener(listener);
  }
  public void setExposureListener(IMarkdownExposureListener listener) {
    if (mMeasurer != null)
      mMeasurer.setExposureListener(listener);
  }
  public void requestMeasure() {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      requestLayout();
    } else {
      UI_THREAD_HANDLER.post(this::requestLayout);
    }
  }
  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (mExternalMeasure) {
      setMeasuredDimension(
          MarkdownValuePack.unpackMeasureResultWidth(mExternalMeasuredSize),
          MarkdownValuePack.unpackMeasureResultHeight(mExternalMeasuredSize));
      invalidate();
    } else {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
  }
  @Override
  public int getBaseline() {
    return mExternalMeasure
        ? MarkdownValuePack.unpackMeasureResultBaseline(mExternalMeasuredSize)
        : super.getBaseline();
  }
  @Override
  protected void alignDrawable(int left, int top) {
    if (!mExternalMeasure && mMeasurer != null) {
      mMeasurer.align(left, top);
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
  protected CustomDrawView createScrollXRegionView() {
    return createRegionView();
  }
  protected CustomDrawView createSelectionHandleView() {
    return createCustomView();
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
    if (mMeasurer != null)
      mMeasurer.setContent(content);
  }
  public void markDirty() {
    if (mMeasurer != null)
      mMeasurer.markDirty();
  }
  public String getContent() {
    return mInstance == 0 ? "" : nativeGetDocumentContent(mInstance);
  }
  public String getContentID() {
    return mInstance == 0 ? "" : nativeGetContentID(mInstance);
  }
  public String getContent(int start, int end, int indexType) {
    return mInstance == 0 ? ""
                          : nativeGetContent(mInstance, start, end, indexType);
  }
  public String getSelectedText() {
    return mInstance == 0 ? "" : nativeGetSelectedText(mInstance);
  }
  public String[] getAllImageUrl() {
    return mInstance == 0 ? new String[0] : nativeGetAllImageUrl(mInstance);
  }
  public String[] getLinkUrl() {
    return mInstance == 0 ? new String[0] : nativeGetLinkUrl(mInstance);
  }
  public String[] getLinkContent() {
    return mInstance == 0 ? new String[0] : nativeGetLinkContent(mInstance);
  }
  public ArrayList<RectF> getLinkBoundingRect() {
    return convertRects(mInstance == 0 ? new float[0]
                                       : nativeGetLinkBoundingRect(mInstance));
  }
  public long[] getSyntaxSourceRanges(String tag) {
    return mInstance == 0 ? new long[0]
                          : nativeGetSyntaxSourceRanges(mInstance, tag);
  }
  public long getSelectedRange() {
    return mInstance == 0 ? MarkdownValuePack.packIntPair(-1, -1)
                          : nativeGetSelectedRange(mInstance);
  }
  public ArrayList<RectF> getSelectedLineBoundingRect() {
    return convertRects(mInstance == 0
                            ? new float[0]
                            : nativeGetSelectedLineBoundingRect(mInstance));
  }
  public long getSelectionHandlePosition() {
    return mInstance == 0 ? MarkdownValuePack.packIntPair(-1, -1)
                          : nativeGetSelectionHandlePosition(mInstance);
  }
  public float getSelectionHandleRadius() {
    return mInstance == 0 ? 0 : nativeGetSelectionHandleRadius(mInstance);
  }
  public ArrayList<RectF> getTextBoundingRect(int start, int end,
                                              int indexType) {
    return convertRects(mInstance == 0 ? new float[0]
                                       : nativeGetTextBoundingRect(
                                             mInstance, start, end, indexType));
  }
  public int getCharIndexByPoint(float x, float y, int indexType) {
    return mInstance == 0
        ? -1
        : nativeGetCharIndexByPoint(mInstance, x, y, indexType);
  }
  public long getCharRangeByPoint(float x, float y, int indexType,
                                  int rangeType) {
    return mInstance == 0
        ? MarkdownValuePack.packIntPair(-1, -1)
        : nativeGetCharRangeByPoint(mInstance, x, y, indexType, rangeType);
  }
  /** UI-thread API. Enables rendering the document using region subviews. */
  @UiThread
  public void setEnableRegionView(boolean enable) {
    if (mInstance != 0) {
      nativeSetEnableRegionView(mInstance, enable);
    }
  }

  public void setTextSelection(int start, int end) {
    if (mMeasurer != null)
      mMeasurer.setTextSelection(start, end);
  }
  public void setStyle(HashMap<String, Object> style) {
    if (mMeasurer != null)
      mMeasurer.setStyle(style);
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
    return mMeasurer == null ? 0 : mMeasurer.getAnimationStep();
  }
  public int getRenderedAnimationStep() {
    return mInstance == 0 ? 0 : nativeGetRenderedAnimationStep(mInstance);
  }
  public void pauseRenderUpdate() {
    if (mInstance != 0) {
      nativePauseRenderUpdate(mInstance);
    }
  }
  public void resumeRenderUpdate() {
    if (mInstance != 0) {
      nativeResumeRenderUpdate(mInstance);
    }
  }

  public void setBooleanProp(int key, boolean value) {
    setNumberProp(key, value ? 1 : 0);
  }
  public void setColorProp(int key, int value) {
    if (mMeasurer != null)
      mMeasurer.setColorProp(key, value);
  }
  public void setNumberProp(int key, double value) {
    if (mMeasurer != null)
      mMeasurer.setNumberProp(key, value);
  }
  public void setStringProp(int key, String value) {
    if (mMeasurer != null)
      mMeasurer.setStringProp(key, value);
  }
  public void setArrayProp(int key, ArrayList<Object> object) {
    if (mMeasurer != null)
      mMeasurer.setArrayProp(key, object);
  }

  public void setObjectProp(int key, HashMap<String, Object> object) {
    if (mMeasurer != null)
      mMeasurer.setObjectProp(key, object);
  }

  protected void updateDisplayMetrics() {
    MarkdownMeasurer.updateDensity(getContext());
  }
  protected void initialVSync() { postInternalVSync(); }
  protected void onVSync(long frameTimeNanos) {
    mInternalVSyncPosted = false;
    if (mMeasurer != null) {
      mMeasurer.onLayoutFrame(frameTimeNanos);
    }
    onRendererFrame(frameTimeNanos);
    postInternalVSync();
  }

  public void disableInternalVSync(boolean disable) {
    if (mDisableInternalVSync == disable) {
      return;
    }
    mDisableInternalVSync = disable;
    if (disable) {
      clearInternalVSync();
    } else {
      postInternalVSync();
    }
  }

  public void onRendererFrame(long frameTimeNanos) {
    if (mInstance != 0) {
      long adjustedTimeMs = frameTimeNanos / 1000000;
      nativeOnRendererFrame(mInstance, adjustedTimeMs);
    }
  }

  public void onFontLoaded(String family, int weight, int style) {
    if (mMeasurer != null)
      mMeasurer.onFontLoaded(family, weight, style);
  }

  public void onImageLoaded(String url) {
    if (mMeasurer != null)
      mMeasurer.onImageLoaded(url);
  }

  private void postInternalVSync() {
    if (mDisableInternalVSync || mInternalVSyncPosted) {
      return;
    }
    mInternalVSyncPosted = true;
    Choreographer.getInstance().postFrameCallback(mInternalVSyncCallback);
  }

  private void clearInternalVSync() {
    if (!mInternalVSyncPosted) {
      return;
    }
    Choreographer.getInstance().removeFrameCallback(mInternalVSyncCallback);
    mInternalVSyncPosted = false;
  }

  private static ArrayList<RectF> convertRects(float[] rects) {
    ArrayList<RectF> result = new ArrayList<>();
    for (int i = 0; i + 3 < rects.length; i += 4) {
      result.add(new RectF(rects[i], rects[i + 1], rects[i + 2], rects[i + 3]));
    }
    return result;
  }

  private native long nativeCreateInstance();
  private native void nativeSetMarkdownMeasurer(long instance,
                                                long measurerInstance);
  private native void nativeDestroyInstance(long instance);
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
  private native int nativeGetRenderedAnimationStep(long instance);
  private native void nativePauseRenderUpdate(long instance);
  private native void nativeResumeRenderUpdate(long instance);
  private native void nativeSetEnableRegionView(long instance, boolean enable);
  private native void nativeOnRendererFrame(long instance, long time);
  private native void nativeSetExternalRenderRegion(long instance,
                                                    boolean external);
  private native void nativeSetRenderRegion(long instance, float left,
                                            float top, float right,
                                            float bottom);
}
