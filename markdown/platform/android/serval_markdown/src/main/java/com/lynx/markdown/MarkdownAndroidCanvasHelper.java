// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import com.lynx.textra.BBufferInputStream;
import com.lynx.textra.JavaCanvasHelper;
import com.lynx.textra.TTText;
import java.io.IOException;
import java.util.ArrayList;

public class MarkdownAndroidCanvasHelper extends JavaCanvasHelper {
  protected static final int CANVAS_OP_EXTEND = -1;
  protected static final int CANVAS_OP_CLIP_PATH = 0;
  protected static final int CANVAS_OP_DRAW_PATH = 1;
  protected static final int CANVAS_OP_DRAW_DELEGATE_ON_PATH = 2;
  protected static final int CANVAS_OP_DRAW_LINEAR_GRADIENT_ON_RECT = 3;
  protected static final int CANVAS_OP_DRAW_LINEAR_GRADIENT_ON_PATH = 4;
  protected static final int PATH_TYPE_ARC = 0;
  protected static final int PATH_TYPE_OVAL = 1;
  protected static final int PATH_TYPE_RECT = 2;
  protected static final int PATH_TYPE_ROUND_RECT = 3;
  protected static final int PATH_TYPE_MOVE_TO = 4;
  protected static final int PATH_TYPE_LINE_TO = 5;
  protected static final int PATH_TYPE_CUBIC_TO = 6;
  protected static final int PATH_TYPE_QUAD_TO = 7;

  protected MarkdownResourceManager mResourceManager;
  protected ArrayList<PointF> mTranslateStack = new ArrayList<>();
  protected float mTranslateX;
  protected float mTranslateY;

  public MarkdownAndroidCanvasHelper(Canvas canvas,
                                     MarkdownResourceManager manager) {
    super(TTText.mFontManager);
    canvas_ = canvas;
    mResourceManager = manager;
  }

  public void setCanvas(Canvas canvas) { canvas_ = canvas; }

  public void drawBuffer(byte[] input) { super.drawBuffer(canvas_, input); }

  @Override
  protected void drawOp(int op, BBufferInputStream stream) throws IOException {
    if (op == CANVAS_OP_EXTEND) {
      drawExtendOp(stream);
    } else {
      super.drawOp(op, stream);
    }
  }

  @Override
  protected void save() {
    super.save();
    mTranslateStack.add(new PointF(mTranslateX, mTranslateY));
  }

  @Override
  protected void restore() {
    super.restore();
    PointF translate = mTranslateStack.get(mTranslateStack.size() - 1);
    mTranslateX = translate.x;
    mTranslateY = translate.y;
    mTranslateStack.remove(mTranslateStack.size() - 1);
  }

  @Override
  protected void translate(BBufferInputStream stream) throws IOException {
    float x = stream.readFloat();
    float y = stream.readFloat();
    canvas_.translate(x, y);
    mTranslateX += x;
    mTranslateY += y;
  }

  public float getTranslateX() { return mTranslateX; }

  public float getTranslateY() { return mTranslateY; }

  protected void drawExtendOp(BBufferInputStream stream) throws IOException {
    int op = stream.readByte();
    if (op == CANVAS_OP_CLIP_PATH) {
      clipPath(stream);
    } else if (op == CANVAS_OP_DRAW_PATH) {
      drawMarkdownPath(stream);
    } else if (op == CANVAS_OP_DRAW_DELEGATE_ON_PATH) {
      drawDelegateOnPath(stream);
    } else if (op == CANVAS_OP_DRAW_LINEAR_GRADIENT_ON_RECT) {
      drawLinearGradientOnRect(stream);
    } else if (op == CANVAS_OP_DRAW_LINEAR_GRADIENT_ON_PATH) {
      drawLinearGradientOnPath(stream);
    }
  }

  protected void clipPath(BBufferInputStream stream) throws IOException {
    Path path = readPath(stream);
    canvas_.clipPath(path);
  }

  protected void drawMarkdownPath(BBufferInputStream stream)
      throws IOException {
    Path path = readPath(stream);
    Paint p = readPaint(stream, paint_);
    applyPaintStyle(p);
    canvas_.drawPath(path, p);
  }

  protected void drawDelegateOnPath(BBufferInputStream stream)
      throws IOException {
    int id = stream.readInt();
    Path path = readPath(stream);
    Paint p = readPaint(stream, paint_);
    p.setColor(color_);
    if (color_ != 0 && stroke_color_ == color_) {
      p.setStyle(Paint.Style.FILL_AND_STROKE);
      drawRunDelegateOnPath(canvas_, mResourceManager.getRunDelegate(id), path,
                            p);
    } else {
      if (color_ != 0) {
        p.setStyle(Paint.Style.FILL);
        drawRunDelegateOnPath(canvas_, mResourceManager.getRunDelegate(id),
                              path, p);
      }
      if (stroke_color_ != 0) {
        p.setStyle(Paint.Style.STROKE);
        p.setColor(stroke_color_);
        drawRunDelegateOnPath(canvas_, mResourceManager.getRunDelegate(id),
                              path, p);
      }
    }
  }

  protected void drawLinearGradientOnRect(BBufferInputStream stream)
      throws IOException {
    Shader shader = readLinearGradient(stream);
    float left = stream.readFloat();
    float top = stream.readFloat();
    float right = stream.readFloat();
    float bottom = stream.readFloat();
    Paint p = readPaint(stream, paint_);
    applyLinearGradient(p, shader);
    if (shader == null) {
      return;
    }
    canvas_.drawRect(left, top, right, bottom, p);
    p.setShader(null);
  }

  protected void drawLinearGradientOnPath(BBufferInputStream stream)
      throws IOException {
    Shader shader = readLinearGradient(stream);
    Path path = readPath(stream);
    skipGradientBounds(stream);
    Paint p = readPaint(stream, paint_);
    applyLinearGradient(p, shader);
    if (shader == null) {
      return;
    }
    canvas_.drawPath(path, p);
    p.setShader(null);
  }

  protected Path readPath(BBufferInputStream stream) throws IOException {
    Path path = createPath();
    int op_count = stream.readInt();
    for (int i = 0; i < op_count; i++) {
      int op_type = stream.readByte();
      if (op_type == PATH_TYPE_ARC) {
        addArc(path, stream);
      } else if (op_type == PATH_TYPE_OVAL) {
        addOval(path, stream);
      } else if (op_type == PATH_TYPE_RECT) {
        addRect(path, stream);
      } else if (op_type == PATH_TYPE_ROUND_RECT) {
        addRoundRect(path, stream);
      } else if (op_type == PATH_TYPE_MOVE_TO) {
        moveTo(path, stream);
      } else if (op_type == PATH_TYPE_LINE_TO) {
        lineTo(path, stream);
      } else if (op_type == PATH_TYPE_CUBIC_TO) {
        cubicTo(path, stream);
      } else if (op_type == PATH_TYPE_QUAD_TO) {
        quadTo(path, stream);
      }
    }
    return path;
  }

  protected Shader readLinearGradient(BBufferInputStream stream)
      throws IOException {
    float startX = stream.readFloat();
    float startY = stream.readFloat();
    float endX = stream.readFloat();
    float endY = stream.readFloat();

    int colorCount = stream.readInt();
    int[] colors = new int[colorCount];
    for (int i = 0; i < colorCount; i++) {
      colors[i] = stream.readInt();
    }

    int stopCount = stream.readInt();
    float[] stops = stopCount == colorCount ? new float[stopCount] : null;
    for (int i = 0; i < stopCount; i++) {
      float stop = stream.readFloat();
      if (stops != null) {
        stops[i] = stop;
      }
    }
    if (colors.length < 2) {
      return null;
    }
    return new LinearGradient(startX, startY, endX, endY, colors, stops,
                              Shader.TileMode.CLAMP);
  }

  protected void skipGradientBounds(BBufferInputStream stream)
      throws IOException {
    stream.readFloat();
    stream.readFloat();
    stream.readFloat();
    stream.readFloat();
  }

  protected void applyPaintStyle(Paint painter) {
    if (color_ != 0 && stroke_color_ == color_) {
      painter.setColor(color_);
      painter.setStyle(Paint.Style.FILL_AND_STROKE);
    } else if (color_ != 0) {
      painter.setColor(color_);
      painter.setStyle(Paint.Style.FILL);
    } else if (stroke_color_ != 0) {
      painter.setColor(stroke_color_);
      painter.setStyle(Paint.Style.STROKE);
    } else {
      painter.setStyle(Paint.Style.FILL);
    }
  }

  protected void applyLinearGradient(Paint painter, Shader shader) {
    painter.setShader(shader);
    painter.setColor(0xffffffff);
    boolean hasFill = color_ != 0;
    boolean hasStroke = stroke_color_ != 0;
    if (hasFill && hasStroke) {
      painter.setStyle(Paint.Style.FILL_AND_STROKE);
    } else if (hasStroke) {
      painter.setStyle(Paint.Style.STROKE);
    } else {
      painter.setStyle(Paint.Style.FILL);
    }
  }

  protected void addArc(Path path, BBufferInputStream stream) {
    float cx = stream.readFloat();
    float cy = stream.readFloat();
    float r = stream.readFloat();
    float start = stream.readFloat();
    float end = stream.readFloat();
    path.addArc(cx - r, cy - r, cx + r, cy + r, start, end - start);
  }
  protected void addOval(Path path, BBufferInputStream stream) {
    float left = stream.readFloat();
    float top = stream.readFloat();
    float right = stream.readFloat();
    float bottom = stream.readFloat();
    path.addOval(left, top, right, bottom, Path.Direction.CW);
  }
  protected void addRect(Path path, BBufferInputStream stream) {
    float left = stream.readFloat();
    float top = stream.readFloat();
    float right = stream.readFloat();
    float bottom = stream.readFloat();
    path.addRect(left, top, right, bottom, Path.Direction.CW);
  }
  protected void addRoundRect(Path path, BBufferInputStream stream) {
    float left = stream.readFloat();
    float top = stream.readFloat();
    float right = stream.readFloat();
    float bottom = stream.readFloat();
    float radiusX = stream.readFloat();
    float radiusY = stream.readFloat();
    path.addRoundRect(left, top, right, bottom, radiusX, radiusY,
                      Path.Direction.CW);
  }
  protected void moveTo(Path path, BBufferInputStream stream) {
    float x = stream.readFloat();
    float y = stream.readFloat();
    path.moveTo(x, y);
  }
  protected void lineTo(Path path, BBufferInputStream stream) {
    float x = stream.readFloat();
    float y = stream.readFloat();
    path.lineTo(x, y);
  }
  protected void cubicTo(Path path, BBufferInputStream stream) {
    float c1x = stream.readFloat();
    float c1y = stream.readFloat();
    float c2x = stream.readFloat();
    float c2y = stream.readFloat();
    float x = stream.readFloat();
    float y = stream.readFloat();
    path.cubicTo(c1x, c1y, c2x, c2y, x, y);
  }
  protected void quadTo(Path path, BBufferInputStream stream) {
    float cx = stream.readFloat();
    float cy = stream.readFloat();
    float x = stream.readFloat();
    float y = stream.readFloat();
    path.quadTo(cx, cy, x, y);
  }

  @Override
  protected void drawRunDelegate(BBufferInputStream stream) {
    int id = stream.readInt();
    float dl = stream.readFloat();
    float dt = stream.readFloat();
    float dr = stream.readFloat();
    float db = stream.readFloat();
    float radius = stream.readFloat();
    if (radius > 0) {
      canvas_.save();
      Path path = new Path();
      path.addRoundRect(dl, dt, dr, db, radius, radius, Path.Direction.CW);
      canvas_.clipPath(path);
    }
    drawRunDelegate(canvas_, mResourceManager.getRunDelegate(id),
                    new Rect((int)dl, (int)dt, (int)dr, (int)db));
    if (radius > 0) {
      canvas_.restore();
    }
  }

  Path mPath;
  Path createPath() {
    if (mPath == null) {
      mPath = new Path();
    } else {
      mPath.rewind();
    }
    return mPath;
  }

  protected Paint readPaint(BBufferInputStream stream, Paint painter)
      throws IOException {
    painter.setAntiAlias(true);
    painter.setShader(null);
    painter.setStrokeWidth(stream.readFloat());
    color_ = stream.readInt();
    stroke_color_ = stream.readInt();
    text_size_ = stream.readFloat();
    painter.setTextSize(text_size_);
    int flag = stream.readByte();
    is_bold_ = (flag & (1 << 2)) > 0;
    is_italic_ = (flag & (1 << 3)) > 0;
    is_underline_ = (flag & (1 << 4)) > 0;
    return painter;
  }

  public void drawRunDelegate(Canvas canvas, Drawable delegate, Rect rect) {
    delegate.setBounds(rect);
    delegate.draw(canvas);
  }

  public void drawRunDelegateOnPath(Canvas canvas, Drawable delegate, Path path,
                                    Paint paint) {
    canvas.clipPath(path);
    delegate.draw(canvas);
  }
}
