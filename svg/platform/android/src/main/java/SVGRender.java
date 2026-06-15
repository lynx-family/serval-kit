// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg;

import static com.lynx.serval.svg.model.GradientModel.GRADIENT_TYPE_OBJECT_BOUNDING_BOX;
import static com.lynx.serval.svg.model.PaintRef.PAINT_COLOR;
import static com.lynx.serval.svg.model.PaintRef.PAINT_IRI;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.DashPathEffect;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Picture;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
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
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.core.util.Pair;
import com.lynx.serval.svg.model.FillPaintModel;
import com.lynx.serval.svg.model.GradientModel;
import com.lynx.serval.svg.model.LinearGradientModel;
import com.lynx.serval.svg.model.PaintRef;
import com.lynx.serval.svg.model.RadialGradientModel;
import com.lynx.serval.svg.model.StopModel;
import com.lynx.serval.svg.model.StrokePaintModel;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

public class SVGRender {
  private static final String TAG = "SVGRender";
  private static final int FILTER_OP_BLUR = 0;
  private static final int FILTER_OP_OFFSET = 1;
  private static final int FILTER_OP_COLOR_MATRIX = 2;
  private static final int FILTER_OP_LUMINANCE_TO_ALPHA = 3;
  private static final int DIAGNOSTIC_NATIVE_LIBRARY_LOAD_FAILED = 6;
  private static final float[] LUMINANCE_TO_ALPHA_MATRIX = new float[] {
      0f, 0f, 0f, 0f, 0f, 0f,      0f,      0f,      0f, 0f,
      0f, 0f, 0f, 0f, 0f, 0.2126f, 0.7152f, 0.0722f, 0f, 0f,
  };

  private static final class FilterLayer {
    final Canvas parentCanvas;
    final Canvas layerCanvas;
    final Bitmap bitmap;
    final Matrix parentMatrix;
    final int[] operations;
    final float[] values;

    FilterLayer(Canvas parentCanvas, Canvas layerCanvas, Bitmap bitmap,
                Matrix parentMatrix, int[] operations, float[] values) {
      this.parentCanvas = parentCanvas;
      this.layerCanvas = layerCanvas;
      this.bitmap = bitmap;
      this.parentMatrix = parentMatrix;
      this.operations = operations == null ? new int[0] : operations.clone();
      this.values = values == null ? new float[0] : values.clone();
    }
  }

  public interface BitmapRequestCallBack {
    public void onSuccess(Bitmap bitmap);

    public void onFailed();
  }

  @WorkerThread
  public interface ResourceManager {
    public void requestBitMap(String url, BitmapRequestCallBack callBack);
  }

  private static final int SCALE_NONE = 0;
  private static final int SCALE_MEET = 1;
  private static final int SCALE_SLICE = 2;
  private static final int ALIGNMENT_NONE = 0;
  private static final int ALIGNMENT_MIN = 1;
  private static final int ALIGNMENT_MID = 2;
  private static final int ALIGNMENT_MAX = 3;
  private Canvas mPictureCanvas;
  private HashMap<String, Pair<String, GradientModel>> mGradientModels;
  @Nullable private SVGRenderEngine mSVGRenderEngineNG;
  private ResourceManager mResourceProvider;
  private String mColorString = null;
  private int mCanvasWidth = 0;
  private int mCanvasHeight = 0;
  private final ArrayDeque<FilterLayer> mFilterLayers = new ArrayDeque<>();

  public static final class SVGDiagnostic {
    public final int code;
    @NonNull public final String message;
    @NonNull public final String subject;
    public final boolean fatal;

    public SVGDiagnostic(int code, @Nullable String message,
                         @Nullable String subject, boolean fatal) {
      this.code = code;
      this.message = message == null ? "" : message;
      this.subject = subject == null ? "" : subject;
      this.fatal = fatal;
    }
  }

  public static final class SVGRenderResult {
    @NonNull public final Picture picture;
    @NonNull public final List<SVGDiagnostic> diagnostics;
    public final boolean hasError;
    @Nullable public final String errorMessage;

    SVGRenderResult(@NonNull Picture picture,
                    @NonNull List<SVGDiagnostic> diagnostics) {
      this.picture = picture;
      this.diagnostics = diagnostics;
      this.hasError = !diagnostics.isEmpty();
      this.errorMessage =
          diagnostics.isEmpty() ? null : diagnostics.get(0).message;
    }
  }

  public SVGRender() {
    mSVGRenderEngineNG = SVGRenderEngine.getInstance();
    mGradientModels = new HashMap<>();
  }

  public void setResourceManager(ResourceManager resourceManager) {
    mResourceProvider = resourceManager;
  }

  public void setColor(@Nullable String color) {
    if (TextUtils.isEmpty(color)) {
      mColorString = null;
      return;
    }
    mColorString = color.trim();
  }

  private static SVGDiagnostic makeNativeLibraryLoadFailedDiagnostic() {
    String nativeLoadError = SVGRenderEngine.getNativeLibraryLoadError();
    String message =
        TextUtils.isEmpty(nativeLoadError)
            ? "SVG native library is not loaded."
            : "SVG native library is not loaded: " + nativeLoadError;
    return new SVGDiagnostic(DIAGNOSTIC_NATIVE_LIBRARY_LOAD_FAILED, message,
                             "serval_svg", true);
  }

  @Nullable
  public String getColor() {
    return mColorString;
  }

  // Prefer renderPictureWithResult() so callers can inspect diagnostics per render.
  @Deprecated
  public Picture renderPicture(String content, Rect viewPort) {
    return renderPictureWithResult(content, viewPort).picture;
  }

  public SVGRenderResult renderPictureWithResult(String content,
                                                 Rect viewPort) {
    Picture picture = new Picture();
    mCanvasWidth = viewPort.width();
    mCanvasHeight = viewPort.height();
    mFilterLayers.clear();
    mDstInLayerActive = false;
    mMaskIsLuminance = false;
    mPictureCanvas =
        picture.beginRecording(viewPort.width(), viewPort.height());
    SVGDiagnostic[] diagnostics = null;
    if (mSVGRenderEngineNG == null) {
      mSVGRenderEngineNG = SVGRenderEngine.getInstance();
    }
    if (mSVGRenderEngineNG != null) {
      diagnostics = mSVGRenderEngineNG.renderWithDiagnostics(
          this, content, viewPort.left, viewPort.top, viewPort.width(),
          viewPort.height(), getColor());
    }
    picture.endRecording();
    List<SVGDiagnostic> diagnosticList;
    if (mSVGRenderEngineNG == null) {
      diagnosticList =
          Collections.singletonList(makeNativeLibraryLoadFailedDiagnostic());
    } else if (diagnostics == null || diagnostics.length == 0) {
      diagnosticList = Collections.<SVGDiagnostic>emptyList();
    } else {
      diagnosticList = Collections.unmodifiableList(Arrays.asList(diagnostics));
    }
    return new SVGRenderResult(picture, diagnosticList);
  }

  public void setViewBox(float x, float y, float width, float height) {}

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
                        final int alignY, final int scale,
                        final float opacity) {
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
            bitmapPaint.setAlpha(clampOpacity(opacity));
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

  public void clipRect(float left, float top, float right, float bottom) {
    if (mPictureCanvas != null) {
      mPictureCanvas.clipRect(left, top, right, bottom);
    }
  }

  public void saveLayer(float left, float top, float right, float bottom) {
    if (mPictureCanvas != null) {
      saveLayerWithPaint(left, top, right, bottom, null);
    }
  }

  public void beginOpacityLayer(float left, float top, float right,
                                float bottom, float opacity) {
    if (mPictureCanvas != null) {
      Paint alphaPaint = new Paint();
      alphaPaint.setAlpha(clampOpacity(opacity));
      saveLayerWithPaint(left, top, right, bottom, alphaPaint);
    }
  }

  private void saveLayerWithPaint(float left, float top, float right,
                                  float bottom, Paint paint) {
    if (mPictureCanvas != null) {
      if (left == 0 && top == 0 && right == 0 && bottom == 0) {
        // Full canvas layer
        mPictureCanvas.saveLayer(null, paint);
      } else {
        RectF bounds = new RectF(left, top, right, bottom);
        mPictureCanvas.saveLayer(bounds, paint);
      }
    }
  }

  public void restoreLayer() {
    if (mPictureCanvas != null) {
      mPictureCanvas.restore();
    }
  }

  public void beginMaskLayer(float left, float top, float right, float bottom,
                             boolean isLuminance) {
    mMaskIsLuminance = isLuminance;
    saveLayer(left, top, right, bottom);
  }

  public void endMaskLayer() {
    if (mPictureCanvas != null) {
      if (mDstInLayerActive) {
        endMaskContentLayer();
      }
      restoreLayer();
      mMaskIsLuminance = false;
    }
  }

  public void beginFilterLayer(float left, float top, float right, float bottom,
                               int[] operations, float[] values) {
    if (mPictureCanvas == null || mCanvasWidth <= 0 || mCanvasHeight <= 0) {
      return;
    }
    Bitmap bitmap = Bitmap.createBitmap(mCanvasWidth, mCanvasHeight,
                                        Bitmap.Config.ARGB_8888);
    Canvas layerCanvas = new Canvas(bitmap);
    Matrix parentMatrix = mPictureCanvas.getMatrix();
    layerCanvas.setMatrix(parentMatrix);
    mFilterLayers.push(new FilterLayer(mPictureCanvas, layerCanvas, bitmap,
                                       new Matrix(parentMatrix), operations,
                                       values));
    mPictureCanvas = layerCanvas;
  }

  public void endFilterLayer() {
    if (mPictureCanvas != null && !mFilterLayers.isEmpty() &&
        mPictureCanvas == mFilterLayers.peek().layerCanvas) {
      restoreFilterLayer();
    }
  }

  private boolean mDstInLayerActive = false;
  private boolean mMaskIsLuminance = false;

  public void beginMaskContentLayer() {
    if (mPictureCanvas != null && !mDstInLayerActive) {
      // Record the full mask into a dedicated layer first, then apply DST_IN
      // once when that layer is restored back to the content layer. For
      // luminance masks, the restore paint converts the fully composed mask
      // image into alpha so black cutouts can punch through earlier white
      // backdrop content.
      Paint xferPaint = createMaskCompositePaint(mMaskIsLuminance);
      mPictureCanvas.saveLayer(null, xferPaint);
      mDstInLayerActive = true;
    }
  }

  public void endMaskContentLayer() {
    if (mPictureCanvas != null && mDstInLayerActive) {
      mPictureCanvas.restore();
      mDstInLayerActive = false;
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
      Paint paint = resolveStrokePaint(canvas, path, strokePaintModel);
      if (paint != null) {
        if (strokePaintModel.mVectorEffect == 1) {
          drawNonScalingStroke(canvas, path, paint);
        } else {
          canvas.drawPath(path, paint);
        }
      }
    }
  }

  private Paint resolveStrokePaint(@NonNull Canvas canvas, @NonNull Path path,
                                   @NonNull StrokePaintModel strokePaintModel) {
    if (strokePaintModel.mType == PAINT_IRI) {
      Pair<String, GradientModel> pair =
          mGradientModels.get(strokePaintModel.mIri);
      if (pair != null && pair.second instanceof LinearGradientModel) {
        return initLinearGradientPaint(canvas, (LinearGradientModel)pair.second,
                                       path, strokePaintModel);
      } else if (pair != null && pair.second instanceof RadialGradientModel) {
        return initRadialGradientPaint(canvas, (RadialGradientModel)pair.second,
                                       path, strokePaintModel);
      }
      return null;
    } else if (strokePaintModel.mType == PAINT_COLOR) {
      return initStrokePaint(strokePaintModel);
    }
    return null;
  }

  private void drawNonScalingStroke(@NonNull Canvas canvas, @NonNull Path path,
                                    @NonNull Paint paint) {
    Matrix currentMatrix = canvas.getMatrix();
    Path transformedPath = new Path();
    path.transform(currentMatrix, transformedPath);

    Shader shader = paint.getShader();
    Matrix previousShaderMatrix = null;
    boolean hadShaderMatrix = false;
    if (shader != null) {
      previousShaderMatrix = new Matrix();
      hadShaderMatrix = shader.getLocalMatrix(previousShaderMatrix);
      Matrix adjustedShaderMatrix =
          hadShaderMatrix ? new Matrix(previousShaderMatrix) : new Matrix();
      adjustedShaderMatrix.postConcat(currentMatrix);
      shader.setLocalMatrix(adjustedShaderMatrix);
    }

    canvas.save();
    canvas.setMatrix(new Matrix());
    canvas.drawPath(transformedPath, paint);
    canvas.restore();

    if (shader != null) {
      shader.setLocalMatrix(hadShaderMatrix ? previousShaderMatrix
                                            : new Matrix());
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

    // Convert spreadMode to tileMode
    Shader.TileMode tileMode = Shader.TileMode.CLAMP;
    if (rgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REFLECT) {
      tileMode = Shader.TileMode.MIRROR;
    } else if (rgModel.mSpreadMode == GradientModel.GRADIENT_SPREAD_REPEAT) {
      tileMode = Shader.TileMode.REPEAT;
    }

    float[] form = rgModel.mTransform;

    if (rgModel.mType == GRADIENT_TYPE_OBJECT_BOUNDING_BOX) {
      float width = boundingBox.width();
      float height = boundingBox.height();
      if (width == 0.f || height == 0.f) {
        paint.setColor(colors[stopSize - 1]);
        return paint;
      }

      float r = rgModel.mFr;
      if (r == 0.f || stopSize == 1) {
        paint.setColor(colors[stopSize - 1]);
        return paint;
      }

      // Create shader in objectBoundingBox coordinate space, then map to user
      // space via local matrix. This matches SVG semantics where cx/cy/r and
      // gradientTransform are defined in the gradient coordinate system.
      RadialGradient radialGradient = new RadialGradient(
          rgModel.mCx, rgModel.mCy, r, colors, positions, tileMode);

      Matrix shaderMatrix = new Matrix();
      shaderMatrix.setValues(new float[] {width, 0.f, boundingBox.left, 0.f,
                                          height, boundingBox.top, 0.f, 0.f,
                                          1.f});

      if (form != null) {
        Matrix gradientTransform = new Matrix();
        gradientTransform.setValues(new float[] {form[0], form[2], form[4],
                                                 form[1], form[3], form[5], 0.f,
                                                 0.f, 1.f});
        shaderMatrix.preConcat(gradientTransform);
      }
      radialGradient.setLocalMatrix(shaderMatrix);
      paint.setShader(radialGradient);
      return paint;
    }

    float r = rgModel.mFr;
    if (r == 0.f || stopSize == 1) {
      paint.setColor(colors[stopSize - 1]);
      return paint;
    }

    // Create shader in user space.
    // fx and fy are ignored because Android RadialGradient doesn't support a
    // 'focus' point that is different from cx,cy.
    RadialGradient radialGradient = new RadialGradient(
        rgModel.mCx, rgModel.mCy, r, colors, positions, tileMode);
    if (form != null) {
      Matrix gradientTransform = new Matrix();
      gradientTransform.setValues(new float[] {
          form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f});
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
      float[] resolvedForm = form;
      if (lgModel.mType == GRADIENT_TYPE_OBJECT_BOUNDING_BOX) {
        resolvedForm = resolveObjectBoundingBoxTransform(
            form, boundingBox.left, boundingBox.top, boundingBox.width(),
            boundingBox.height());
      }
      Matrix matrix = new Matrix();
      float[] formValue = new float[] {resolvedForm[0],
                                       resolvedForm[2],
                                       resolvedForm[4],
                                       resolvedForm[1],
                                       resolvedForm[3],
                                       resolvedForm[5],
                                       0.f,
                                       0.f,
                                       1.f};
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

  private Paint createMaskCompositePaint(boolean isLuminanceMask) {
    Paint compositePaint = new Paint();
    compositePaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_IN));
    if (isLuminanceMask) {
      compositePaint.setColorFilter(new ColorMatrixColorFilter(
          new ColorMatrix(LUMINANCE_TO_ALPHA_MATRIX)));
    }
    return compositePaint;
  }

  private void restoreFilterLayer() {
    FilterLayer layer = mFilterLayers.pop();
    Bitmap filtered = applyFilterBitmap(layer.bitmap, layer.operations,
                                        layer.values, layer.parentMatrix);
    mPictureCanvas = layer.parentCanvas;
    mPictureCanvas.save();
    mPictureCanvas.setMatrix(new Matrix());
    Paint bitmapPaint =
        new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    mPictureCanvas.drawBitmap(filtered, 0, 0, bitmapPaint);
    mPictureCanvas.restore();
  }

  private Bitmap applyFilterBitmap(Bitmap source, int[] operations,
                                   float[] values, Matrix matrix) {
    Bitmap current = source;
    int valueIndex = 0;
    for (int operation : operations) {
      switch (operation) {
        case FILTER_OP_BLUR: {
          if (valueIndex + 1 >= values.length) {
            return current;
          }
          float sigmaX = mapVectorLength(matrix, values[valueIndex++], 0f);
          float sigmaY = mapVectorLength(matrix, 0f, values[valueIndex++]);
          current = blurBitmap(current, sigmaX, sigmaY);
          break;
        }
        case FILTER_OP_OFFSET: {
          if (valueIndex + 1 >= values.length) {
            return current;
          }
          float[] vector =
              new float[] {values[valueIndex++], values[valueIndex++]};
          matrix.mapVectors(vector);
          current = offsetBitmap(current, vector[0], vector[1]);
          break;
        }
        case FILTER_OP_COLOR_MATRIX: {
          if (valueIndex + 19 >= values.length) {
            return current;
          }
          float[] colorMatrix = new float[20];
          System.arraycopy(values, valueIndex, colorMatrix, 0, 20);
          valueIndex += 20;
          current = applyColorMatrix(current, colorMatrix, true);
          break;
        }
        case FILTER_OP_LUMINANCE_TO_ALPHA:
          current = applyColorMatrix(current, LUMINANCE_TO_ALPHA_MATRIX, false);
          break;
        default:
          return current;
      }
    }
    return current;
  }

  private float mapVectorLength(Matrix matrix, float x, float y) {
    float[] vector = new float[] {x, y};
    matrix.mapVectors(vector);
    return (float)Math.hypot(vector[0], vector[1]);
  }

  private Bitmap offsetBitmap(Bitmap source, float dx, float dy) {
    if (dx == 0f && dy == 0f) {
      return source;
    }
    Bitmap result = Bitmap.createBitmap(source.getWidth(), source.getHeight(),
                                        Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(result);
    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    canvas.drawBitmap(source, dx, dy, paint);
    return result;
  }

  private Bitmap applyColorMatrix(Bitmap source, float[] matrix,
                                  boolean convertSvgBias) {
    float[] androidMatrix = matrix.clone();
    if (convertSvgBias) {
      androidMatrix[4] *= 255f;
      androidMatrix[9] *= 255f;
      androidMatrix[14] *= 255f;
      androidMatrix[19] *= 255f;
    }
    Bitmap result = Bitmap.createBitmap(source.getWidth(), source.getHeight(),
                                        Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(result);
    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    paint.setColorFilter(
        new ColorMatrixColorFilter(new ColorMatrix(androidMatrix)));
    canvas.drawBitmap(source, 0, 0, paint);
    return result;
  }

  private Bitmap blurBitmap(Bitmap source, float sigmaX, float sigmaY) {
    int radiusX = Math.max(0, (int)Math.ceil(Math.abs(sigmaX) * 3f));
    int radiusY = Math.max(0, (int)Math.ceil(Math.abs(sigmaY) * 3f));
    if (radiusX == 0 && radiusY == 0) {
      return source;
    }

    int width = source.getWidth();
    int height = source.getHeight();
    int[] pixels = new int[width * height];
    int[] temp = new int[width * height];
    source.getPixels(pixels, 0, width, 0, 0, width, height);
    if (radiusX > 0) {
      boxBlurHorizontal(pixels, temp, width, height, radiusX);
    } else {
      System.arraycopy(pixels, 0, temp, 0, pixels.length);
    }
    if (radiusY > 0) {
      boxBlurVertical(temp, pixels, width, height, radiusY);
    } else {
      System.arraycopy(temp, 0, pixels, 0, temp.length);
    }

    Bitmap result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    result.setPixels(pixels, 0, width, 0, 0, width, height);
    return result;
  }

  private void boxBlurHorizontal(int[] src, int[] dst, int width, int height,
                                 int radius) {
    int window = radius * 2 + 1;
    for (int y = 0; y < height; y++) {
      int row = y * width;
      long a = 0;
      long r = 0;
      long g = 0;
      long b = 0;
      for (int i = -radius; i <= radius; i++) {
        int color = src[row + clamp(i, 0, width - 1)];
        a += (color >>> 24) & 0xff;
        r += (color >>> 16) & 0xff;
        g += (color >>> 8) & 0xff;
        b += color & 0xff;
      }
      for (int x = 0; x < width; x++) {
        dst[row + x] = ((int)(a / window) << 24) | ((int)(r / window) << 16) |
                       ((int)(g / window) << 8) | (int)(b / window);
        int remove = src[row + clamp(x - radius, 0, width - 1)];
        int add = src[row + clamp(x + radius + 1, 0, width - 1)];
        a += ((add >>> 24) & 0xff) - ((remove >>> 24) & 0xff);
        r += ((add >>> 16) & 0xff) - ((remove >>> 16) & 0xff);
        g += ((add >>> 8) & 0xff) - ((remove >>> 8) & 0xff);
        b += (add & 0xff) - (remove & 0xff);
      }
    }
  }

  private void boxBlurVertical(int[] src, int[] dst, int width, int height,
                               int radius) {
    int window = radius * 2 + 1;
    for (int x = 0; x < width; x++) {
      long a = 0;
      long r = 0;
      long g = 0;
      long b = 0;
      for (int i = -radius; i <= radius; i++) {
        int color = src[clamp(i, 0, height - 1) * width + x];
        a += (color >>> 24) & 0xff;
        r += (color >>> 16) & 0xff;
        g += (color >>> 8) & 0xff;
        b += color & 0xff;
      }
      for (int y = 0; y < height; y++) {
        dst[y * width + x] = ((int)(a / window) << 24) |
                             ((int)(r / window) << 16) |
                             ((int)(g / window) << 8) | (int)(b / window);
        int remove = src[clamp(y - radius, 0, height - 1) * width + x];
        int add = src[clamp(y + radius + 1, 0, height - 1) * width + x];
        a += ((add >>> 24) & 0xff) - ((remove >>> 24) & 0xff);
        r += ((add >>> 16) & 0xff) - ((remove >>> 16) & 0xff);
        g += ((add >>> 8) & 0xff) - ((remove >>> 8) & 0xff);
        b += (add & 0xff) - (remove & 0xff);
      }
    }
  }

  private int clamp(int value, int min, int max) {
    return Math.max(min, Math.min(value, max));
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

  private static float[] resolveObjectBoundingBoxTransform(
      float[] form, float left, float top, float width, float height) {
    if (form == null || form.length != 6) {
      return form;
    }
    if (width == 0f || height == 0f) {
      return form;
    }
    float[] bboxToUser = new float[] {width, 0f, 0f, height, left, top};
    float[] userToBbox = new float[] {
        1f / width, 0f, 0f, 1f / height, -left / width, -top / height};
    return multiply(multiply(bboxToUser, form), userToBbox);
  }

  private static float[] multiply(float[] lhs, float[] rhs) {
    float a = lhs[0] * rhs[0] + lhs[2] * rhs[1];
    float b = lhs[1] * rhs[0] + lhs[3] * rhs[1];
    float c = lhs[0] * rhs[2] + lhs[2] * rhs[3];
    float d = lhs[1] * rhs[2] + lhs[3] * rhs[3];
    float e = lhs[0] * rhs[4] + lhs[2] * rhs[5] + lhs[4];
    float f = lhs[1] * rhs[4] + lhs[3] * rhs[5] + lhs[5];
    return new float[] {a, b, c, d, e, f};
  }
}
