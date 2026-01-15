// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg;

import android.graphics.Canvas;
import android.graphics.Picture;
import android.graphics.Rect;
import android.graphics.drawable.PictureDrawable;

public class SVGDrawable extends PictureDrawable {
  public SVGDrawable(Picture picture) { super(picture); }

  @Override
  public void draw(Canvas canvas) {
    Rect bounds = getBounds();
    canvas.save();
    if (getPicture() != null) {
      canvas.translate(bounds.left, bounds.top);
      canvas.drawPicture(getPicture());
    }
    canvas.restore();
  }
}
