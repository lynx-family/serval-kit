// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown.tttext;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class NativeBufferOutputStream extends DataOutputStream {
  public NativeBufferOutputStream(OutputStream stream) { super(stream); }

  public void writeString(String string) throws IOException {
    byte[] bytes = string.getBytes(StandardCharsets.UTF_8);
    writeInt(bytes.length);
    write(bytes, 0, bytes.length);
  }
}
