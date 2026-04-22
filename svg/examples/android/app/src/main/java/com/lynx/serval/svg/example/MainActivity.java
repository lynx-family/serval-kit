// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import com.lynx.serval.svg.SVGDrawable;
import com.lynx.serval.svg.SVGRender;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
  private static final String HOST_COLOR_COMPARE_FILE =
      "currentcolor-host-default-compare.svg";
  private static final String HOST_COLOR_OVERRIDE_FILE =
      "currentcolor-content-color-override.svg";
  private static final String CATEGORY_CORE = "Core";
  private static final String CATEGORY_COLOR_PARSING = "ColorParsing";
  private static final String CATEGORY_CURRENT_COLOR = "CurrentColor";
  private static final String CATEGORY_MASK = "Mask";
  private static final String CATEGORY_PATTERN = "Pattern";
  private static final String CATEGORY_VECTOR_EFFECT = "VectorEffect";
  private static final String HOST_DEFAULT_COLOR = "#4F6BFF";
  private static final int PREVIEW_WIDTH_DP = 260;
  private static final int PREVIEW_HEIGHT_DP = 195;
  private Spinner categorySpinner;
  private LinearLayout previewListContainer;
  private final List<String> categories = new ArrayList<>();
  private final Map<String, List<String>> categorizedFiles =
      new LinkedHashMap<>();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    ViewCompat.setOnApplyWindowInsetsListener(
        findViewById(R.id.main), (v, insets) -> {
          Insets systemBars =
              insets.getInsets(WindowInsetsCompat.Type.systemBars());
          v.setPadding(systemBars.left, systemBars.top, systemBars.right,
                       systemBars.bottom);
          return insets;
        });

    categorySpinner = findViewById(R.id.category_spinner);
    previewListContainer = findViewById(R.id.preview_list_container);

    setupCategorySpinner();
  }

  private void setupCategorySpinner() {
    try {
      String[] files = getAssets().list("svg");
      if (files == null || files.length == 0) {
        Toast
            .makeText(this, "No SVG files found in assets/svg",
                      Toast.LENGTH_LONG)
            .show();
        return;
      }

      List<String> fileList = new ArrayList<>(Arrays.asList(files));
      fileList.add("string_test.svg");  // Add string test case
      buildCategorizedFiles(fileList);

      ArrayAdapter<String> categoryAdapter = new ArrayAdapter<>(
          this, android.R.layout.simple_spinner_item, categories);
      categoryAdapter.setDropDownViewResource(
          android.R.layout.simple_spinner_dropdown_item);
      categorySpinner.setAdapter(categoryAdapter);

      categorySpinner.setOnItemSelectedListener(
          new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
              renderCategory(categories.get(position));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
              // Do nothing
            }
          });

      String initialFile = "basic_shapes.svg";
      String initialCategory = categoryForFile(initialFile);
      int categoryIndex = Math.max(categories.indexOf(initialCategory), 0);
      categorySpinner.setSelection(categoryIndex);
      renderCategory(initialCategory);

    } catch (IOException e) {
      e.printStackTrace();
      Toast
          .makeText(this, "Error listing assets: " + e.getMessage(),
                    Toast.LENGTH_LONG)
          .show();
    }
  }

  private void buildCategorizedFiles(List<String> fileList) {
    categorizedFiles.clear();
    categories.clear();
    for (String category : new String[] {
             CATEGORY_CORE, CATEGORY_COLOR_PARSING, CATEGORY_CURRENT_COLOR,
             CATEGORY_MASK, CATEGORY_PATTERN, CATEGORY_VECTOR_EFFECT}) {
      categorizedFiles.put(category, new ArrayList<>());
      categories.add(category);
    }
    for (String fileName : fileList) {
      List<String> files = categorizedFiles.get(categoryForFile(fileName));
      if (files != null) {
        files.add(fileName);
      }
    }
  }

  private String categoryForFile(String fileName) {
    if (fileName.startsWith("color-parsing-")) {
      return CATEGORY_COLOR_PARSING;
    }
    if (fileName.startsWith("currentcolor-")) {
      return CATEGORY_CURRENT_COLOR;
    }
    if (fileName.startsWith("mask-")) {
      return CATEGORY_MASK;
    }
    if (fileName.startsWith("pattern-") ||
        "stroke-gradient-vs-pattern.svg".equals(fileName)) {
      return CATEGORY_PATTERN;
    }
    if (fileName.startsWith("vector-effect-")) {
      return CATEGORY_VECTOR_EFFECT;
    }
    return CATEGORY_CORE;
  }

  private void renderCategory(String category) {
    previewListContainer.removeAllViews();
    List<String> files = categorizedFiles.get(category);
    if (files == null || files.isEmpty()) {
      return;
    }
    LayoutInflater inflater = LayoutInflater.from(this);
    for (String fileName : files) {
      View row = inflater.inflate(R.layout.svg_preview_row,
                                  previewListContainer, false);
      TextView fileNameView = row.findViewById(R.id.preview_name);
      ImageView previewImageView = row.findViewById(R.id.preview_image);
      fileNameView.setText(fileName);
      previewImageView.setContentDescription(fileName);
      previewListContainer.addView(row);
      loadAndRenderSvg(fileName, previewImageView);
    }
  }

  private void loadAndRenderSvg(String fileName, ImageView targetView) {
    if ("string_test.svg".equals(fileName)) {
      String svgContent =
          "<svg width=\"200\" height=\"200\" viewBox=\"0 0 200 200\" xmlns=\"http://www.w3.org/2000/svg\">\n"
          + "  <defs>\n"
          +
          "    <pattern id=\"TrianglePattern\" x=\"0\" y=\"0\" width=\"20\" height=\"20\" patternUnits=\"userSpaceOnUse\">\n"
          + "      <path d=\"M 0 0 L 10 0 L 5 10 Z\" fill=\"red\" />\n"
          + "    </pattern>\n"
          + "  </defs>\n"
          + "\n"
          +
          "  <rect x=\"0\" y=\"0\" width=\"200\" height=\"200\" fill=\"url(#TrianglePattern)\" />\n"
          + "</svg>\n";
      renderSvg(fileName, svgContent, targetView, dpToPx(PREVIEW_WIDTH_DP),
                dpToPx(PREVIEW_HEIGHT_DP));
      return;
    }
    try {
      String content = readAssetFile("svg/" + fileName);
      if (content != null) {
        renderSvg(fileName, content, targetView, dpToPx(PREVIEW_WIDTH_DP),
                  dpToPx(PREVIEW_HEIGHT_DP));
      }
    } catch (IOException e) {
      e.printStackTrace();
      Toast
          .makeText(this, "Error reading file: " + fileName, Toast.LENGTH_SHORT)
          .show();
    }
  }

  private String readAssetFile(String path) throws IOException {
    StringBuilder sb = new StringBuilder();
    try (InputStream is = getAssets().open(path);
         BufferedReader reader =
             new BufferedReader(new InputStreamReader(is))) {
      String line;
      while ((line = reader.readLine()) != null) {
        sb.append(line).append("\n");
      }
    }
    return sb.toString();
  }

  private int dpToPx(int dp) {
    return Math.round(TypedValue.applyDimension(
        TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics()));
  }

  private void renderSvg(String fileName, String svgContent,
                         ImageView targetView, int targetWidth,
                         int targetHeight) {
    SVGRender render = new SVGRender();
    boolean shouldSetHostColor = HOST_COLOR_COMPARE_FILE.equals(fileName) ||
                                 HOST_COLOR_OVERRIDE_FILE.equals(fileName);
    render.setColor(shouldSetHostColor ? HOST_DEFAULT_COLOR : null);
    int safeWidth = Math.max(targetWidth, 1);
    int safeHeight = Math.max(targetHeight, 1);
    Rect renderRect = new Rect(0, 0, safeWidth, safeHeight);
    Picture picture = render.renderPicture(svgContent, renderRect);

    SVGDrawable drawable = new SVGDrawable(picture);
    targetView.setImageDrawable(drawable);
    targetView.invalidate();
  }
}
