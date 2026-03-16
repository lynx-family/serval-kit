// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
  private static final List<String> MASK_EXAMPLE_FILES = Arrays.asList(
      "mask-comprehensive-test.svg",
      "mask-luminance-gradient-test.svg",
      "mask-alpha-units-test.svg");

  @Test
  public void useAppContext() {
    // Context of the app under test.
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();
    assertEquals("com.lynx.serval.svg.example", appContext.getPackageName());
  }

  @Test
  public void maskExampleAsset_isPackagedInAssets() throws IOException {
    Context appContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext();

    String[] svgFiles = appContext.getAssets().list("svg");
    assertTrue("svg assets directory should be available", svgFiles != null);
    List<String> packagedFiles = Arrays.asList(svgFiles);
    for (String fileName : MASK_EXAMPLE_FILES) {
      assertTrue(
          fileName + " should be packaged into assets/svg",
          packagedFiles.contains(fileName));
    }
  }
}
