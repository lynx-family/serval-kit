// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.model;

public class GradientModel {
  public static int GRADIENT_SPREAD_PAD = 0;
  public static int GRADIENT_SPREAD_REFLECT = 1;
  public static int GRADIENT_SPREAD_REPEAT = 2;
  public static int GRADIENT_TYPE_USE_SPACE_ON_USE = 0;
  public static int GRADIENT_TYPE_OBJECT_BOUNDING_BOX = 1;

  public int mType = GRADIENT_TYPE_OBJECT_BOUNDING_BOX;
  public float[] mTransform;
  public int mSpreadMode;
  public StopModel[] mStopModels;
}
