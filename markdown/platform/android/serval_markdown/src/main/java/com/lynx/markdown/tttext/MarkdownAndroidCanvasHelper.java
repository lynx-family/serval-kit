// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown.tttext;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import com.lynx.textra.BBufferInputStream;
import com.lynx.textra.JavaCanvasHelper;
import com.lynx.textra.TTText;
import com.lynx.textra.TTTextDefinition;
import com.lynx.textra.TTTextUtils;
import java.io.IOException;
import java.util.ArrayList;

public class MarkdownAndroidCanvasHelper extends JavaCanvasHelper {
  protected static final int CANVAS_OP_EXTEND = -1;
  protected static final int CANVAS_OP_CLIP_PATH = 0;
  protected static final int CANVAS_OP_DRAW_PATH = 1;
  protected static final int CANVAS_OP_DRAW_DELEGATE_ON_PATH = 2;
  protected static final int PATH_TYPE_ARC = 0;
  protected static final int PATH_TYPE_OVAL = 1;
  protected static final int PATH_TYPE_RECT = 2;
  protected static final int PATH_TYPE_ROUND_RECT = 3;
  protected static final int PATH_TYPE_MOVE_TO = 4;
  protected static final int PATH_TYPE_LINE_TO = 5;
  protected static final int PATH_TYPE_CUBIC_TO = 6;
  protected static final int PATH_TYPE_QUAD_TO = 7;

  protected JavaResourceManager resource_manager_;
  protected ArrayList<PointF> mTranslateStack = new ArrayList<>();
  protected float mTranslateX;
  protected float mTranslateY;

  IDrawerCallback drawer_callback_;

  public MarkdownAndroidCanvasHelper(Canvas canvas, JavaResourceManager manager,
                                     IDrawerCallback drawer) {
    super(TTText.mFontManager);
    canvas_ = canvas;
    resource_manager_ = manager;
    drawer_callback_ = drawer;
  }

  public void setCanvas(Canvas canvas) { canvas_ = canvas; }

  public void drawBuffer(byte[] input) {
    BBufferInputStream inputStream = new BBufferInputStream(input);
    try {
      while (inputStream.available() > 0) {
        byte op_value = inputStream.readByte();
        if (op_value == CANVAS_OP_EXTEND) {
          drawExtendOp(inputStream);
        } else {
          TTTextDefinition.CanvasOp op = TTTextDefinition.GetCanvasOp(op_value);
          drawOp(op, inputStream);
        }
      }
    } catch (IOException e) {
      e.printStackTrace();
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
    float x = TTTextUtils.Dp2Px(stream.readFloat());
    float y = TTTextUtils.Dp2Px(stream.readFloat());
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
    canvas_.drawPath(path, p);
  }

  protected void drawDelegateOnPath(BBufferInputStream stream)
      throws IOException {
    int id = stream.readInt();
    Path path = readPath(stream);
    Paint p = readPaint(stream, paint_);
    if (drawer_callback_ == null) {
      return;
    }
    p.setColor(color_);
    if (color_ != 0 && stroke_color_ == color_) {
      p.setStyle(Paint.Style.FILL_AND_STROKE);
      drawRunDelegateOnPath(canvas_, resource_manager_.getRunDelegate(id), path,
                            p);
    } else {
      if (color_ != 0) {
        p.setStyle(Paint.Style.FILL);
        drawRunDelegateOnPath(canvas_, resource_manager_.getRunDelegate(id),
                              path, p);
      }
      if (stroke_color_ != 0) {
        p.setStyle(Paint.Style.STROKE);
        p.setColor(stroke_color_);
        drawRunDelegateOnPath(canvas_, resource_manager_.getRunDelegate(id),
                              path, p);
      }
    }
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

  protected void addArc(Path path, BBufferInputStream stream) {
    float cx = TTTextUtils.Dp2Px(stream.readFloat());
    float cy = TTTextUtils.Dp2Px(stream.readFloat());
    float r = TTTextUtils.Dp2Px(stream.readFloat());
    float start = stream.readFloat();
    float end = stream.readFloat();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      path.addArc(cx - r, cy - r, cx + r, cy + r, start, end - start);
    }
  }
  protected void addOval(Path path, BBufferInputStream stream) {
    float left = TTTextUtils.Dp2Px(stream.readFloat());
    float top = TTTextUtils.Dp2Px(stream.readFloat());
    float right = TTTextUtils.Dp2Px(stream.readFloat());
    float bottom = TTTextUtils.Dp2Px(stream.readFloat());
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      path.addOval(left, top, right, bottom, Path.Direction.CW);
    }
  }
  protected void addRect(Path path, BBufferInputStream stream) {
    float left = TTTextUtils.Dp2Px(stream.readFloat());
    float top = TTTextUtils.Dp2Px(stream.readFloat());
    float right = TTTextUtils.Dp2Px(stream.readFloat());
    float bottom = TTTextUtils.Dp2Px(stream.readFloat());
    path.addRect(left, top, right, bottom, Path.Direction.CW);
  }
  protected void addRoundRect(Path path, BBufferInputStream stream) {
    float left = TTTextUtils.Dp2Px(stream.readFloat());
    float top = TTTextUtils.Dp2Px(stream.readFloat());
    float right = TTTextUtils.Dp2Px(stream.readFloat());
    float bottom = TTTextUtils.Dp2Px(stream.readFloat());
    float radiusX = TTTextUtils.Dp2Px(stream.readFloat());
    float radiusY = TTTextUtils.Dp2Px(stream.readFloat());
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      path.addRoundRect(left, top, right, bottom, radiusX, radiusY,
                        Path.Direction.CW);
    }
  }
  protected void moveTo(Path path, BBufferInputStream stream) {
    float x = TTTextUtils.Dp2Px(stream.readFloat());
    float y = TTTextUtils.Dp2Px(stream.readFloat());
    path.moveTo(x, y);
  }
  protected void lineTo(Path path, BBufferInputStream stream) {
    float x = TTTextUtils.Dp2Px(stream.readFloat());
    float y = TTTextUtils.Dp2Px(stream.readFloat());
    path.lineTo(x, y);
  }
  protected void cubicTo(Path path, BBufferInputStream stream) {
    float c1x = TTTextUtils.Dp2Px(stream.readFloat());
    float c1y = TTTextUtils.Dp2Px(stream.readFloat());
    float c2x = TTTextUtils.Dp2Px(stream.readFloat());
    float c2y = TTTextUtils.Dp2Px(stream.readFloat());
    float x = TTTextUtils.Dp2Px(stream.readFloat());
    float y = TTTextUtils.Dp2Px(stream.readFloat());
    path.cubicTo(c1x, c1y, c2x, c2y, x, y);
  }
  protected void quadTo(Path path, BBufferInputStream stream) {
    float cx = TTTextUtils.Dp2Px(stream.readFloat());
    float cy = TTTextUtils.Dp2Px(stream.readFloat());
    float x = TTTextUtils.Dp2Px(stream.readFloat());
    float y = TTTextUtils.Dp2Px(stream.readFloat());
    path.quadTo(cx, cy, x, y);
  }

  @Override
  protected void drawRunDelegate(BBufferInputStream stream) throws IOException {
    int id = stream.readInt();
    float dl = TTTextUtils.Dp2Px(stream.readFloat());
    float dt = TTTextUtils.Dp2Px(stream.readFloat());
    float dr = TTTextUtils.Dp2Px(stream.readFloat());
    float db = TTTextUtils.Dp2Px(stream.readFloat());
    float radius = TTTextUtils.Dp2Px(stream.readFloat());
    if (drawer_callback_ == null)
      return;
    if (radius > 0) {
      canvas_.save();
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        Path path = new Path();
        path.addRoundRect(dl, dt, dr, db, radius, radius, Path.Direction.CW);
        canvas_.clipPath(path);
      }
    }
    drawRunDelegate(canvas_, resource_manager_.getRunDelegate(id),
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
    painter.setStrokeWidth(TTTextUtils.Dp2Px(stream.readFloat()));
    color_ = stream.readInt();
    stroke_color_ = stream.readInt();
    text_size_ = TTTextUtils.Dp2Px(stream.readFloat());
    painter.setTextSize(text_size_);
    int flag = stream.readByte();
    is_bold_ = (flag & (1 << 2)) > 0;
    is_italic_ = (flag & (1 << 3)) > 0;
    is_underline_ = (flag & (1 << 4)) > 0;
    return painter;
  }

  public void drawRunDelegate(Canvas canvas, IRunDelegate delegate, Rect rect) {
  }

  public void drawRunDelegateOnPath(Canvas canvas, IRunDelegate delegate,
                                    Path path, Paint paint) {}
}
