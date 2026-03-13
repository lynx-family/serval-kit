// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg;

import android.graphics.Color;
import android.graphics.Path;
import android.os.Build;
import android.text.BoringLayout;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.SpannedString;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.util.Log;
import com.lynx.serval.svg.model.FillPaintModel;
import com.lynx.serval.svg.model.LinearGradientModel;
import com.lynx.serval.svg.model.RadialGradientModel;
import com.lynx.serval.svg.model.StopModel;
import com.lynx.serval.svg.model.StrokePaintModel;
import com.lynx.serval.svg.utils.PathUtils;

public class SVGRenderEngine {
  public static final String TAG = "SrSVGRenderEngine";
  public static final String SVG = "svg";
  public static final String RECT = "rect";
  public static final String LINE = "line";
  public static final String CIRCLE = "circle";
  public static final String PATH = "path";
  public static final String POLYGON = "polygon";
  public static final String POLYLINE = "polyline";
  public static final String ELLIPSE = "ellipse";
  public static final String IMAGE = "image";
  public static final String LINEAR_GRADIENT = "linearGradient";
  public static final String RADIAL_GRADIENT = "radialGradient";
  public static final String TRANSLATE = "translate";
  public static final String CLIP_PATH = "clipPath";
  public static final String SAVE = "save";
  public static final String RESTORE = "restore";
  public static final String DRAW = "draw";
  private static final String LYNX_SVG_THREAD = "lynx_svg_thread";
  private static final String LIB_SERVAL_SVG = "serval_svg";
  private static final int PATH_OP_DIFFERENCE = 0;
  private static final int PATH_OP_INTERSECT = 1;
  private static final int PATH_OP_UNION = 2;
  private static final int PATH_OP_XOR = 3;
  private static final int PATH_OP_REVERSE_DIFFERENCE = 4;
  public static final int JNI_OK = 0;
  private static volatile boolean sIsNativeLibraryLoaded = false;
  private static SVGRenderEngine mInstance;

  public static SVGRenderEngine getInstance() {
    if (mInstance == null) {
      synchronized (SVGRenderEngine.class) {
        if (mInstance == null) {
          mInstance = new SVGRenderEngine();
        }
      }
    }
    return mInstance;
  }

  private SVGRenderEngine() { loadNativeLibrary(); }

  private void loadNativeLibrary() {
    if (sIsNativeLibraryLoaded) {
      return;
    }
    try {
      System.loadLibrary(LIB_SERVAL_SVG);
    } catch (UnsatisfiedLinkError error) {
      Log.e(TAG, "Loading native library fail: " + error.getMessage());
      return;
    }
    Log.i(TAG, "Loading native library successfully");
    sIsNativeLibraryLoaded = true;
  }

  native int render(SVGRender svgRender, String content, float left, float top,
                    float width, float height);

  native float[] calculateViewBoxTransform(float vpLeft, float vpTop,
                                           float vpWidth, float vpHeight,
                                           float vbLeft, float vbTop,
                                           float vbWidth, float vbHeight,
                                           int alignX, int alignY, int scale);

  public static FillPaintModel makeFillPaintModel(int type, String iri,
                                                  long color, int rule,
                                                  float opacity) {
    return new FillPaintModel(type, iri, color, opacity, rule);
  }

  public static StrokePaintModel makeStrokePaintModel(
      int type, String iri, long color, float width, float opacity,
      int strokeLineCap, int strokeLineJoin, float strokeMiterLimit,
      float strokeDashOffset, float[] strokeDashArray) {
    StrokePaintModel strokePaintModel = new StrokePaintModel(
        type, iri, color, width, opacity, strokeLineCap, strokeLineJoin,
        strokeMiterLimit, strokeDashOffset, strokeDashArray);
    return strokePaintModel;
  }

  public static SpannableStringBuilder makeStringBuilder() {
    return new SpannableStringBuilder();
  }

  public static void appendSpan(SpannableStringBuilder builder, String str,
                                int color, float textSize) {
    if (TextUtils.isEmpty(str)) {
      return;
    }
    int len = str.length();

    SpannableString spannableString = new SpannableString(str);
    // set color
    spannableString.setSpan(new ForegroundColorSpan(color), 0, len,
                            Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    // set bold
    //TODO(@hujing.1) set typeFace
    //   spannableString.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), 0, len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    //set size
    spannableString.setSpan(new AbsoluteSizeSpan((int)textSize, true), 0, len,
                            Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

    builder.append(spannableString);
  }

  public static Path makeMutablePath() { return new Path(); }

  public static Path makeRectPath(float x, float y, float rx, float ry,
                                  float width, float height) {
    return PathUtils.makeRectPath(x, y, rx, ry, width, height, x + width,
                                  y + height);
  }

  public static Path makeCirclePath(float cx, float cy, float r) {
    return PathUtils.makeCirclePath(cx, cy, r);
  }

  public static Path makeLinePath(float startX, float startY, float endX,
                                  float endY) {
    return PathUtils.makeLinePath(startX, startY, endX, endY);
  }

  public static Path makeEllipsePath(float centerX, float centerY,
                                     float radiusX, float radiusY) {
    return PathUtils.makeEllipsePath(centerX, centerY, radiusX, radiusY);
  }

  public static Path makePolygonPath(float[] points) {
    return PathUtils.makePolygonPath(points, true);
  }

  public static Path makePath(byte[] ops, float[] values) {
    return PathUtils.makePath(ops, values);
  }

  public static Path makePolyLinePath(float[] points) {
    return PathUtils.makePolygonPath(points, false);
  }

  public static void op(Path path1, Path path2, int type) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      Path.Op op = Path.Op.UNION;
      switch (type) {
        case PATH_OP_DIFFERENCE:
          op = Path.Op.DIFFERENCE;
          break;
        case PATH_OP_INTERSECT:
          op = Path.Op.INTERSECT;
          break;
        case PATH_OP_UNION:
          op = Path.Op.UNION;
          break;
        case PATH_OP_XOR:
          op = Path.Op.XOR;
          break;
        case PATH_OP_REVERSE_DIFFERENCE:
          op = Path.Op.REVERSE_DIFFERENCE;
          break;
        default:
          break;
      }
      path1.op(path2, op);
    }
  }

  public static StopModel makeStopModel(float offset, long color,
                                        float opacity) {
    return new StopModel(offset, color, opacity);
  }

  public static void makeLinearGradient(SVGRender render, String id,
                                        float[] transform, int spreadModel,
                                        float x1, float x2, float y1, float y2,
                                        int gradientType, StopModel[] models) {
    if (render != null) {
      LinearGradientModel linearGradientModel = new LinearGradientModel();
      linearGradientModel.mType = gradientType;
      linearGradientModel.mTransform = transform;
      linearGradientModel.mStopModels = models;
      linearGradientModel.mX1 = x1;
      linearGradientModel.mX2 = x2;
      linearGradientModel.mY1 = y1;
      linearGradientModel.mY2 = y2;
      linearGradientModel.mSpreadMode = spreadModel;
      render.addGradientModel(id, LINEAR_GRADIENT, linearGradientModel);
    }
  }

  public static void makeRadialGradient(SVGRender render, String id,
                                        float[] transForm, int spreadModel,
                                        float cx, float cy, float fr, float fx,
                                        float fy, int gradientType,
                                        StopModel[] models) {
    if (render != null) {
      RadialGradientModel gradientModel = new RadialGradientModel();
      gradientModel.mType = gradientType;
      gradientModel.mTransform = transForm;
      gradientModel.mStopModels = models;
      gradientModel.mSpreadMode = spreadModel;
      gradientModel.mCx = cx;
      gradientModel.mCy = cy;
      gradientModel.mFr = fr;
      gradientModel.mFx = fx;
      gradientModel.mFy = fy;
      render.addGradientModel(id, RADIAL_GRADIENT, gradientModel);
    }
  }

  public static void layout(float max_width) {}
}
