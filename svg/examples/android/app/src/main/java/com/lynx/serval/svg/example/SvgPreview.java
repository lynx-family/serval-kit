// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Rect;
import android.util.Log;
import android.util.Xml;
import android.widget.ImageView;
import com.lynx.serval.svg.SVGDrawable;
import com.lynx.serval.svg.SVGRender;
import java.io.IOException;
import java.io.StringReader;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

// Image examples are static previews. Re-record their Picture when resources
// arrive, without adding resource-loading or cache APIs to SVGDrawable.
final class SvgPreview implements AutoCloseable {
  private final String content;
  private final ImageView view;
  private final Rect viewport;
  private final SVGRender imageRender;
  private SVGDrawable drawable;
  private boolean closed;

  SvgPreview(String content, String color, ImageView view, int width,
             int height, SvgImageLoader imageLoader) {
    this.content = content;
    this.view = view;
    viewport = new Rect(0, 0, width, height);
    if (containsImage(content)) {
      imageRender = new SVGRender();
      imageRender.setColor(color);
      imageRender.setResourceManager(
          imageLoader.resourceManager(this::renderImage));
      renderImage();
    } else {
      // Keep the existing session and animation scheduling for all other cases.
      imageRender = null;
      drawable = new SVGDrawable(content, color);
      view.setImageDrawable(drawable);
    }
  }

  private static boolean containsImage(String content) {
    XmlPullParser parser = Xml.newPullParser();
    try {
      parser.setInput(new StringReader(content));
      for (int event = parser.next(); event != XmlPullParser.END_DOCUMENT;
           event = parser.next()) {
        if (event == XmlPullParser.START_TAG &&
            "image".equals(parser.getName())) {
          return true;
        }
      }
    } catch (IOException | XmlPullParserException ignored) {
      // Leave malformed documents to the renderer's existing diagnostics path.
    }
    return false;
  }

  private void renderImage() {
    if (closed) {
      return;
    }
    SVGRender.SVGRenderResult result =
        imageRender.renderPictureWithResult(content, viewport);
    if (result.hasError) {
      Log.w("SvgPreview", "SVG rendering failed: " + result.errorMessage);
    }
    if (drawable != null) {
      drawable.close();
    }
    drawable = new SVGDrawable(result.picture);
    view.setImageDrawable(drawable);
  }

  @Override
  public void close() {
    closed = true;
    if (drawable != null) {
      drawable.close();
    }
  }
}
