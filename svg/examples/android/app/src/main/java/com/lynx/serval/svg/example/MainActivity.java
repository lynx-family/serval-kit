// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MainActivity extends AppCompatActivity {
  private static final class SvgPreviewMetadata {
    final String title;
    final List<String> tags;
    final List<String> descriptions;

    SvgPreviewMetadata(String title, List<String> tags,
                       List<String> descriptions) {
      this.title = title;
      this.tags = tags;
      this.descriptions = descriptions;
    }
  }

  private static final String HOST_COLOR_COMPARE_FILE =
      "currentcolor-host-default-compare.svg";
  private static final String HOST_COLOR_OVERRIDE_FILE =
      "currentcolor-content-color-override.svg";
  private static final String CATEGORY_OTHERS = "Others";
  private static final String CATEGORY_COLOR_PARSING = "ColorParsing";
  private static final String CATEGORY_CURRENT_COLOR = "CurrentColor";
  private static final String CATEGORY_ILLEGAL_PARSING = "IllegalParsing";
  private static final String CATEGORY_MASK = "Mask";
  private static final String CATEGORY_PATTERN = "Pattern";
  private static final String CATEGORY_SVG_ROOT = "SvgRoot";
  private static final String CATEGORY_USE = "Use";
  private static final String CATEGORY_GRADIENT = "Gradient";
  private static final String CATEGORY_SHAPE = "Shape";
  private static final String CATEGORY_VECTOR_EFFECT = "VectorEffect";
  private static final String CATEGORY_AI_SVG = "AI SVG";
  private static final String ADVANCED_DEMO_FILE = "ai_svg_advanced_demo.svg";
  private static final String HOST_DEFAULT_COLOR = "#4F6BFF";
  private static final String DIAGNOSTIC_TAG = "SVGDiagnostic";
  private static final String[] PREVIEW_METADATA_FILES = {
      "svg_root_metadata.json", "svg_shape_metadata.json",
      "svg_use_metadata.json", "svg_gradient_metadata.json"};
  private static final int PREVIEW_WIDTH_DP = 260;
  private static final int PREVIEW_HEIGHT_DP = 195;
  private Spinner categorySpinner;
  private LinearLayout previewListContainer;
  private final List<String> categories = new ArrayList<>();
  private final Map<String, List<String>> categorizedFiles =
      new LinkedHashMap<>();
  private final Map<String, SvgPreviewMetadata> previewMetadataByFile =
      new HashMap<>();

  private void loadPreviewMetadata() {
    previewMetadataByFile.clear();
    for (String metadataFile : PREVIEW_METADATA_FILES) {
      try {
        String content = readAssetFile(metadataFile);
        if (content == null || content.isEmpty()) {
          continue;
        }
        JSONObject root = new JSONObject(content);
        JSONArray cases = root.optJSONArray("cases");
        if (cases == null) {
          continue;
        }
        for (int i = 0; i < cases.length(); i++) {
          JSONObject item = cases.optJSONObject(i);
          if (item == null) {
            continue;
          }
          String fileName = item.optString("fileName", "");
          if (fileName.isEmpty()) {
            continue;
          }
          previewMetadataByFile.put(
              fileName,
              new SvgPreviewMetadata(
                  item.optString("title", fileName),
                  jsonArrayToList(item.optJSONArray("tags")),
                  jsonArrayToList(item.optJSONArray("descriptions"))));
        }
      } catch (IOException | JSONException e) {
        e.printStackTrace();
      }
    }
  }

  private List<String> jsonArrayToList(JSONArray array) {
    List<String> result = new ArrayList<>();
    if (array == null) {
      return result;
    }
    for (int i = 0; i < array.length(); i++) {
      result.add(array.optString(i));
    }
    return result;
  }

  private SvgPreviewMetadata metadataForFile(String fileName) {
    return previewMetadataByFile.get(fileName);
  }

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
      fileList.add(ADVANCED_DEMO_FILE);
      loadPreviewMetadata();
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

      String initialCategory = CATEGORY_SHAPE;
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
             CATEGORY_AI_SVG,
             CATEGORY_SHAPE, CATEGORY_COLOR_PARSING, CATEGORY_CURRENT_COLOR,
             CATEGORY_ILLEGAL_PARSING, CATEGORY_MASK, CATEGORY_PATTERN,
             CATEGORY_SVG_ROOT, CATEGORY_USE, CATEGORY_GRADIENT,
             CATEGORY_VECTOR_EFFECT, CATEGORY_OTHERS}) {
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
    if (ADVANCED_DEMO_FILE.equals(fileName)) {
      return CATEGORY_AI_SVG;
    }
    if (fileName.startsWith("color-parsing-")) {
      return CATEGORY_COLOR_PARSING;
    }
    if (fileName.startsWith("currentcolor-")) {
      return CATEGORY_CURRENT_COLOR;
    }
    if (fileName.startsWith("invalid-")) {
      return CATEGORY_ILLEGAL_PARSING;
    }
    if (fileName.startsWith("mask-")) {
      return CATEGORY_MASK;
    }
    if (fileName.startsWith("pattern-") ||
        "stroke-gradient-vs-pattern.svg".equals(fileName)) {
      return CATEGORY_PATTERN;
    }
    if (fileName.startsWith("svg-root-") ||
        fileName.startsWith("svg-preserve-aspect-ratio-")) {
      return CATEGORY_SVG_ROOT;
    }
    if (fileName.startsWith("use-")) {
      return CATEGORY_USE;
    }
    if (fileName.startsWith("gradient-")) {
      return CATEGORY_GRADIENT;
    }
    if (fileName.startsWith("shape-")) {
      return CATEGORY_SHAPE;
    }
    if (fileName.startsWith("vector-effect-")) {
      return CATEGORY_VECTOR_EFFECT;
    }
    return CATEGORY_OTHERS;
  }

  private void renderCategory(String category) {
    previewListContainer.removeAllViews();
    List<String> files = categorizedFiles.get(category);
    if (files == null || files.isEmpty()) {
      return;
    }
    LayoutInflater inflater = LayoutInflater.from(this);
    for (String fileName : files) {
      if (ADVANCED_DEMO_FILE.equals(fileName)) {
        previewListContainer.addView(createAdvancedDemoRow());
        continue;
      }
      View row = inflater.inflate(R.layout.svg_preview_row,
                                  previewListContainer, false);
      SvgPreviewMetadata metadata = metadataForFile(fileName);
      TextView titleView = row.findViewById(R.id.preview_title);
      TextView fileNameView = row.findViewById(R.id.preview_name);
      TextView tagsView = row.findViewById(R.id.preview_tags);
      TextView descriptionView = row.findViewById(R.id.preview_description);
      ImageView previewImageView = row.findViewById(R.id.preview_image);
      titleView.setText(metadata != null ? metadata.title : fileName);
      fileNameView.setText("case: " + fileName.replace(".svg", ""));
      if (metadata != null && !metadata.tags.isEmpty()) {
        tagsView.setText("tags: " + TextUtils.join(", ", metadata.tags));
        tagsView.setVisibility(View.VISIBLE);
      } else {
        tagsView.setVisibility(View.GONE);
      }
      if (metadata != null && !metadata.descriptions.isEmpty()) {
        StringBuilder descriptionText = new StringBuilder();
        for (int i = 0; i < metadata.descriptions.size(); i++) {
          if (i > 0) {
            descriptionText.append("\n");
          }
          descriptionText.append(i + 1).append(". ").append(
              metadata.descriptions.get(i));
        }
        descriptionView.setText(descriptionText.toString());
        descriptionView.setVisibility(View.VISIBLE);
      } else {
        descriptionView.setVisibility(View.GONE);
      }
      previewImageView.setContentDescription(fileName);
      previewListContainer.addView(row);
      loadAndRenderSvg(fileName, previewImageView);
    }
  }

  private View createAdvancedDemoRow() {
    LinearLayout row = new LinearLayout(this);
    row.setOrientation(LinearLayout.VERTICAL);
    row.setPadding(0, dpToPx(8), 0, dpToPx(16));

    TextView title = new TextView(this);
    title.setText("AI SVG advanced demo");
    title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
    title.setTextColor(0xFF1F2937);
    row.addView(title, new LinearLayout.LayoutParams(
                           ViewGroup.LayoutParams.MATCH_PARENT,
                           ViewGroup.LayoutParams.WRAP_CONTENT));

    TextView description = new TextView(this);
    description.setText(
        "Streams SVG chunks, plays SMIL animation, tap shapes for SVG node events.");
    description.setTextColor(0xFF4B5563);
    description.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
    row.addView(description, new LinearLayout.LayoutParams(
                                 ViewGroup.LayoutParams.MATCH_PARENT,
                                 ViewGroup.LayoutParams.WRAP_CONTENT));

    TextView status = new TextView(this);
    status.setTextColor(0xFF0F766E);
    status.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
    row.addView(status, new LinearLayout.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.WRAP_CONTENT));

    AdvancedSvgDemoView demoView = new AdvancedSvgDemoView(this);
    demoView.setStatusListener(status::setText);
    LinearLayout.LayoutParams demoParams = new LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, dpToPx(260));
    demoParams.topMargin = dpToPx(12);
    row.addView(demoView, demoParams);
    return row;
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
    SVGRender.SVGRenderResult result =
        render.renderPictureWithResult(svgContent, renderRect);
    Picture picture = result.picture;
    if (result.hasError) {
      Log.i(DIAGNOSTIC_TAG,
            "file=" + fileName + " errorMessage=" + result.errorMessage +
                " diagnosticCount=" + result.diagnostics.size());
    }

    SVGDrawable drawable = new SVGDrawable(picture);
    targetView.setImageDrawable(drawable);
    targetView.invalidate();
  }
}
