// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import com.lynx.markdown.tttext.IRunDelegate;
import com.lynx.textra.TTTextDefinition;

public class MarkdownRunDelegate implements IRunDelegate {
  int mDesireWidth = 0;
  int mDesireHeight = 0;

  MarkdownRunDelegate(int desireWidth, int desireHeight) {
    mDesireWidth = desireWidth;
    mDesireHeight = desireHeight;
  }

  @Override
  public float getAscent() {
    return -mDesireHeight;
  }
  @Override
  public float getDescent() {
    return 0;
  }
  @Override
  public float getAdvance() {
    return mDesireWidth;
  }
}
