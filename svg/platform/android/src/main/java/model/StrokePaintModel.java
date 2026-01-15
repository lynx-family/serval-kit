// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.model;

public class StrokePaintModel extends PaintRef {

  public float mWith;

  public int mStrokeLineCap;  //  BUTT    (0), ROUND   (1), SQUARE  (2);

  public int mStrokeLineJoin;  //  MITER   (0), ROUND   (1),BEVEL   (2);

  public float mStrokeMiterLimit;

  public float[] mStrokeDashArray;

  public float mStrokeDashOffset;

  public StrokePaintModel(int type, String iri, long color, float width,
                          float opacity, int strokeLineCap, int strokeLineJoin,
                          float strokeMiterLimit, float strokeDashOffset,
                          float[] strokeDashArray) {
    super(type, color, opacity, iri);
    mWith = width;
    mStrokeLineCap = strokeLineCap;
    mStrokeLineJoin = strokeLineJoin;
    mStrokeMiterLimit = strokeMiterLimit;
    mStrokeDashOffset = strokeDashOffset;
    mStrokeDashArray = strokeDashArray;
  }
}
