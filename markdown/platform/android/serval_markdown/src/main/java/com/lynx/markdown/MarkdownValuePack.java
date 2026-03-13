// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.markdown;

public class MarkdownValuePack {
  /**
   * Packs (width, height, baseline) into a 64-bit long using 21 bits each:
   *
   * <pre>
   * [ width:21 ][ height:21 ][ baseline:21 ]
   * </pre>
   *
   * Each field is stored as an unsigned 21-bit integer, so the maximum supported value is:
   * 2^21 - 1 = 2,097,151.
   */
  public static long packMeasureResult(int width, int height, int baseline) {
    final long mask = (1L << 21) - 1L;
    return (((long)width & mask) << 42) | (((long)height & mask) << 21) |
        ((long)baseline & mask);
  }
  public static long packIntPair(int first, int second) {
    long result = 0;
    return ((result | first) << 32) | second;
  }
  public static int unpackPairFirst(long value) { return (int)(value >>> 32); }
  public static int unpackPairSecond(long value) {
    return (int)(value & 0xffffffffL);
  }
}
