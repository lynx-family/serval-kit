// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.Spinner;
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
import java.util.List;

public class MainActivity extends AppCompatActivity {
  private ImageView imageView;
  private Spinner spinner;

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

    imageView = findViewById(R.id.svg_image);
    spinner = findViewById(R.id.svg_spinner);

    setupSpinner();
  }

  private void setupSpinner() {
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
      ArrayAdapter<String> adapter = new ArrayAdapter<>(
          this, android.R.layout.simple_spinner_item, fileList);
      adapter.setDropDownViewResource(
          android.R.layout.simple_spinner_dropdown_item);
      spinner.setAdapter(adapter);

      spinner.setOnItemSelectedListener(
          new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                                       int position, long id) {
              String fileName = fileList.get(position);
              loadAndRenderSvg(fileName);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
              // Do nothing
            }
          });

    } catch (IOException e) {
      e.printStackTrace();
      Toast
          .makeText(this, "Error listing assets: " + e.getMessage(),
                    Toast.LENGTH_LONG)
          .show();
    }
  }

  private void loadAndRenderSvg(String fileName) {
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

      renderSvgWhenReady(svgContent);
      return;
    }
    try {
      String content = readAssetFile("svg/" + fileName);
      if (content != null) {
        renderSvgWhenReady(content);
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

  private void renderSvgWhenReady(String svgContent) {
    if (imageView.getWidth() > 0 && imageView.getHeight() > 0) {
      renderSvg(svgContent, imageView.getWidth(), imageView.getHeight());
      return;
    }
    imageView.post(()
                       -> renderSvg(svgContent, imageView.getWidth(),
                                    imageView.getHeight()));
  }

  private void renderSvg(String svgContent, int targetWidth, int targetHeight) {
    SVGRender render = new SVGRender();
    int safeWidth = Math.max(targetWidth, 1);
    int safeHeight = Math.max(targetHeight, 1);
    Rect renderRect = new Rect(0, 0, safeWidth, safeHeight);
    Picture picture = render.renderPicture(svgContent, renderRect);

    SVGDrawable drawable = new SVGDrawable(picture);
    imageView.setImageDrawable(drawable);
    imageView.invalidate();
  }
}
