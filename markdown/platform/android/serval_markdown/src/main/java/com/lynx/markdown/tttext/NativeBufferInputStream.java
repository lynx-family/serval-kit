// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown.tttext;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public class NativeBufferInputStream extends DataInputStream {
  /**
   * Creates a DataInputStream that uses the specified
   * underlying InputStream.
   *
   * @param in the specified input stream
   */
  public NativeBufferInputStream(InputStream in) {
    super(in);
  }

  public String readCString() throws IOException {
    int tag_l = readInt();
    if (tag_l == 0)
      return "";
    byte[] tag_b = new byte[tag_l];
    if (read(tag_b, 0, tag_l) != -1) {
      return new String(tag_b, StandardCharsets.UTF_8);
    }
    return "";
  }
}
