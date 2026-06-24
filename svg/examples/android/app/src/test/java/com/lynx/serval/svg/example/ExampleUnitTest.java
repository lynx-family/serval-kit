// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;

public class ExampleUnitTest {
  private static final Path ASSET_DIR = Paths.get("src/main/assets/svg");
  private static final Path METADATA_DIR = Paths.get("src/main/assets");
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
  public void maskExampleAssets_areCheckedIn() {
    for (String fileName : MASK_EXAMPLE_FILES) {
      Path assetPath = ASSET_DIR.resolve(fileName);
      assertTrue("mask example asset should exist at " + assetPath,
                 Files.exists(assetPath));
    }
  }

  @Test
  public void maskExampleAssets_coverMaskModesAndUnits() throws IOException {
    for (String fileName : MASK_EXAMPLE_FILES) {
      Path assetPath = ASSET_DIR.resolve(fileName);
      String content = readText(assetPath);

      assertTrue(fileName + " should define at least one <mask>",
                 content.contains("<mask "));
      assertTrue(fileName + " should apply a mask to a drawable element",
                 content.contains("mask=\"url(#"));
    }

    String luminanceContent =
        readText(ASSET_DIR.resolve("mask-luminance-gradient-test.svg"));
    assertTrue(
        "luminance test asset should explicitly cover mask-type=luminance",
        luminanceContent.contains("mask-type=\"luminance\""));
    assertTrue("luminance test asset should cover gradient-driven masks",
               luminanceContent.contains("<linearGradient"));

    String alphaContent =
        readText(ASSET_DIR.resolve("mask-alpha-units-test.svg"));
    assertTrue("alpha test asset should explicitly cover mask-type=alpha",
               alphaContent.contains("mask-type=\"alpha\""));
    assertTrue(
        "alpha test asset should cover objectBoundingBox mask content units",
        alphaContent.contains("maskContentUnits=\"objectBoundingBox\""));
  }

  @Test
  public void smilAnimationAssets_areCheckedIn() {
    for (String fileName : SMIL_EXAMPLE_FILES) {
      Path assetPath = ASSET_DIR.resolve(fileName);
      assertTrue("SMIL example asset should exist at " + assetPath,
                 Files.exists(assetPath));
    }
  }

  @Test
  public void smilAnimationMetadata_listsEveryAndroidAsset()
      throws IOException {
    String metadata =
        readText(METADATA_DIR.resolve("svg_smil_animation_metadata.json"));
    for (String fileName : SMIL_EXAMPLE_FILES) {
      assertTrue(fileName + " should be listed in SMIL metadata",
                 metadata.contains("\"fileName\": \"" + fileName + "\""));
    }
  }

  @Test
  public void smilAttributeMatrix_coversAndroidAnimatedAttributes()
      throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-attribute-matrix.svg"));

    for (String attributeName :
         Arrays.asList("x", "r", "x2", "stroke-width", "fill", "transform")) {
      assertTrue("matrix should cover attributeName=" + attributeName,
                 content.contains("attributeName=\"" + attributeName + "\""));
    }
    assertTrue("matrix should cover animateColor",
               content.contains("<animateColor "));
    assertTrue("matrix should cover animateMotion",
               content.contains("<animateMotion "));
    assertTrue("matrix should cover inherited stroke on line",
               content.contains("<g stroke=") && content.contains("<line "));
  }

  @Test
  public void smilAnimateColor_coversWebAnimateAndLegacySyntax()
      throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-animate-color.svg"));

    assertTrue("color case should cover web animate fill",
               content.contains("<animate attributeName=\"fill\""));
    assertTrue("color case should cover web animate stroke",
               content.contains("<animate attributeName=\"stroke\""));
    assertTrue("color case should cover web animate stop-color",
               content.contains("<animate attributeName=\"stop-color\""));
    assertTrue("color case should cover legacy animateColor",
               content.contains("<animateColor attributeName=\"fill\""));
    assertTrue("color case should cover rgb() syntax",
               content.contains("rgb("));
    assertTrue("color case should cover rgba() syntax",
               content.contains("rgba("));
    assertTrue("color case should cover named colors",
               content.contains("magenta") && content.contains("lime"));
    assertTrue("color case should cover transparent",
               content.contains("transparent"));
  }

  @Test
  public void smilGradientStops_coversStopAnimations() throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-gradient-stops.svg"));

    assertTrue("gradient stop case should cover stop-color animation",
               content.contains("attributeName=\"stop-color\""));
    assertTrue("gradient stop case should cover offset animation",
               content.contains("attributeName=\"offset\""));
    assertTrue("gradient stop case should cover href target stop-opacity",
               content.contains("href=\"#fadeStop\"") &&
                   content.contains("attributeName=\"stop-opacity\""));
  }

  @Test
  public void smilGradientGeometry_coversGradientAttributes()
      throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-gradient-geometry.svg"));

    assertTrue("gradient geometry case should cover x2 animation",
               content.contains("attributeName=\"x2\""));
    assertTrue("gradient geometry case should cover radial r animation",
               content.contains("attributeName=\"r\""));
    assertTrue("gradient geometry case should cover gradientTransform",
               content.contains("attributeName=\"gradientTransform\""));
  }

  @Test
  public void smilPathMorph_coversPathDAnimation() throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-path-morph.svg"));

    assertTrue("path morph case should cover path d animation",
               content.contains("attributeName=\"d\""));
    assertTrue("path morph case should cover closed path interpolation",
               content.contains(" Z;"));
    assertTrue("path morph case should cover cubic path interpolation",
               content.contains(" C "));
  }

  @Test
  public void smilStructuredAttributes_coverNumberListAnimations()
      throws IOException {
    String content =
        readText(ASSET_DIR.resolve("smil-structured-attributes.svg"));

    assertTrue("structured case should cover points animation",
               content.contains("attributeName=\"points\""));
    assertTrue("structured case should cover stroke-dasharray animation",
               content.contains("attributeName=\"stroke-dasharray\""));
    assertTrue("structured case should contain comma separated points",
               content.contains("40,130"));
  }

  @Test
  public void smilViewBox_coversRootViewBoxAnimation() throws IOException {
    String content = readText(ASSET_DIR.resolve("smil-viewbox.svg"));

    assertTrue("viewBox case should cover root viewBox animation",
               content.contains("attributeName=\"viewBox\""));
    assertTrue("viewBox case should include the shifted viewBox target",
               content.contains("80 0 320 180"));
  }

  private static String readText(Path path) throws IOException {
    return new String(Files.readAllBytes(path), StandardCharsets.UTF_8);
  }
}
