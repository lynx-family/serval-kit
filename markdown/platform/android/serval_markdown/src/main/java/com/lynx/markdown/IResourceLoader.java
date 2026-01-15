// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Typeface;
import com.lynx.markdown.tttext.IRunDelegate;
public interface IResourceLoader {
  IRunDelegate loadImage(String source, float desire_width, float desire_height,
                         float max_width, float max_height);
  Typeface loadFont(String family);
  IRunDelegate measureInlineView(String idSelector, float max_width,
                                 float max_height);

  IRunDelegate generateBackground(String backgroundImage, float borderRadius,
                                  float fontSize, float rootFontSize);

  IRunDelegate loadGradient(String gradient, float fontSize,
                            float rootFontSize);
}
