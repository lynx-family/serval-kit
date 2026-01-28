// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Typeface;
import android.graphics.drawable.Drawable;

public interface IResourceLoader {
  Drawable loadImage(String source);
  Typeface loadFont(String family);
  class InlineView {
    /**
     * creator of inline view
     * @param handle: a handle of view
     * @param verticalAlign: view's vertical align, it's value should be
     *     Constants.VERTICAL_ALIGN_XXX
     */
    public InlineView(MarkdownViewHandle handle, int verticalAlign) {
      mHandle = handle;
      mVerticalAlign = verticalAlign;
    }
    public MarkdownViewHandle mHandle;
    public int mVerticalAlign;
  }
  InlineView loadInlineView(String id);
}
