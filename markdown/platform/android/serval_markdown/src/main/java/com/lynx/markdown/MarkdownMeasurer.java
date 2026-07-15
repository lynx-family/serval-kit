// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.content.res.Resources;
import androidx.annotation.Keep;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/** Measures Markdown content without creating a {@link ServalMarkdownView}. */
@Keep
public final class MarkdownMeasurer {
  private MarkdownMeasurer() {}

  /**
   * Measures Markdown using the same parser, style, and line-breaking logic as
   * {@link ServalMarkdownView}.
   *
   * @param markdown Markdown source text; {@code null} is treated as empty
   *     content
   * @param style the same style map accepted by
   *     {@link ServalMarkdownView#setStyle(HashMap)}; {@code null} uses the
   *     default style
   * @param maxWidth maximum layout width in pixels; must be finite and
   *     non-negative
   * @param maxLines maximum number of lines; values less than or equal to zero
   *     mean unlimited
   */
  public static MeasureResult measure(String markdown,
                                      HashMap<String, Object> style,
                                      float maxWidth, int maxLines) {
    if (Float.isNaN(maxWidth) || Float.isInfinite(maxWidth) || maxWidth < 0) {
      throw new IllegalArgumentException(
          "maxWidth must be finite and non-negative");
    }
    Markdown.ensureInitialized();
    MarkdownBufferWriter writer = new MarkdownBufferWriter();
    try {
      writer.writeMap(style);
    } catch (IOException e) {
      throw new IllegalStateException("Failed to serialize Markdown style", e);
    }
    float density = Resources.getSystem().getDisplayMetrics().density;
    byte[] result =
        nativeMeasure(markdown == null ? "" : markdown, writer.getBuffer(),
                      maxWidth, maxLines, density);
    try (NativeBufferInputStream stream =
             new NativeBufferInputStream(new ByteArrayInputStream(result))) {
      float width = stream.readFloat();
      float height = stream.readFloat();
      int lineCount = stream.readInt();
      if (lineCount < 0) {
        throw new IOException("Invalid Markdown measure line count");
      }
      String[] lines = new String[lineCount];
      for (int index = 0; index < lineCount; ++index) {
        lines[index] = stream.readCString();
      }
      return new MeasureResult(width, height, lines);
    } catch (IOException e) {
      throw new IllegalStateException("Failed to read Markdown measure result",
                                      e);
    }
  }

  @Keep
  public static final class MeasureResult {
    private final float mWidth;
    private final float mHeight;
    private final List<String> mLines;

    @Keep
    private MeasureResult(float width, float height, String[] lines) {
      mWidth = width;
      mHeight = height;
      mLines = Collections.unmodifiableList(Arrays.asList(lines));
    }

    public float getWidth() { return mWidth; }

    public float getHeight() { return mHeight; }

    public int getLineCount() { return mLines.size(); }

    public List<String> getLines() { return mLines; }
  }

  private static native byte[] nativeMeasure(String markdown, byte[] style,
                                             float maxWidth, int maxLines,
                                             float density);
}
