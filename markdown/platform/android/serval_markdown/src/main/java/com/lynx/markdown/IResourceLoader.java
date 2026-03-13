// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Typeface;
import android.graphics.drawable.Drawable;

public interface IResourceLoader {
  Drawable loadImage(String source);
  Typeface loadFont(String family, int weight, int style);
  IMarkdownViewHandle loadInlineView(String id);
}
