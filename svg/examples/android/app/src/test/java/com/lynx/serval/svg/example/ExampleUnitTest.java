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
  private static final List<String> MASK_EXAMPLE_FILES = Arrays.asList(
      "mask-comprehensive-test.svg",
      "mask-luminance-gradient-test.svg",
      "mask-alpha-units-test.svg");

  @Test
  public void maskExampleAssets_areCheckedIn() {
    for (String fileName : MASK_EXAMPLE_FILES) {
      Path assetPath = ASSET_DIR.resolve(fileName);
      assertTrue(
          "mask example asset should exist at " + assetPath,
          Files.exists(assetPath));
    }
  }

  @Test
  public void maskExampleAssets_coverMaskModesAndUnits() throws IOException {
    for (String fileName : MASK_EXAMPLE_FILES) {
      Path assetPath = ASSET_DIR.resolve(fileName);
      String content =
          new String(Files.readAllBytes(assetPath), StandardCharsets.UTF_8);

      assertTrue(fileName + " should define at least one <mask>",
          content.contains("<mask "));
      assertTrue(fileName + " should apply a mask to a drawable element",
          content.contains("mask=\"url(#"));
    }

    String luminanceContent = new String(Files.readAllBytes(
        ASSET_DIR.resolve("mask-luminance-gradient-test.svg")), StandardCharsets.UTF_8);
    assertTrue("luminance test asset should explicitly cover mask-type=luminance",
        luminanceContent.contains("mask-type=\"luminance\""));
    assertTrue("luminance test asset should cover gradient-driven masks",
        luminanceContent.contains("<linearGradient"));

    String alphaContent = new String(Files.readAllBytes(
        ASSET_DIR.resolve("mask-alpha-units-test.svg")), StandardCharsets.UTF_8);
    assertTrue("alpha test asset should explicitly cover mask-type=alpha",
        alphaContent.contains("mask-type=\"alpha\""));
    assertTrue("alpha test asset should cover objectBoundingBox mask content units",
        alphaContent.contains("maskContentUnits=\"objectBoundingBox\""));
  }
}
