// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Picture;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.PictureDrawable;
import android.os.Looper;
import android.view.Choreographer;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Collections;
import java.util.List;

public class SVGDrawable
    extends PictureDrawable implements Animatable, AutoCloseable {
  @Nullable private final SVGRender.SVGSession mSession;
  private final Rect mRenderRect = new Rect();
  @Nullable private SVGRender.SVGRenderResult mCachedResult;
  @Nullable private SVGRender.SVGRenderResult mLastResult;
  @NonNull
  private List<SVGRender.SVGDiagnostic> mLastDiagnostics =
      Collections.emptyList();
  private int mCachedWidth = -1;
  private int mCachedHeight = -1;
  private final boolean mHasAnimations;
  private boolean mClosed;
  private boolean mRunning;
  private boolean mFrameCallbackPosted;

  private final Choreographer.FrameCallback mFrameCallback =
      new Choreographer.FrameCallback() {
        @Override
        public void doFrame(long frameTimeNanos) {
          mFrameCallbackPosted = false;
          if (mRunning && !mClosed && isVisible()) {
            mRunning =
                mSession != null && mSession.onFrameTimeNanos(frameTimeNanos);
            invalidateSelf();
          }
        }
      };

  public SVGDrawable(Picture picture) {
    super(picture);
    mSession = null;
    mHasAnimations = false;
  }

  public SVGDrawable(@NonNull String content) { this(content, null); }

  public SVGDrawable(@NonNull String content, @Nullable String color) {
    super(null);
    SVGRender render = new SVGRender();
    render.setColor(color);
    mSession = render.createSession(content);
    mHasAnimations = mSession.isValid() && mSession.hasAnimations();
    if (mHasAnimations) {
      start();
    }
  }

  public boolean hasAnimations() { return mHasAnimations; }

  @Nullable
  public SVGRender.SVGRenderResult getLastRenderResult() {
    return mLastResult;
  }

  @NonNull
  public List<SVGRender.SVGDiagnostic> getLastDiagnostics() {
    return mLastDiagnostics;
  }

  @Override
  public void draw(Canvas canvas) {
    if (mClosed) {
      return;
    }
    Rect bounds = getBounds();
    if (bounds.width() <= 0 || bounds.height() <= 0) {
      return;
    }
    if (mHasAnimations) {
      if (drawAnimatedFrame(canvas, bounds)) {
        postFrameCallback();
      }
      return;
    }
    Picture picture = pictureForBounds(bounds);
    if (picture == null) {
      return;
    }
    int saveCount = canvas.save();
    try {
      canvas.translate(bounds.left, bounds.top);
      canvas.drawPicture(picture);
    } finally {
      canvas.restoreToCount(saveCount);
    }
  }

  @Override
  protected void onBoundsChange(Rect bounds) {
    mCachedResult = null;
    mCachedWidth = -1;
    mCachedHeight = -1;
    if (mHasAnimations && mRunning && !mClosed && bounds.width() > 0 &&
        bounds.height() > 0) {
      invalidateSelf();
      postFrameCallback();
    }
  }

  @Override
  public void start() {
    if (!mHasAnimations || mClosed) {
      return;
    }
    if (!mRunning) {
      mSession.startAnimation();
      mRunning = mSession.needsAnimationFrame();
      invalidateSelf();
      postFrameCallback();
    }
  }

  @Override
  public void stop() {
    mRunning = false;
    if (mSession != null) {
      mSession.stopAnimation();
    }
    removeFrameCallback();
  }

  @Override
  public boolean isRunning() {
    return mRunning;
  }

  @Override
  public boolean setVisible(boolean visible, boolean restart) {
    boolean changed = super.setVisible(visible, restart);
    if (visible) {
      if (restart) {
        resetAnimationClock();
      }
      start();
    } else {
      stop();
    }
    return changed;
  }

  @Override
  public void close() {
    if (mClosed) {
      return;
    }
    stop();
    if (mSession != null) {
      mSession.close();
    }
    mClosed = true;
  }

  @Override
  public void setAlpha(int alpha) {}

  @Override
  public void setColorFilter(ColorFilter colorFilter) {}

  @Override
  public int getOpacity() {
    return PixelFormat.TRANSLUCENT;
  }

  @Nullable
  private Picture pictureForBounds(Rect bounds) {
    if (mSession == null) {
      return getPicture();
    }
    if (!mSession.isValid()) {
      return null;
    }
    int width = bounds.width();
    int height = bounds.height();
    mRenderRect.set(0, 0, width, height);
    if (mCachedResult == null || mCachedWidth != width ||
        mCachedHeight != height) {
      mCachedResult = mSession.renderPictureAtTimeWithResult(mRenderRect, 0.0);
      mCachedWidth = width;
      mCachedHeight = height;
    }
    mLastResult = mCachedResult;
    mLastDiagnostics = mCachedResult.diagnostics;
    return mCachedResult.picture;
  }

  private boolean drawAnimatedFrame(@NonNull Canvas canvas, Rect bounds) {
    if (mSession == null || !mSession.isValid()) {
      mLastDiagnostics = Collections.emptyList();
      return false;
    }
    mLastResult = null;
    mRenderRect.set(0, 0, bounds.width(), bounds.height());
    int saveCount = canvas.save();
    try {
      canvas.translate(bounds.left, bounds.top);
      mSession.renderCurrentFrameToCanvas(canvas, mRenderRect);
      mLastDiagnostics = Collections.emptyList();
    } finally {
      canvas.restoreToCount(saveCount);
    }
    return true;
  }

  private void resetAnimationClock() {
    if (mSession != null) {
      mSession.resetAnimationClock();
    }
  }

  private void postFrameCallback() {
    if (!mHasAnimations || !mRunning || mClosed || !isVisible() ||
        mSession == null || !mSession.needsAnimationFrame() ||
        mFrameCallbackPosted || Looper.myLooper() != Looper.getMainLooper()) {
      return;
    }
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
    mFrameCallbackPosted = true;
  }

  private void removeFrameCallback() {
    if (!mFrameCallbackPosted || Looper.myLooper() != Looper.getMainLooper()) {
      return;
    }
    Choreographer.getInstance().removeFrameCallback(mFrameCallback);
    mFrameCallbackPosted = false;
  }
}
