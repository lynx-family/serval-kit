// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.annotation.Keep;

@Keep
public class MarkdownGestureView extends CustomDrawView {

  private long mNativeGestureTarget = 0;
  private final int mTouchSlop;
  private float mDownX = 0;
  private float mDownY = 0;
  private boolean mIsValidClick = false;
  private CheckForLongPress mCheckForLongPress = null;
  private static final int PAN_STATE_IDLE = 0;
  private static final int PAN_STATE_PENDING = 1;
  private static final int PAN_STATE_TRACKING = 2;
  private int mPanState = PAN_STATE_IDLE;
  private boolean mParentInterceptDisallowed = false;

  public MarkdownGestureView(Context context, long nativeGestureTarget) {
    super(context);
    mNativeGestureTarget = nativeGestureTarget;
    mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    setClickable(true);
    setLongClickable(true);
  }

  protected void setNativeGestureTarget(long nativeGestureTarget) {
    mNativeGestureTarget = nativeGestureTarget;
  }

  private final class CheckForLongPress implements Runnable {
    private final float mX;
    private final float mY;

    public CheckForLongPress(float x, float y) {
      mX = x;
      mY = y;
    }

    @Override
    public void run() {
      longPressSuccess(mX, mY);
      // clear self
      mCheckForLongPress = null;
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mNativeGestureTarget == 0) {
      return super.onTouchEvent(event);
    }
    int action = event.getActionMasked();
    float x = event.getX();
    float y = event.getY();
    float deltaX = x - mDownX;
    float deltaY = y - mDownY;
    switch (action) {
      case MotionEvent.ACTION_DOWN:
        mDownX = x;
        mDownY = y;
        if (!checkShouldPan(x, y, 0, 0)) {
          mIsValidClick = true;
          removeCheckLongPressCallback();
          mCheckForLongPress = new CheckForLongPress(x, y);
          postDelayed(mCheckForLongPress,
                      ViewConfiguration.getLongPressTimeout());
        }
        break;
      case MotionEvent.ACTION_MOVE:
        if (mPanState == PAN_STATE_PENDING) {
          float slop = mTouchSlop * mTouchSlop;
          if (deltaX * deltaX + deltaY * deltaY >= slop) {
            mIsValidClick = false;
            if (!checkShouldPan(mDownX, mDownY, deltaX, deltaY)) {
              mPanState = PAN_STATE_IDLE;
            }
          }
        }
        if (mPanState == PAN_STATE_TRACKING) {
          nativeDispatchPan(mNativeGestureTarget, x, y, deltaX, deltaY,
                            convertGestureEventType(MotionEvent.ACTION_MOVE));
        }
        break;
      case MotionEvent.ACTION_UP:
        if (mIsValidClick) {
          nativeDispatchTap(mNativeGestureTarget, mDownX, mDownY);
        }
        if (mPanState == PAN_STATE_TRACKING) {
          nativeDispatchPan(mNativeGestureTarget, x, y, deltaX, deltaY,
                            convertGestureEventType(MotionEvent.ACTION_UP));
        }
        resetState();
        break;
      case MotionEvent.ACTION_CANCEL:
        if (mPanState == PAN_STATE_TRACKING) {
          nativeDispatchPan(mNativeGestureTarget, x, y, deltaX, deltaY,
                            convertGestureEventType(MotionEvent.ACTION_CANCEL));
        }
        resetState();
        break;
    }
    return true;
  }

  boolean checkShouldPan(float x, float y, float motionX, float motionY) {
    boolean result =
        nativeShouldBeginPan(mNativeGestureTarget, x, y, motionX, motionY);
    if (result) {
      mPanState = PAN_STATE_TRACKING;
      nativeDispatchPan(mNativeGestureTarget, x, y, motionX, motionY,
                        convertGestureEventType(MotionEvent.ACTION_DOWN));
      setInterceptAllowed(false);
      removeCheckLongPressCallback();
    }
    return result;
  }

  void longPressSuccess(float x, float y) {
    mIsValidClick = false;
    boolean accept = nativeDispatchLongPress(mNativeGestureTarget, x, y);
    if (accept) {
      mPanState = PAN_STATE_PENDING;
      setInterceptAllowed(false);
    }
  }

  void resetState() {
    mDownX = 0;
    mDownY = 0;
    mPanState = PAN_STATE_PENDING;
    mIsValidClick = false;
    removeCheckLongPressCallback();
    releaseIntercept();
  }

  private void removeCheckLongPressCallback() {
    if (mCheckForLongPress == null) {
      return;
    }

    removeCallbacks(mCheckForLongPress);
    mCheckForLongPress = null;
  }

  private static int convertGestureEventType(int action) {
    if (action == MotionEvent.ACTION_DOWN) {
      return 1;
    }
    if (action == MotionEvent.ACTION_MOVE) {
      return 2;
    }
    if (action == MotionEvent.ACTION_UP) {
      return 3;
    }
    if (action == MotionEvent.ACTION_CANCEL) {
      return 4;
    }
    return 0;
  }

  private void setInterceptAllowed(boolean allowed) {
    requestDisallowInterceptTouchEvent(!allowed);
    mParentInterceptDisallowed = !allowed;
  }

  private void releaseIntercept() {
    if (!mParentInterceptDisallowed) {
      return;
    }
    setInterceptAllowed(true);
  }

  private native boolean nativeDispatchTap(long target, float x, float y);
  private native boolean nativeDispatchLongPress(long target, float x, float y);
  private native boolean nativeShouldBeginPan(long target, float x, float y,
                                              float motionX, float motionY);
  private native boolean nativeDispatchPan(long target, float x, float y,
                                           float motionX, float motionY,
                                           int type);
}
