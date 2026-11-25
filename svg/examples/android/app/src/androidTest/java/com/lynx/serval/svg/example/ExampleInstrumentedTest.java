// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.text.BoringLayout;
import android.text.TextPaint;
import android.util.Base64;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.serval.svg.SVGRender;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
  private static final int BACKGROUND_COLOR = Color.rgb(0x07, 0x14, 0x23);
  private static final List<String> MASK_EXAMPLE_FILES = Arrays.asList(
      "mask-comprehensive-test.svg", "mask-luminance-gradient-test.svg",
      "mask-alpha-units-test.svg");
  private static final List<String> SMIL_EXAMPLE_FILES =
      Arrays.asList("smil_animation.svg", "smil-basic-attributes.svg",
                    "smil-animate-color.svg", "smil-attribute-matrix.svg",
                    "smil-gradient-stops.svg", "smil-gradient-geometry.svg",
                    "smil-keytimes-calc.svg", "smil-path-morph.svg",
                    "smil-structured-attributes.svg", "smil-viewbox.svg",
                    "smil-restore-cleanup.svg", "smil-motion-transform.svg",
                    "smil-repeat-accumulate.svg", "smil-timing-restart.svg");

  @Test
  public void useAppContext() {
    // Context of the app under test.
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    assertEquals("com.lynx.serval.svg.example", appContext.getPackageName());
  }

  @Test
  public void textAnchor_staticLayoutUsesMeasuredWidth() {
    // Force StaticLayout on Android versions that accept Chinese in BoringLayout.
    String label = "\u200E1011\u2082 中文";
    assertTrue(BoringLayout.isBoring(label, new TextPaint()) == null);
    for (String anchor : Arrays.asList("middle", "end")) {
      String svg = "<svg viewBox='0 0 400 100'><text x='200' y='60' "
                   + "font-size='28' text-anchor='" + anchor + "'>" + label +
                   "</text></svg>";
      Bitmap output = Bitmap.createBitmap(400, 100, Bitmap.Config.ARGB_8888);
      SVGRender.SVGRenderResult result =
          new SVGRender().renderPictureWithResult(svg,
                                                  new Rect(0, 0, 400, 100));
      new Canvas(output).drawPicture(result.picture);
      int left = output.getWidth();
      int right = -1;
      for (int y = 0; y < output.getHeight(); ++y) {
        for (int x = 0; x < output.getWidth(); ++x) {
          if (Color.alpha(output.getPixel(x, y)) > 0) {
            left = Math.min(left, x);
            right = Math.max(right, x);
          }
        }
      }
      assertTrue(anchor + " text should be visible", left <= right);
      int inkAnchor = anchor.equals("middle") ? (left + right) / 2 : right;
      assertTrue(anchor + " anchor should be near x=200, was " + inkAnchor,
                 Math.abs(inkAnchor - 200) <= 6);
    }
  }

  @Test
  public void textReferences_renderLikeLiteralSupplementaryCharacters() {
    Bitmap literal = renderTextSample(
        "A\uD83D\uDE00中<tspan fill='blue'>\uD83D\uDE03\uD83D\uDE80</tspan>Z");
    Bitmap references = renderTextSample(
        "A&#128512;&#x4E2D;<tspan fill='blue'>&#x1F603;\uD83D\uDE80</tspan>Z");
    Bitmap empty = Bitmap.createBitmap(literal.getWidth(), literal.getHeight(),
                                       Bitmap.Config.ARGB_8888);
    assertTrue("literal text should be visible", !literal.sameAs(empty));
    assertTrue("character references should draw the same pixels as literals",
               literal.sameAs(references));
  }

  @Test
  public void textYAlignsPlainTextAndEmojiToBaseline() {
    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    paint.setTextSize(28);
    // U+200E forces StaticLayout even on devices that treat emoji as boring.
    for (String text : Arrays.asList("Hg", "\u200E🍌", "🐒")) {
      Bitmap actual = renderTextSample(text);
      Bitmap reference = Bitmap.createBitmap(
          actual.getWidth(), actual.getHeight(), Bitmap.Config.ARGB_8888);
      new Canvas(reference).drawText(text, 20, 60, paint);

      int top = actual.getHeight();
      int bottom = -1;
      int referenceTop = reference.getHeight();
      int referenceBottom = -1;
      for (int y = 0; y < actual.getHeight(); ++y) {
        for (int x = 0; x < actual.getWidth(); ++x) {
          if (Color.alpha(actual.getPixel(x, y)) > 0) {
            top = Math.min(top, y);
            bottom = y;
          }
          if (Color.alpha(reference.getPixel(x, y)) > 0) {
            referenceTop = Math.min(referenceTop, y);
            referenceBottom = y;
          }
        }
      }
      actual.recycle();
      reference.recycle();
      assertTrue(text + " should render visible ink",
                 top <= bottom && referenceTop <= referenceBottom);
      assertEquals(text + " ink top at SVG y", referenceTop, top, 1);
      assertEquals(text + " ink bottom at SVG y", referenceBottom, bottom, 1);
    }
  }

  private static Bitmap renderTextSample(String text) {
    String svg = "<svg viewBox='0 0 400 100'><text x='20' y='60' "
                 + "font-size='28'>" + text + "</text></svg>";
    SVGRender.SVGRenderResult result =
        new SVGRender().renderPictureWithResult(svg, new Rect(0, 0, 400, 100));
    assertTrue("text should render without diagnostics",
               result.diagnostics.isEmpty());
    Bitmap output = Bitmap.createBitmap(400, 100, Bitmap.Config.ARGB_8888);
    new Canvas(output).drawPicture(result.picture);
    return output;
  }

  @Test
  public void imagePattern_userSpaceRendersLoadedImageWithinCircle()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        readAssetFile(appContext, "svg/pattern-image-remote-whitelist.svg");
    assertImagePatternRendersWithinCircle(content, Color.BLUE);
  }

  @Test
  public void imagePattern_objectBoundingBoxPercentagesUseOuterViewport()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        readAssetFile(appContext, "svg/pattern-image-remote-whitelist.svg");
    content =
        content.replace("patternUnits=\"userSpaceOnUse\"",
                        "patternUnits=\"objectBoundingBox\" "
                            + "patternContentUnits=\"objectBoundingBox\"");
    assertTrue(content.contains("patternContentUnits=\"objectBoundingBox\""));
    assertImagePatternRendersWithinCircle(content, Color.RED);
  }

  private static void assertImagePatternRendersWithinCircle(String content,
                                                            int rightColor) {
    String expectedUrl =
        "https://tosv.byted.org/obj/iOSPackageBackUp/uploadRocketBug/"
        + "2161343/1630292959_tool-off.png";
    Bitmap source = Bitmap.createBitmap(16, 16, Bitmap.Config.ARGB_8888);
    source.eraseColor(Color.RED);
    Paint blue = new Paint();
    blue.setColor(Color.BLUE);
    new Canvas(source).drawRect(8, 0, 16, 16, blue);

    int[] requestCount = {0};
    SVGRender render = new SVGRender();
    render.setResourceManager((url, callback) -> {
      assertEquals(expectedUrl, url);
      ++requestCount[0];
      callback.onSuccess(source);
    });
    SVGRender.SVGRenderResult result =
        render.renderPictureWithResult(content, new Rect(0, 0, 200, 200));
    Bitmap output = Bitmap.createBitmap(200, 200, Bitmap.Config.ARGB_8888);
    new Canvas(output).drawPicture(result.picture);

    assertTrue("image pattern should render without diagnostics",
               result.diagnostics.isEmpty());
    assertTrue("image pattern should request its source", requestCount[0] > 0);
    assertEquals(Color.RED, output.getPixel(50, 100));
    assertEquals(rightColor, output.getPixel(150, 100));
    assertEquals("pattern should be clipped to the circle", Color.TRANSPARENT,
                 output.getPixel(10, 10));
  }

  @Test
  public void patternChildPercentages_matchChromeReferenceProbes()
      throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    JSONObject manifest =
        new JSONObject(readAssetFile(context, "pattern_percent_cases.json"));
    JSONArray cases = manifest.getJSONArray("cases");
    Map<String, Bitmap> images = new HashMap<>();
    for (int i = 0; i < cases.length(); i++) {
      JSONObject fixture = cases.getJSONObject(i);
      String file = fixture.getString("file");
      int width = fixture.getInt("width");
      int height = fixture.getInt("height");
      SVGRender render = new SVGRender();
      render.setResourceManager((url, callback) -> {
        Bitmap image = images.get(url);
        if (image == null) {
          int separator = url.indexOf(";base64,");
          assertTrue("shared image fixtures must be self-contained",
                     url.startsWith("data:image/") && separator >= 0);
          byte[] data =
              Base64.decode(url.substring(separator + 8), Base64.DEFAULT);
          image = BitmapFactory.decodeByteArray(data, 0, data.length);
          images.put(url, image);
        }
        callback.onSuccess(image);
      });
      SVGRender.SVGRenderResult result = render.renderPictureWithResult(
          readAssetFile(context, "svg/" + file), new Rect(0, 0, width, height));
      assertTrue(file + " diagnostics: " + result.errorMessage,
                 result.diagnostics.isEmpty());
      Bitmap output =
          Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
      new Canvas(output).drawPicture(result.picture);
      JSONArray samples = fixture.getJSONArray("samples");
      for (int j = 0; j < samples.length(); j++) {
        JSONObject sample = samples.getJSONObject(j);
        int x = sample.getInt("x");
        int y = sample.getInt("y");
        int pixel = output.getPixel(x, y);
        int[] actual = {Color.red(pixel), Color.green(pixel), Color.blue(pixel),
                        Color.alpha(pixel)};
        JSONArray expected = sample.getJSONArray("rgba");
        for (int channel = 0; channel < actual.length; channel++) {
          assertTrue(file + " (" + x + "," + y + ") channel=" + channel +
                         " actual=" + actual[channel] +
                         " expected=" + expected.getInt(channel),
                     Math.abs(actual[channel] - expected.getInt(channel)) <= 3);
        }
      }
    }
  }

  @Test
  public void maskExampleAsset_isPackagedInAssets() throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();

    String[] svgFiles = appContext.getAssets().list("svg");
    assertTrue("svg assets directory should be available", svgFiles != null);
    List<String> packagedFiles = Arrays.asList(svgFiles);
    for (String fileName : MASK_EXAMPLE_FILES) {
      assertTrue(fileName + " should be packaged into assets/svg",
                 packagedFiles.contains(fileName));
    }
  }

  @Test
  public void smilExampleAssets_arePackagedInAssets() throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();

    String[] svgFiles = appContext.getAssets().list("svg");
    assertTrue("svg assets directory should be available", svgFiles != null);
    List<String> packagedFiles = Arrays.asList(svgFiles);
    for (String fileName : SMIL_EXAMPLE_FILES) {
      assertTrue(fileName + " should be packaged into assets/svg",
                 packagedFiles.contains(fileName));
    }
  }

  @Test
  public void smilExampleAssets_reportAnimationsAndChangeAcrossFrames()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    Rect viewPort = new Rect(0, 0, 320, 180);

    for (String fileName : SMIL_EXAMPLE_FILES) {
      String content = readAssetFile(appContext, "svg/" + fileName);
      SVGRender render = new SVGRender();
      try (SVGRender.SVGSession session = render.createSession(content)) {
        assertTrue(fileName + " session should be valid", session.isValid());
        assertTrue(fileName + " should report animations",
                   session.hasAnimations());

        Bitmap frame0 = renderFrame(session, viewPort, 0.0);
        Bitmap frame1 = renderFrame(session, viewPort, 1.0);
        assertFramesDiffer(fileName + " should change between t=0 and t=1",
                           frame0, frame1);
      }
    }
  }

  @Test
  public void smilAttributeMatrix_rendersExpectedSamplesAtFixedTimes()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-attribute-matrix.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL matrix session should be valid", session.isValid());
      assertTrue("SMIL matrix session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      assertBackground("moving x target should be empty at t=0", frame0, 120,
                       30);
      assertForeground("moving x target should be filled at t=1", frame1, 120,
                       30);

      assertBackground("radius probe should be outside the circle at t=0",
                       frame0, 188, 30);
      assertForeground("radius probe should be inside the circle at t=1",
                       frame1, 188, 30);

      assertPaintTurnsPink(frame0.getPixel(243, 30), frame1.getPixel(243, 30));

      assertBackground("inherited line stroke target should be empty at t=0",
                       frame0, 132, 72);
      assertForeground("inherited line stroke target should be filled at t=1",
                       frame1, 132, 72);

      assertBackground("stroke-width probe should be empty at t=0", frame0, 258,
                       77);
      assertForeground("stroke-width probe should be filled at t=1", frame1,
                       258, 77);

      assertBackground("transform target should be empty at t=0", frame0, 128,
                       128);
      assertForeground("transform target should be filled at t=1", frame1, 128,
                       128);

      assertBackground("motion target should be empty at t=0", frame0, 286,
                       128);
      assertForeground("motion target should be filled at t=1", frame1, 286,
                       128);
    }
  }

  @Test
  public void smilRepeatAccumulate_rendersFiniteAndOngoingIntervals()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        readAssetFile(appContext, "svg/smil-repeat-accumulate.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);
      Bitmap frame3 = renderFrame(session, viewPort, 3.3);

      assertBackground("accumulate endpoint should be empty at t=0", frame0,
                       156, 36);
      assertForeground("accumulate endpoint should freeze after repeats",
                       frame3, 156, 36);

      assertBackground("accumulated transform target should be empty at t=0",
                       frame0, 78, 136);
      assertForeground(
          "accumulated transform target should freeze after repeats", frame3,
          78, 136);

      assertBackground("ongoing repeat target should be empty at t=0", frame0,
                       286, 36);
      assertForeground("ongoing repeat target should be filled at t=1", frame1,
                       286, 36);

      assertBackground("repeatDur freeze target should be empty at t=0", frame0,
                       286, 150);
      assertForeground(
          "repeatDur freeze target should be filled after repeatDur", frame3,
          286, 150);
    }
  }

  @Test
  public void smilGradientStops_renderAnimatedStops() throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-gradient-stops.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL gradient stops session should be valid",
                 session.isValid());
      assertTrue("SMIL gradient stops session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      int startStopColor = frame0.getPixel(34, 41);
      int middleStopColor = frame1.getPixel(34, 41);
      assertTrue(
          "stop-color should move from blue toward pink: start=" +
              colorString(startStopColor) +
              " middle=" + colorString(middleStopColor),
          Color.red(middleStopColor) > Color.red(startStopColor) + 100 &&
              Color.green(startStopColor) > Color.green(middleStopColor) + 70);

      assertBackground("offset stop target should be dark at t=0", frame0, 260,
                       91);
      assertForeground("offset stop target should be filled at t=1", frame1,
                       260, 91);

      assertFarFromColor("stop-opacity href target should start translucent",
                         frame0.getPixel(34, 141), Color.WHITE, 420);
      assertNearColor("stop-opacity href target should become almost opaque",
                      frame1.getPixel(34, 141), Color.WHITE, 110);
    }
  }

  @Test
  public void smilAnimateColor_rendersWebAnimateAndLegacySyntax()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-animate-color.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL animateColor session should be valid",
                 session.isValid());
      assertTrue("SMIL animateColor session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      assertNearColor("rgb() fill should start cyan-like",
                      frame0.getPixel(60, 46), Color.rgb(76, 201, 240), 70);
      int magentaFill = frame1.getPixel(60, 46);
      assertTrue(
          "named color fill should become magenta: " + colorString(magentaFill),
          Color.red(magentaFill) > 200 && Color.blue(magentaFill) > 200 &&
              Color.green(magentaFill) < 80);

      assertFarFromColor("rgba() stroke should start translucent",
                         frame0.getPixel(160, 24), Color.GREEN, 260);
      assertNearColor("named color stroke should become lime",
                      frame1.getPixel(160, 24), Color.GREEN, 95);

      assertBackground("transparent fill should start invisible", frame0, 260,
                       46);
      assertNearColor("transparent fill should animate to yellow",
                      frame1.getPixel(260, 46), Color.YELLOW, 95);

      assertTrue("gradient stop should animate from red toward blue",
                 Color.blue(frame1.getPixel(32, 129)) >
                     Color.blue(frame0.getPixel(32, 129)) + 90);

      assertNearColor("legacy animateColor fill should become blue",
                      frame1.getPixel(270, 129), Color.BLUE, 95);
    }
  }

  @Test
  public void smilGradientGeometry_renderAnimatedGradientAttributes()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        readAssetFile(appContext, "svg/smil-gradient-geometry.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL gradient geometry session should be valid",
                 session.isValid());
      assertTrue("SMIL gradient geometry session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      int linearStart = frame0.getPixel(88, 37);
      int linearMiddle = frame1.getPixel(88, 37);
      assertTrue(
          "linearGradient x2 should brighten the sample: start=" +
              colorString(linearStart) + " middle=" + colorString(linearMiddle),
          Color.red(linearMiddle) > Color.red(linearStart) + 70 &&
              Color.green(linearMiddle) > Color.green(linearStart) + 70 &&
              Color.blue(linearMiddle) > Color.blue(linearStart) + 70);

      assertBackground("radialGradient radius target should be dark at t=0",
                       frame0, 282, 93);
      assertForeground("radialGradient radius target should be filled at t=1",
                       frame1, 282, 93);

      int transformLeftDelta =
          colorDistance(frame0.getPixel(34, 143), frame1.getPixel(34, 143));
      int transformRightDelta =
          colorDistance(frame0.getPixel(140, 143), frame1.getPixel(140, 143));
      assertTrue("gradientTransform should move the gradient: leftDelta=" +
                     transformLeftDelta + " rightDelta=" + transformRightDelta,
                 Math.max(transformLeftDelta, transformRightDelta) > 90);
    }
  }

  @Test
  public void smilPathMorph_rendersCompatiblePathInterpolation()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-path-morph.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL path morph session should be valid", session.isValid());
      assertTrue("SMIL path morph session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      assertBackground("morphed fill target should be empty at t=0", frame0,
                       140, 88);
      assertForeground("morphed fill target should be filled at t=1", frame1,
                       140, 88);

      assertBackground("morphed cubic target should be empty at t=0", frame0,
                       226, 65);
      assertForeground("morphed cubic target should be stroked at t=1", frame1,
                       226, 65);
    }
  }

  @Test
  public void smilStructuredAttributes_renderNumberListInterpolation()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content =
        readAssetFile(appContext, "svg/smil-structured-attributes.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL structured attribute session should be valid",
                 session.isValid());
      assertTrue("SMIL structured attribute session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      assertBackground("points morph target should be empty at t=0", frame0,
                       135, 90);
      assertForeground("points morph target should be filled at t=1", frame1,
                       135, 90);

      assertBackground("dasharray gap target should be empty at t=0", frame0,
                       205, 70);
      assertForeground("dasharray gap target should be stroked at t=1", frame1,
                       205, 70);
    }
  }

  @Test
  public void smilViewBox_rendersAnimatedRootViewBox() throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-viewbox.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      assertTrue("SMIL viewBox session should be valid", session.isValid());
      assertTrue("SMIL viewBox session should report animations",
                 session.hasAnimations());

      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame1 = renderFrame(session, viewPort, 1.0);

      assertBackground("viewBox target should be empty at t=0", frame0, 225,
                       55);
      assertForeground("viewBox target should be filled at t=1", frame1, 225,
                       55);
    }
  }

  @Test
  public void smilRestoreCleanup_doesNotLeakPresentationStateAcrossFrames()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-restore-cleanup.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      Bitmap activeFrame = renderFrame(session, viewPort, 0.5);
      Bitmap loopFrame = renderFrame(session, viewPort, 1.0);
      Bitmap inactiveFrame = renderFrame(session, viewPort, 2.0);

      assertFarFromColor("opacity should be reduced while set is active",
                         activeFrame.getPixel(64, 54), Color.WHITE, 220);
      assertNearColor("opacity should restore to default after set ends",
                      inactiveFrame.getPixel(64, 54), Color.WHITE, 42);

      assertForeground("stroke-width set should draw a thick stroke",
                       activeFrame, 160, 58);
      assertBackground("stroke-width should restore to default thin stroke",
                       inactiveFrame, 160, 58);

      int rectFill = Color.rgb(0x4c, 0xc9, 0xf0);
      assertFarFromColor("fill-opacity should be reduced while set is active",
                         activeFrame.getPixel(250, 46), rectFill, 90);
      assertNearColor("fill-opacity should restore to default after set ends",
                      inactiveFrame.getPixel(250, 46), rectFill, 42);

      assertForeground("looping stroke-width line should be visible at t=1",
                       loopFrame, 160, 136);
    }
  }

  @Test
  public void smilTimingRestart_rendersRestartModesAndDriver()
      throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    String content = readAssetFile(appContext, "svg/smil-timing-restart.svg");
    SVGRender render = new SVGRender();
    Rect viewPort = new Rect(0, 0, 320, 180);

    try (SVGRender.SVGSession session = render.createSession(content)) {
      Bitmap frame0 = renderFrame(session, viewPort, 0.0);
      Bitmap frame06 = renderFrame(session, viewPort, 0.6);
      Bitmap frame09 = renderFrame(session, viewPort, 0.9);
      Bitmap frame10 = renderFrame(session, viewPort, 1.0);
      Bitmap frame16 = renderFrame(session, viewPort, 1.6);

      assertBackground("driver target should be empty at t=0", frame0, 286, 64);
      assertForeground("driver target should be filled at t=1", frame10, 286,
                       64);

      assertForeground(
          "restart=always should use the first interval before restart",
          frame06, 89, 30);
      assertForeground("restart=always should restart on overlapping begin",
                       frame09, 48, 30);

      assertForeground("restart=whenNotActive should keep the active interval",
                       frame09, 119, 68);
      assertForeground(
          "restart=whenNotActive should accept the next inactive begin",
          frame16, 48, 68);

      assertBackground("restart=never endpoint should be empty at t=0", frame0,
                       140, 106);
      assertForeground("restart=never should freeze at the first end state",
                       frame16, 140, 106);
    }
  }

  private static Bitmap renderFrame(SVGRender.SVGSession session, Rect viewPort,
                                    double seconds) {
    Bitmap bitmap = Bitmap.createBitmap(viewPort.width(), viewPort.height(),
                                        Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    List<SVGRender.SVGDiagnostic> diagnostics =
        session.renderToCanvasAtTimeWithDiagnostics(canvas, viewPort, seconds);
    assertTrue("render diagnostics should be empty at t=" + seconds,
               diagnostics.isEmpty());
    return bitmap;
  }

  private static void assertFramesDiffer(String message, Bitmap first,
                                         Bitmap second) {
    int changedSamples = 0;
    for (int y = 4; y < first.getHeight(); y += 8) {
      for (int x = 4; x < first.getWidth(); x += 8) {
        if (colorDistance(first.getPixel(x, y), second.getPixel(x, y)) > 35) {
          changedSamples++;
        }
      }
    }
    assertTrue(message + ": changedSamples=" + changedSamples,
               changedSamples >= 4);
  }

  private static void assertBackground(String message, Bitmap bitmap, int x,
                                       int y) {
    assertTrue(message + ": " + colorString(bitmap.getPixel(x, y)),
               colorDistance(bitmap.getPixel(x, y), BACKGROUND_COLOR) < 28);
  }

  private static void assertForeground(String message, Bitmap bitmap, int x,
                                       int y) {
    int color = bitmap.getPixel(x, y);
    assertTrue(message + ": " + colorString(color),
               Color.alpha(color) > 200 &&
                   colorDistance(color, BACKGROUND_COLOR) > 55);
  }

  private static void assertPaintTurnsPink(int startColor, int middleColor) {
    assertTrue("paint probe should start orange: " + colorString(startColor),
               Color.red(startColor) > 180 && Color.green(startColor) > 100 &&
                   Color.blue(startColor) < 80);
    assertTrue("paint probe should become pink: " + colorString(middleColor),
               Color.red(middleColor) > 180 &&
                   Color.blue(middleColor) > Color.blue(startColor) + 80 &&
                   Color.green(startColor) > Color.green(middleColor) + 35);
  }

  private static void assertNearColor(String message, int actual, int expected,
                                      int maxDistance) {
    assertTrue(message + ": actual=" + colorString(actual) +
                   " expected=" + colorString(expected),
               colorDistance(actual, expected) <= maxDistance);
  }

  private static void assertFarFromColor(String message, int actual,
                                         int expected, int minDistance) {
    assertTrue(message + ": actual=" + colorString(actual) +
                   " expected=" + colorString(expected),
               colorDistance(actual, expected) >= minDistance);
  }

  private static int colorDistance(int first, int second) {
    int dr = Color.red(first) - Color.red(second);
    int dg = Color.green(first) - Color.green(second);
    int db = Color.blue(first) - Color.blue(second);
    return Math.abs(dr) + Math.abs(dg) + Math.abs(db);
  }

  private static String colorString(int color) {
    return "rgba(" + Color.red(color) + ", " + Color.green(color) + ", " +
        Color.blue(color) + ", " + Color.alpha(color) + ")";
  }

  private static String readAssetFile(Context context, String path)
      throws IOException {
    StringBuilder sb = new StringBuilder();
    try (InputStream inputStream = context.getAssets().open(path);
         BufferedReader reader =
             new BufferedReader(new InputStreamReader(inputStream))) {
      String line;
      while ((line = reader.readLine()) != null) {
        sb.append(line).append('\n');
      }
    }
    return sb.toString();
  }
}
