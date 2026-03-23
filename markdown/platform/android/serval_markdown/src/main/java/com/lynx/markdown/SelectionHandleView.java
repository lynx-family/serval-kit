// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.Context;
import android.view.MotionEvent;
import androidx.annotation.Keep;

@Keep
public class SelectionHandleView extends CustomDrawView {
  private float mDownX = 0;
  private float mDownY = 0;
  private long mNativePlatformView = 0;

  public SelectionHandleView(Context context, long nativePlatformView) {
    super(context);
    mNativePlatformView = nativePlatformView;
    setClickable(true);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mNativePlatformView == 0) {
      return false;
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

    boolean consumed = false;
    if (gestureEventType != 0) {
      float motionX = x - mDownX;
      float motionY = y - mDownY;
      consumed = nativeDispatchPanByPlatformView(
          mNativePlatformView, x, y, motionX, motionY, gestureEventType);
      if (consumed && (gestureEventType == 1 || gestureEventType == 2)) {
        if (getParent() != null) {
          getParent().requestDisallowInterceptTouchEvent(true);
        }
      } else if (gestureEventType == 3 || gestureEventType == 4) {
        if (getParent() != null) {
          getParent().requestDisallowInterceptTouchEvent(false);
        }
      }
    }
    return true;
  }

  private native boolean nativeDispatchPanByPlatformView(long platformView,
                                                         float x, float y,
                                                         float motionX,
                                                         float motionY,
                                                         int type);
}
