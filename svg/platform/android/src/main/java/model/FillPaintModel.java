// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.model;

public class FillPaintModel extends PaintRef {
  public int mFillRule;

  public FillPaintModel(int type, String iri, long color, float opacity,
                        int rule) {
    super(type, color, opacity, iri);
    mFillRule = rule;
  }
}
