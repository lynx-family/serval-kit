// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.model;

public class PaintRef {
  public static final int PAINT_NONE = 0;
  public static final int PAINT_COLOR = 1;
  public static final int PAINT_IRI = 2;
  public int mType;
  public String mIri;
  public long mColor;
  public float mOpacity;

  public PaintRef(int type, long color, float opacity, String iri) {
    mType = type;
    mColor = color;
    mOpacity = opacity;
    mIri = iri;
  }
}
