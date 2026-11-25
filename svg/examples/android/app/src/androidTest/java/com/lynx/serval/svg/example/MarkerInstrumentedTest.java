// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.PixelCopy;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.serval.svg.SVGRender;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;

/** Chrome reference probes exercised through the Android backend on a device. */
@RunWith(AndroidJUnit4.class)
public class MarkerInstrumentedTest {
  private static String asset(Context context, String path) throws Exception {
    try (InputStream stream = context.getAssets().open(path)) {
      java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
      byte[] buffer = new byte[4096];
      int length;
      while ((length = stream.read(buffer)) != -1) {
        bytes.write(buffer, 0, length);
      }
      return bytes.toString("UTF-8");
    }
  }

  @Test
  public void markerSoftwareCanvas_matchesChromeReference() throws Exception {
    checkReferences(false);
  }

  @Test
  public void markerHardwareCanvas_matchesChromeReference() throws Exception {
    assertTrue("PixelCopy requires Android 8 or later",
               Build.VERSION.SDK_INT >= 26);
    checkReferences(true);
  }

  @Test
  public void nonScalingStrokeMarkersUseHostCanvasMatrix() {
    String svg =
        "<svg viewBox='0 0 100 40'><defs>"
        + "<marker id='tip' refY='1.5'>"
        + "<rect width='3' height='3' fill='red'/></marker>"
        + "<pattern id='paint' width='10' height='10' "
        + "patternUnits='userSpaceOnUse'>"
        + "<rect width='10' height='10' fill='#00ff00'/></pattern></defs>"
        + "<g fill='none' stroke-width='4' marker-end='url(#tip)'>"
        + "<path d='M10 12H40' stroke='blue' "
        + "vector-effect='non-scaling-stroke'/>"
        + "<path d='M10 24H40' stroke='url(#paint)' "
        + "vector-effect='non-scaling-stroke'/>"
        + "</g></svg>";
    Bitmap bitmap = Bitmap.createBitmap(220, 130, Bitmap.Config.ARGB_8888);
    try (SVGRender.SVGSession session = new SVGRender().createSession(svg)) {
      Canvas canvas = new Canvas(bitmap);
      canvas.translate(3, 5);
      canvas.scale(2, 3);
      session.renderCurrentFrameToCanvas(canvas, new Rect(0, 0, 100, 40));

      assertEquals(Color.BLUE, bitmap.getPixel(40, 41));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(40, 37));
      assertEquals(Color.RED, bitmap.getPixel(88, 36));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(88, 51));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(101, 41));

      assertEquals(Color.GREEN, bitmap.getPixel(40, 77));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(40, 73));
      assertEquals(Color.RED, bitmap.getPixel(88, 72));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(88, 87));
    } finally {
      bitmap.recycle();
    }
  }

  private void checkReferences(boolean hardware) throws Exception {
    Instrumentation instrumentation =
        InstrumentationRegistry.getInstrumentation();
    Context context = instrumentation.getTargetContext();
    JSONObject manifest = new JSONObject(asset(context, "marker_cases.json"));
    assertEquals(1, manifest.getInt("schema_version"));
    JSONArray cases = manifest.getJSONArray("cases");
    assertTrue("Marker manifest must not be empty", cases.length() > 0);
    String mode = hardware ? "hardware" : "software";
    File directory =
        new File(context.getFilesDir(), "marker-regression/" + mode);
    assertTrue(directory.isDirectory() || directory.mkdirs());
    JSONArray reports = new JSONArray();
    int failures = 0;
    Activity activity = null;
    try {
      if (hardware) {
        activity = instrumentation.startActivitySync(
            new Intent(context, MainActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .putExtra("category", "Marker"));
      }
      for (int i = 0; i < cases.length(); ++i) {
        JSONObject fixture = cases.getJSONObject(i);
        String file = fixture.getString("file");
        int width = fixture.getInt("width"), height = fixture.getInt("height");
        SVGRender.SVGRenderResult result =
            new SVGRender().renderPictureWithResult(
                asset(context, "svg/" + file), new Rect(0, 0, width, height));
        assertTrue(file + " diagnostics: " + result.errorMessage,
                   result.diagnostics.isEmpty());
        Bitmap bitmap;
        if (hardware) {
          bitmap = captureHardware(instrumentation, activity, result.picture,
                                   width, height);
        } else {
          bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
          new Canvas(bitmap).drawPicture(result.picture);
        }
        try (FileOutputStream stream = new FileOutputStream(
                 new File(directory, file.replace(".svg", ".png")))) {
          assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream));
        }
        JSONArray samples = fixture.getJSONArray("samples");
        JSONArray probeReports = new JSONArray();
        boolean passed = true;
        for (int j = 0; j < samples.length(); ++j) {
          JSONObject sample = samples.getJSONObject(j);
          int x = sample.getInt("x"), y = sample.getInt("y");
          int pixel = bitmap.getPixel(x, y);
          int[] actual = {Color.red(pixel), Color.green(pixel),
                          Color.blue(pixel), Color.alpha(pixel)};
          JSONArray reference = sample.getJSONArray("rgba");
          JSONArray expected = new JSONArray();
          boolean matched = true;
          for (int channel = 0; channel < 4; ++channel) {
            int value = reference.getInt(channel);
            // A window screenshot composites transparent SVG pixels onto white.
            if (hardware) {
              int alpha = reference.getInt(3);
              value = channel == 3
                          ? 255
                          : (value * alpha + 255 * (255 - alpha) + 127) / 255;
            }
            expected.put(value);
            matched &= Math.abs(actual[channel] - value) <= 3;
          }
          passed &= matched;
          probeReports.put(new JSONObject()
                               .put("x", x)
                               .put("y", y)
                               .put("expected", expected)
                               .put("actual", new JSONArray(actual))
                               .put("passed", matched));
        }
        if (!passed) {
          ++failures;
        }
        reports.put(new JSONObject()
                        .put("file", file)
                        .put("passed", passed)
                        .put("samples", probeReports));
        bitmap.recycle();
      }
    } finally {
      if (activity != null) {
        Activity finished = activity;
        instrumentation.runOnMainSync(finished::finish);
      }
    }
    JSONObject report =
        new JSONObject()
            .put("platform", "android-" + mode)
            .put("model", Build.MANUFACTURER + " " + Build.MODEL)
            .put("android_version", Build.VERSION.RELEASE)
            .put("sdk", Build.VERSION.SDK_INT)
            .put("reference_generator", manifest.getJSONObject("generator"))
            .put("cases", reports)
            .put("failures", failures);
    try (OutputStreamWriter writer = new OutputStreamWriter(
             new FileOutputStream(new File(directory, "results.json")),
             StandardCharsets.UTF_8)) {
      writer.write(report.toString(2));
    }
    assertEquals(mode + " marker probes failed; see " + directory, 0, failures);
  }

  private static Bitmap captureHardware(Instrumentation instrumentation,
                                        Activity activity, Picture picture,
                                        int width, int height)
      throws Exception {
    CountDownLatch drawn = new CountDownLatch(1);
    View[] target = new View[1];
    instrumentation.runOnMainSync(() -> {
      FrameLayout root = new FrameLayout(activity);
      root.setBackgroundColor(Color.WHITE);
      View view = new View(activity) {
        @Override
        protected void onDraw(Canvas canvas) {
          canvas.drawColor(Color.WHITE);
          canvas.drawPicture(picture);
          drawn.countDown();
        }
      };
      target[0] = view;
      root.addView(view, new FrameLayout.LayoutParams(width, height));
      activity.setContentView(root);
    });
    assertTrue("Hardware view did not draw", drawn.await(10, TimeUnit.SECONDS));
    instrumentation.waitForIdleSync();
    assertTrue("Expected a hardware accelerated view",
               target[0].isHardwareAccelerated());
    Bitmap output = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    CountDownLatch copied = new CountDownLatch(1);
    int[] status = {-1};
    instrumentation.runOnMainSync(() -> {
      int[] location = new int[2];
      target[0].getLocationInWindow(location);
      PixelCopy.request(activity.getWindow(),
                        new Rect(location[0], location[1], location[0] + width,
                                 location[1] + height),
                        output, result -> {
                          status[0] = result;
                          copied.countDown();
                        }, new Handler(Looper.getMainLooper()));
    });
    assertTrue("PixelCopy timed out", copied.await(10, TimeUnit.SECONDS));
    assertEquals("PixelCopy failed", PixelCopy.SUCCESS, status[0]);
    return output;
  }
}
