// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import com.lynx.markdown.tttext.CBufferOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
public class MarkdownBufferWriter {
  private static final int OBJECT_TYPE_NULL = 0;
  private static final int OBJECT_TYPE_MAP = 1;
  private static final int OBJECT_TYPE_ARRAY = 2;
  private static final int OBJECT_TYPE_BOOLEAN = 3;
  private static final int OBJECT_TYPE_INT = 4;
  private static final int OBJECT_TYPE_LONG = 5;
  private static final int OBJECT_TYPE_DOUBLE = 6;
  private static final int OBJECT_TYPE_STRING = 7;

  private final ByteArrayOutputStream mByteArray;
  private final CBufferOutputStream mStream;
  public MarkdownBufferWriter() {
    mByteArray = new ByteArrayOutputStream();
    mStream = new CBufferOutputStream(mByteArray);
  }
  public void writeMap(HashMap<String, Object> map) throws IOException {
    mStream.writeInt(OBJECT_TYPE_MAP);
    if (map == null) {
      mStream.writeInt(0);
      return;
    }
    mStream.writeInt(map.size());
    for (String key : map.keySet()) {
      mStream.writeString(key);
      Object object = map.get(key);
      writeObject(object);
    }
  }
  public void writeArray(ArrayList<Object> array) throws IOException {
    mStream.writeInt(OBJECT_TYPE_ARRAY);
    if (array == null) {
      mStream.writeInt(0);
      return;
    }
    mStream.writeInt(array.size());
    for (Object object : array) {
      writeObject(object);
    }
  }
  public void writeBoolean(boolean v) throws IOException {
    mStream.writeInt(OBJECT_TYPE_BOOLEAN);
    mStream.writeByte(v ? 1 : 0);
  }
  public void writeInt(int v) throws IOException {
    mStream.writeInt(OBJECT_TYPE_INT);
    mStream.writeInt(v);
  }
  public void writeLong(long v) throws IOException {
    mStream.writeInt(OBJECT_TYPE_LONG);
    mStream.writeLong(v);
  }
  public void writeDouble(double v) throws IOException {
    mStream.writeInt(OBJECT_TYPE_DOUBLE);
    mStream.writeDouble(v);
  }
  public void writeString(String s) throws IOException {
    mStream.writeInt(OBJECT_TYPE_STRING);
    mStream.writeString(s);
  }
  public void writeNull() throws IOException {
    mStream.writeInt(OBJECT_TYPE_NULL);
  }
  public void writeObject(Object object) throws IOException {
    if (object == null) {
      writeNull();
    } else if (object instanceof Boolean) {
      writeBoolean((Boolean)object);
    } else if (object instanceof Integer) {
      writeInt(((Integer)object));
    } else if (object instanceof Long) {
      writeLong((Long)object);
    } else if (object instanceof Number) {
      writeDouble(((Number)object).doubleValue());
    } else if (object instanceof String) {
      writeString((String)object);
    } else if (object instanceof ArrayList) {
      writeArray((ArrayList<Object>)object);
    } else if (object instanceof HashMap) {
      writeMap((HashMap<String, Object>)object);
    } else {
      writeNull();
    }
  }
  public byte[] getBuffer() { return mByteArray.toByteArray(); }
}
