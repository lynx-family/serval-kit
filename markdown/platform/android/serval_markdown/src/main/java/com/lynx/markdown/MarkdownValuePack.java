// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.markdown;

public class MarkdownValuePack {
  public static long packIntPair(int first, int second) {
    long result = 0;
    return ((result | first) << 32) | second;
  }
  public static int unpackPairFirst(long value) { return (int)(value >>> 32); }
  public static int unpackPairSecond(long value) {
    return (int)(value & 0xffffffffL);
  }
}
