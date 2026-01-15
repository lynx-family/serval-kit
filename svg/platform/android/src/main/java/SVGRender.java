// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg;

import static com.lynx.serval.svg.model.GradientModel.GRADIENT_TYPE_OBJECT_BOUNDING_BOX;
import static com.lynx.serval.svg.model.PaintRef.PAINT_COLOR;
import static com.lynx.serval.svg.model.PaintRef.PAINT_IRI;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Picture;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.os.Build;
import android.text.BoringLayout;
import android.text.Layout;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.SpannedString;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import androidx.core.util.Pair;
import com.lynx.serval.svg.model.FillPaintModel;
import com.lynx.serval.svg.model.GradientModel;
import com.lynx.serval.svg.model.LinearGradientModel;
import com.lynx.serval.svg.model.PaintRef;
import com.lynx.serval.svg.model.RadialGradientModel;
import com.lynx.serval.svg.model.StopModel;
import com.lynx.serval.svg.model.StrokePaintModel;
import java.util.HashMap;

public class SVGRender {
  public interface BitmapRequestCallBack {
    public void onSuccess(Bitmap bitmap);

    public void onFailed();
  }

  @WorkerThread
  public interface ResourceManager {
    public void requestBitMap(String url, BitmapRequestCallBack callBack);
  }

  private static final String TAG = "SrSVGRender";
  private static final int SCALE_NONE = 0;
  private static final int SCALE_MEET = 1;
  private static final int SCALE_SLICE = 2;
  private static final int ALIGNMENT_NONE = 0;
  private static final int ALIGNMENT_MIN = 1;
  private static final int ALIGNMENT_MID = 2;
  private static final int ALIGNMENT_MAX = 3;
  private Canvas mPictureCanvas;
  private HashMap<String, Pair<String, GradientModel>> mGradientModels;
  private SVGRenderEngine mSVGRenderEngineNG;
  private ResourceManager mResourceProvider;

  public SVGRender() {
    mSVGRenderEngineNG = SVGRenderEngine.getInstance();
    mGradientModels = new HashMap<>();
  }

  public void setResourceManager(ResourceManager resourceManager) {
    mResourceProvider = resourceManager;
  }

  public Picture renderPicture(String content, Rect viewPort) {
    Picture picture = new Picture();
    mPictureCanvas =
        picture.beginRecording(viewPort.width(), viewPort.height());
    if (mSVGRenderEngineNG != null) {
      mSVGRenderEngineNG.render(this, content, viewPort.left, viewPort.top,
                                viewPort.width(), viewPort.height());
    }
    picture.endRecording();
    return picture;
  }

  public void setViewBox(float x, float y, float width, float height) {
    Log.i(TAG, "setViewBox: " + x + ", " + y + ", " + width + ", " + height);
  }

  public void save() {
    if (mPictureCanvas != null) {
      mPictureCanvas.save();
    }
  }

  public void restore() {
    if (mPictureCanvas != null) {
      mPictureCanvas.restore();
    }
  }

  public void translate(float x, float y) {
    if (mPictureCanvas != null) {
      mPictureCanvas.translate(x, y);
    }
  }

  public void transform(float[] form) {
    if (mPictureCanvas != null && form != null && form.length == 6) {
      float[] formValue = new float[] {
          form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f};
      Matrix matrix = new Matrix();
      matrix.setValues(formValue);
      mPictureCanvas.concat(matrix);
    }
  }

  public void draw(Path path, FillPaintModel fillPaintModel,
                   StrokePaintModel strokePaintModel) {
    if (mPictureCanvas != null && path != null) {
      drawPathWithFillModel(mPictureCanvas, path, fillPaintModel);
      drawPathWithStokeModel(mPictureCanvas, path, strokePaintModel);
    }
  }

  public void drawText(SpannableStringBuilder builder, int textAnchor, float x,
                       float y) {
    TextPaint textPaint = new TextPaint();
    textPaint.setAntiAlias(true);
    // create BoringLayout/staticLayout
    BoringLayout.Metrics boringMetrics =
        BoringLayout.isBoring(builder, textPaint);
    if (boringMetrics == null) {
      StaticLayout staticLayout =
          new StaticLayout(builder, textPaint, Integer.MAX_VALUE,
                           Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false);
      int width = staticLayout.getWidth();
      if (textAnchor == 1) {  // middle
        x = x - width / 2;
      } else if (textAnchor == 2) {  // bottom
        x = x - width;
      }
      if (staticLayout.getLineCount() > 0) {
        mPictureCanvas.translate(x, y + (staticLayout.getLineAscent(0) -
                                         staticLayout.getLineDescent(0)) /
                                            2);
      }
      staticLayout.draw(mPictureCanvas);
    } else {
      //TODO(@hujing.1) alignment
      BoringLayout boringLayout = BoringLayout.make(
          builder, textPaint, Integer.MAX_VALUE, Layout.Alignment.ALIGN_NORMAL,
          1.0f, 0.0f, boringMetrics, false);

      if (textAnchor == 1) {  // middle
        x = x - boringMetrics.width / 2;
      } else if (textAnchor == 2) {  // bottom
        x = x - boringMetrics.width;
      }
      mPictureCanvas.translate(
          x, y + (boringMetrics.ascent - boringMetrics.descent) / 2);
      boringLayout.draw(mPictureCanvas);
    }
  }

  public void drawImage(final String url, final float x, final float y,
                        final float width, final float height, final int alignX,
                        final int alignY, final int scale) {
    if (TextUtils.isEmpty(url) || mResourceProvider == null) {
      return;
    }
    mResourceProvider.requestBitMap(url, new BitmapRequestCallBack() {
      @Override
      public void onSuccess(Bitmap bitmap) {
        // drawImage
        if (bitmap == null) {
          // TODO log
          return;
        }
        float bitmapWidth = bitmap.getWidth();
        float bitmapHeight = bitmap.getHeight();
        if (width > 0 && height > 0 && bitmapWidth > 0 && bitmapHeight > 0 &&
            mSVGRenderEngineNG != null) {
          // Local transform from image's raw dimension to the specified <image> dimension.
          float form[] = mSVGRenderEngineNG.calculateViewBoxTransform(
              x, y, width, height, 0, 0, bitmapWidth, bitmapHeight, alignX,
              alignY, scale);
          if (form != null && form.length == 6 && mPictureCanvas != null) {
            float[] formValue =
                new float[] {form[0], form[2], form[4], form[1], form[3],
                             form[5], 0.f,     0.f,     1.f};
            Matrix matrix = new Matrix();
            matrix.setValues(formValue);
            mPictureCanvas.concat(matrix);
            Paint bitmapPaint = new Paint(Paint.FILTER_BITMAP_FLAG);
            mPictureCanvas.drawBitmap(bitmap, 0, 0, bitmapPaint);
          }
        }
      }

      @Override
      public void onFailed() {
        // TODO log
      }
    });
  }

  public void clipPath(Path path, int clipRule) {
    if (mPictureCanvas != null && path != null) {
      if (clipRule == 1) {
        path.setFillType(Path.FillType.EVEN_ODD);
      } else {
        path.setFillType(Path.FillType.WINDING);
      }
      mPictureCanvas.clipPath(path);
    }
  }

  private void drawPathWithFillModel(@NonNull Canvas canvas, @NonNull Path path,
                                     FillPaintModel fillPaintModel) {
    if (fillPaintModel != null) {
      // Note: If fill paint type is PAINT_NONE, do nothing.
      if (fillPaintModel.mFillRule == 1) {
        path.setFillType(Path.FillType.EVEN_ODD);
      } else {
        path.setFillType(Path.FillType.WINDING);
      }

      if (fillPaintModel.mType == PAINT_IRI) {
        drawPathWithPaintRef(canvas, path, fillPaintModel);
      } else if (fillPaintModel.mType == PAINT_COLOR) {
        Paint paint = initFillPaint(fillPaintModel);
        canvas.drawPath(path, paint);
      }
    } else {
      // Note: If fillPaintModel is null which means that the fill label is not set, we need to init
      // default paint and draw.
      Paint paint = initFillPaint();
      canvas.drawPath(path, paint);
    }
  }

  private void drawPathWithStokeModel(Canvas canvas, Path path,
                                      StrokePaintModel strokePaintModel) {
    if (strokePaintModel != null) {
      if (strokePaintModel.mType == PAINT_IRI) {
        drawPathWithPaintRef(canvas, path, strokePaintModel);
      } else if (strokePaintModel.mType == PAINT_COLOR) {
        Paint paint = initStrokePaint(strokePaintModel);
        canvas.drawPath(path, paint);
      }
    }
  }

  private void drawPathWithPaintRef(@NonNull Canvas canvas, @NonNull Path path,
                                    @NonNull PaintRef paintRef) {
    if (paintRef.mType == PAINT_IRI && !TextUtils.isEmpty(paintRef.mIri)) {
      Paint paint = null;
      Pair<String, GradientModel> pair = mGradientModels.get(paintRef.mIri);
      if (pair != null && pair.second instanceof LinearGradientModel) {
        LinearGradientModel lgModel = (LinearGradientModel)pair.second;
        paint = initLinearGradientPaint(canvas, lgModel, path, paintRef);
      } else if (pair != null && pair.second instanceof RadialGradientModel) {
        RadialGradientModel rgModel = (RadialGradientModel)pair.second;
        paint = initRadialGradientPaint(canvas, rgModel, path, paintRef);
      }
      if (paint != null) {
        canvas.drawPath(path, paint);
      }
    }
  }

  private Paint initRadialGradientPaint(Canvas canvas,
                                        @NonNull RadialGradientModel rgModel,
                                        @NonNull Path path,
                                        @NonNull PaintRef paintRef) {
    int stopSize = rgModel.mStopModels.length;
    if (stopSize == 0) {
      // If there are no stops defined, we are to treat it as paint = 'none'
      return null;
    }
    Paint paint = null;
    if (paintRef instanceof FillPaintModel) {
      paint = initFillPaint((FillPaintModel)paintRef);
    } else if (paintRef instanceof StrokePaintModel) {
      paint = initStrokePaint((StrokePaintModel)paintRef);
    }
    paint.setAlpha(clampOpacity(paintRef.mOpacity));
    float[] positions = new float[stopSize];
    int[] colors = new int[stopSize];
    float lastOffset = -1.f;
    for (int i = 0; i < stopSize; ++i) {
      // prepare position
      StopModel stopModel = rgModel.mStopModels[i];
      float offset = stopModel.mOffset;
      if (i == 0 || offset >= lastOffset) {
        positions[i] = offset;
        lastOffset = offset;
      } else {
        // Each offset must be equal or greater than the last one.
        // If it doesn't we need to replace it with the previous value.
        positions[i] = lastOffset;
      }
      // prepare color
      colors[i] = getColorWithOpacity(stopModel.mColor, stopModel.mStopOpacity);
    }
    RectF boundingBox = calculatePathBounds(path);
    float r = rgModel.mFr;
    float cx = rgModel.mCx;
    float cy = rgModel.mCy;
    if (rgModel.mType == GRADIENT_TYPE_OBJECT_BOUNDING_BOX) {
      float width = boundingBox.width();
      float height = boundingBox.height();
      float maxSize = Math.max(width, height);
      r = r * maxSize;
      cx = boundingBox.left + cx * maxSize;
      cy = boundingBox.top + cy * maxSize;
    }
    // If gradient vector is zero length, we instead fill with last stop color
    if (r == 0.f || stopSize == 1) {
      paint.setColor(colors[stopSize - 1]);
      return paint;
    }

    // Convert spreadMode to tileMode
    Shader.TileMode tileMode = Shader.TileMode.CLAMP;
    if (rgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REFLECT) {
      tileMode = Shader.TileMode.MIRROR;
    } else if (rgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REPEAT) {
      tileMode = Shader.TileMode.REPEAT;
    }
    // Create shader instance
    // fx and fy are ignored because Android RadialGradient doesn't support a
    // 'focus' point that is different from cx,cy.
    RadialGradient radialGradient =
        new RadialGradient(cx, cy, r, colors, positions, tileMode);
    // Calculate the gradient transform matrix
    float[] form = rgModel.mTransform;
    if (form != null) {
      // support gradient transform
      Matrix gradientTransform = new Matrix();
      float[] formValue = new float[] {
          form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f};
      gradientTransform.setValues(formValue);
      radialGradient.setLocalMatrix(gradientTransform);
    }
    paint.setShader(radialGradient);
    return paint;
  }

  private Paint initLinearGradientPaint(Canvas canvas,
                                        @NonNull LinearGradientModel lgModel,
                                        @NonNull Path path,
                                        @NonNull PaintRef paintRef) {
    int stopSize = lgModel.mStopModels.length;
    if (stopSize == 0) {
      // If there are no stops defined, we are to treat it as paint = 'none'
      return null;
    }
    Paint paint = null;
    if (paintRef instanceof FillPaintModel) {
      paint = initFillPaint((FillPaintModel)paintRef);
    } else if (paintRef instanceof StrokePaintModel) {
      paint = initStrokePaint((StrokePaintModel)paintRef);
    }
    paint.setAlpha(clampOpacity(paintRef.mOpacity));
    float[] positions = new float[stopSize];
    int[] colors = new int[stopSize];
    float lastOffset = -1.f;
    for (int i = 0; i < stopSize; ++i) {
      // prepare position
      StopModel stopModel = lgModel.mStopModels[i];
      float offset = stopModel.mOffset;
      if (i == 0 || offset >= lastOffset) {
        positions[i] = offset;
        lastOffset = offset;
      } else {
        // Each offset must be equal or greater than the last one.
        // If it doesn't we need to replace it with the previous value.
        positions[i] = lastOffset;
      }
      // prepare color
      colors[i] = getColorWithOpacity(stopModel.mColor, stopModel.mStopOpacity);
    }
    float x1 = lgModel.mX1;
    float y1 = lgModel.mY1;
    float x2 = lgModel.mX2;
    float y2 = lgModel.mY2;
    RectF boundingBox = calculatePathBounds(path);
    if (lgModel.mType == GRADIENT_TYPE_OBJECT_BOUNDING_BOX) {
      float minX = boundingBox.left;
      float minY = boundingBox.top;
      float width = boundingBox.width();
      float height = boundingBox.height();
      x1 = minX + x1 * width;
      y1 = minY + y1 * height;
      x2 = minX + x2 * width;
      y2 = minY + y2 * height;
    }
    // If gradient vector is zero length, we instead fill with last stop color
    if ((x1 == x2 && y1 == y2) || stopSize == 1) {
      paint.setColor(colors[stopSize - 1]);
      return paint;
    }
    // Convert spreadMode to tileMode
    Shader.TileMode tileMode = Shader.TileMode.CLAMP;
    if (lgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REFLECT) {
      tileMode = Shader.TileMode.MIRROR;
    } else if (lgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REPEAT) {
      tileMode = Shader.TileMode.REPEAT;
    }
    // Create shader instance
    LinearGradient linearGradient =
        new LinearGradient(x1, y1, x2, y2, colors, positions, tileMode);

    float[] form = lgModel.mTransform;
    if (form != null) {
      //support gradient transform
      Matrix matrix = new Matrix();
      matrix.preTranslate(boundingBox.left, boundingBox.top);
      matrix.preScale(boundingBox.width(), boundingBox.height());

      float[] formValue = new float[] {
          form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f};
      matrix.setValues(formValue);
      linearGradient.setLocalMatrix(matrix);
    }

    paint.setShader(linearGradient);
    return paint;
  }

  private Paint initFillPaint(FillPaintModel fillPaintModel) {
    Paint fillPaint = initFillPaint();
    if (fillPaintModel != null) {
      fillPaint.setColor(
          getColorWithOpacity(fillPaintModel.mColor, fillPaintModel.mOpacity));
    }
    return fillPaint;
  }

  private Paint initFillPaint() {
    Paint fillPaint = new Paint();
    fillPaint.setStyle(Paint.Style.FILL);
    fillPaint.setTypeface(Typeface.DEFAULT);
    fillPaint.setFlags(Paint.ANTI_ALIAS_FLAG | Paint.LINEAR_TEXT_FLAG |
                       Paint.SUBPIXEL_TEXT_FLAG);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
      fillPaint.setHinting(Paint.HINTING_OFF);
    }
    return fillPaint;
  }

  private Paint initStrokePaint(StrokePaintModel strokePaintModel) {
    Paint strokePaint = initStrokePaint();
    if (strokePaintModel != null) {
      strokePaint.setColor(getColorWithOpacity(strokePaintModel.mColor,
                                               strokePaintModel.mOpacity));
      strokePaint.setStrokeWidth(strokePaintModel.mWith);
      initStrokeExtraInfo(strokePaint, strokePaintModel);
    }
    return strokePaint;
  }

  private Paint initStrokePaint() {
    Paint strokePaint = new Paint();
    strokePaint.setStyle(Paint.Style.STROKE);
    strokePaint.setTypeface(Typeface.DEFAULT);
    strokePaint.setFlags(Paint.ANTI_ALIAS_FLAG | Paint.LINEAR_TEXT_FLAG |
                         Paint.SUBPIXEL_TEXT_FLAG);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
      strokePaint.setHinting(Paint.HINTING_OFF);
    }
    return strokePaint;
  }

  private void initStrokeExtraInfo(@NonNull Paint paint,
                                   @NonNull StrokePaintModel strokePaintModel) {
    // strokeLineCap
    int strokeLineCap = strokePaintModel.mStrokeLineCap;
    switch (strokeLineCap) {
      case 0:
        paint.setStrokeCap(Paint.Cap.BUTT);
        break;
      case 1:
        paint.setStrokeCap(Paint.Cap.ROUND);
        break;
      case 2:
        paint.setStrokeCap(Paint.Cap.SQUARE);
        break;
      default:
        break;
    }
    // strokeLineJoin
    int strokeJoin = strokePaintModel.mStrokeLineJoin;
    switch (strokeJoin) {
      case 0:
        paint.setStrokeJoin(Paint.Join.MITER);
        break;
      case 1:
        paint.setStrokeJoin(Paint.Join.ROUND);
        break;
      case 2:
        paint.setStrokeJoin(Paint.Join.BEVEL);
        break;
      default:
        break;
    }
    // strokeMiterLimit
    paint.setStrokeMiter(strokePaintModel.mStrokeMiterLimit);

    // strokeDashArray & strokeOffset
    if (strokePaintModel.mStrokeDashArray == null ||
        strokePaintModel.mStrokeDashArray.length == 0) {
      paint.setPathEffect(null);
    } else {
      float intervalSum = 0f;
      float[] dashArray = strokePaintModel.mStrokeDashArray;
      int n = dashArray.length;
      int arrayLen = (n % 2 == 0) ? n : n * 2;
      float[] intervals = new float[arrayLen];
      for (int i = 0; i < arrayLen; i++) {
        intervals[i] = dashArray[i % n];
        intervalSum += intervals[i];
      }
      if (intervalSum == 0) {
        paint.setPathEffect(null);
      } else {
        float offset = strokePaintModel.mStrokeDashOffset;
        if (offset < 0) {
          offset = intervalSum + (offset % intervalSum);
        }
        paint.setPathEffect(new DashPathEffect(intervals, offset));
      }
    }
  }

  public void addGradientModel(String id, String type, GradientModel model) {
    mGradientModels.put("#" + id, new Pair<>(type, model));
  }

  // Convert a float in range 0..1 to an int in range 0..255.
  public static int clampOpacity(float opacity) {
    int i = (int)(opacity * 256f);
    return Math.max(0, Math.min(i, 255));
  }

  public static int getColorWithOpacity(long color, float opacity) {
    int alpha = (int)((color >> 24) & 0xff);
    alpha = Math.round(alpha * opacity);
    alpha = (alpha < 0) ? 0 : Math.min(alpha, 255);
    return (alpha << 24) | ((int)(color & 0xffffff));
  }

  private RectF calculatePathBounds(Path path) {
    RectF pathBounds = new RectF();
    path.computeBounds(pathBounds, true);
    return pathBounds;
  }

  public static float[] calculatePathBoundsArray(Path path) {
    RectF pathBounds = new RectF();
    path.computeBounds(pathBounds, true);
    float[] array = {pathBounds.left, pathBounds.top, pathBounds.width(),
                     pathBounds.height()};
    return array;
  }

  public static void applyTransform(Path path, float[] form) {
    if (path != null) {
      float[] formValue = new float[] {
          form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f};
      Matrix matrix = new Matrix();
      matrix.setValues(formValue);
      path.transform(matrix);
    }
  }
}
