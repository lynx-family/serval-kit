// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.os.Looper;
import android.util.Base64;
import android.widget.ImageView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.serval.svg.SVGDrawable;
import com.lynx.serval.svg.SVGRender;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SvgImageLoaderTest {
  @Test
  public void asyncImage_repaintsDirectAndRepeatedPatternFromOneDownload()
      throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String image = "<image width='100%' height='100%' "
                   + "href='https://example.com/image.png'/>";
    String[] contents = {
        "<svg viewBox='0 0 200 200'>" + image + "</svg>",
        "<svg viewBox='0 0 200 200'><defs><pattern id='p' "
            + "patternUnits='userSpaceOnUse' width='50' height='50'>" + image +
            "</pattern></defs><circle cx='100' cy='100' r='100' "
            + "fill='url(#p)'/></svg>"};
    Bitmap source = Bitmap.createBitmap(16, 16, Bitmap.Config.ARGB_8888);
    source.eraseColor(Color.RED);
    Paint blue = new Paint();
    blue.setColor(Color.BLUE);
    new Canvas(source).drawRect(8, 0, 16, 16, blue);
    AtomicInteger downloads = new AtomicInteger();
    AtomicBoolean workerThread = new AtomicBoolean();
    CountDownLatch started = new CountDownLatch(1);
    CountDownLatch release = new CountDownLatch(1);
    CountDownLatch ready = new CountDownLatch(2);
    SvgImageLoader loader = new SvgImageLoader(url -> {
      downloads.incrementAndGet();
      workerThread.set(Looper.myLooper() != Looper.getMainLooper());
      started.countDown();
      try {
        if (!release.await(5, TimeUnit.SECONDS)) {
          throw new IOException("Test image was not released.");
        }
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
        throw new IOException(e);
      }
      return source;
    });
    SvgPreview[] previews = new SvgPreview[2];
    PreviewImageView[] views = new PreviewImageView[2];
    try {
      onMain(() -> {
        for (int i = 0; i < contents.length; ++i) {
          PreviewImageView view = new PreviewImageView(context);
          views[i] = view;
          previews[i] =
              new SvgPreview(contents[i], null, view, 200, 200, loader);
          view.onDrawableChanged = () -> {
            assertEquals(Looper.getMainLooper(), Looper.myLooper());
            ready.countDown();
          };
          assertEquals(Color.TRANSPARENT,
                       draw(view.getDrawable()).getPixel(100, 100));
        }
      });
      assertTrue(started.await(5, TimeUnit.SECONDS));
      release.countDown();
      assertTrue("both previews should receive one resource notification",
                 ready.await(5, TimeUnit.SECONDS));
      onMain(() -> {
        Bitmap direct = draw(views[0].getDrawable());
        assertEquals(Color.RED, direct.getPixel(50, 100));
        assertEquals(Color.BLUE, direct.getPixel(150, 100));
        Bitmap pattern = draw(views[1].getDrawable());
        assertEquals(Color.RED, pattern.getPixel(60, 100));
        // 100% remains the outer 200px viewport, so each 50px tile clips
        // the red quarter of the image rather than rescaling it to the tile.
        assertEquals(Color.RED, pattern.getPixel(85, 100));
        assertEquals(Color.RED, pattern.getPixel(110, 100));
        assertEquals(Color.TRANSPARENT, pattern.getPixel(10, 10));
        assertEquals(0, ready.getCount());
      });
      assertEquals("previews and pattern tiles should share one download", 1,
                   downloads.get());
      assertTrue("image loading must run off the main thread",
                 workerThread.get());
    } finally {
      release.countDown();
      onMain(() -> {
        loader.close();
        for (SvgPreview preview : previews) {
          if (preview != null) {
            preview.close();
          }
        }
      });
    }
  }

  @Test
  public void
  previewWithoutImage_keepsAnimatedSessionAndIgnoresImageInComment() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        "<svg viewBox='0 0 200 200'><!-- <image href='ignored'/> -->"
        + "<rect width='20' height='20' fill='red'>"
        + "<animate attributeName='x' from='0' to='100' dur='1s' "
        + "repeatCount='indefinite'/></rect></svg>";
    SvgImageLoader loader = new SvgImageLoader();
    onMain(() -> {
      ImageView view = new ImageView(context);
      SvgPreview preview =
          new SvgPreview(content, null, view, 200, 200, loader);
      try {
        assertTrue(view.getDrawable() instanceof SVGDrawable);
        assertTrue(((SVGDrawable)view.getDrawable()).hasAnimations());
      } finally {
        preview.close();
        loader.close();
      }
    });
  }

  @Test
  public void dataImagePreview_reRecordsPictureWhenDecoded() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    Bitmap source = Bitmap.createBitmap(4, 4, Bitmap.Config.ARGB_8888);
    source.eraseColor(Color.GREEN);
    ByteArrayOutputStream encoded = new ByteArrayOutputStream();
    assertTrue(source.compress(Bitmap.CompressFormat.PNG, 100, encoded));
    String content =
        "<svg viewBox='0 0 200 200'><image width='100%' height='100%' "
        + "href='data:image/png;base64," +
        Base64.encodeToString(encoded.toByteArray(), Base64.NO_WRAP) +
        "'/></svg>";
    SvgImageLoader loader = new SvgImageLoader();
    CountDownLatch ready = new CountDownLatch(1);
    PreviewImageView[] views = new PreviewImageView[1];
    SvgPreview[] previews = new SvgPreview[1];
    try {
      onMain(() -> {
        views[0] = new PreviewImageView(context);
        previews[0] = new SvgPreview(content, null, views[0], 200, 200, loader);
        views[0].onDrawableChanged = ready::countDown;
        assertEquals(Color.TRANSPARENT,
                     draw(views[0].getDrawable()).getPixel(100, 100));
      });
      assertTrue(ready.await(5, TimeUnit.SECONDS));
      onMain(
          ()
              -> assertEquals(Color.GREEN,
                              draw(views[0].getDrawable()).getPixel(100, 100)));
    } finally {
      onMain(() -> {
        if (previews[0] != null) {
          previews[0].close();
        }
        loader.close();
      });
    }
  }

  @Test
  public void dataImage_isDecodedAsynchronouslyAndCached() throws Exception {
    Bitmap source = Bitmap.createBitmap(4, 4, Bitmap.Config.ARGB_8888);
    source.eraseColor(Color.GREEN);
    ByteArrayOutputStream encoded = new ByteArrayOutputStream();
    assertTrue(source.compress(Bitmap.CompressFormat.PNG, 100, encoded));
    String url = "data:image/png;base64," +
                 Base64.encodeToString(encoded.toByteArray(), Base64.NO_WRAP);
    SvgImageLoader loader = new SvgImageLoader();
    CountDownLatch ready = new CountDownLatch(1);
    Bitmap[] result = new Bitmap[1];
    AtomicInteger misses = new AtomicInteger();
    SVGRender.BitmapRequestCallBack callback =
        new SVGRender.BitmapRequestCallBack() {
          @Override
          public void onSuccess(Bitmap bitmap) {
            result[0] = bitmap;
          }

          @Override
          public void onFailed() {
            misses.incrementAndGet();
          }
        };
    SVGRender.ResourceManager resources =
        loader.resourceManager(ready::countDown);
    try {
      onMain(() -> resources.requestBitMap(url, callback));
      assertEquals(1, misses.get());
      assertTrue(ready.await(5, TimeUnit.SECONDS));
      onMain(() -> {
        resources.requestBitMap(url, callback);
        assertNotNull(result[0]);
        assertEquals(Color.GREEN, result[0].getPixel(2, 2));
        assertEquals(1, misses.get());
      });
    } finally {
      onMain(loader::close);
    }
  }

  private static final class PreviewImageView extends ImageView {
    Runnable onDrawableChanged;

    PreviewImageView(Context context) { super(context); }

    @Override
    public void setImageDrawable(Drawable drawable) {
      super.setImageDrawable(drawable);
      if (onDrawableChanged != null) {
        onDrawableChanged.run();
      }
    }
  }

  private static Bitmap draw(Drawable drawable) {
    Bitmap output = Bitmap.createBitmap(200, 200, Bitmap.Config.ARGB_8888);
    drawable.setBounds(0, 0, 200, 200);
    drawable.draw(new Canvas(output));
    return output;
  }

  private static void onMain(Runnable action) {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(action);
  }
}
