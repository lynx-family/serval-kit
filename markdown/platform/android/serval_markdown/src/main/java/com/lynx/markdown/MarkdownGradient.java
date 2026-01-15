// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

public class MarkdownGradient extends MarkdownRunDelegate {
  private final String mGradient;
  public MarkdownGradient(String gradient) {
    super(0, 0);
    mGradient = gradient;
  }
  public String getGradient() { return mGradient; }
}
