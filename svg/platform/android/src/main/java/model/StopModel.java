// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.model;

public class StopModel {
  public float mOffset;
  public long mColor;
  public float mStopOpacity;

  public StopModel(float offset, long color, float opacity) {
    mOffset = offset;
    mColor = color;
    mStopOpacity = opacity;
  }

  @Override
  public String toString() {
    return "StopModel{"
        + "mOffset=" + mOffset + ", mColor=" + mColor +
        ", mStopOpacity=" + mStopOpacity + '}';
  }
}
