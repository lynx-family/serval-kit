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
        Toast.makeText(this, "No SVG files found in assets/svg", Toast.LENGTH_LONG).show();
        return;
      }

      List<String> fileList = new ArrayList<>(Arrays.asList(files));
      fileList.add("string_test.svg"); // Add string test case
      ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, fileList);
      adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
      spinner.setAdapter(adapter);

      spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
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
      Toast.makeText(this, "Error listing assets: " + e.getMessage(), Toast.LENGTH_LONG).show();
    }
  }

  private void loadAndRenderSvg(String fileName) {
    if ("string_test.svg".equals(fileName)) {
      String svgContent = "<svg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12' fill='none'><path d='M16.656 1.029c1.637-0.025 3.262-0.012 4.886-0.025 0.054 2.031 0.878 3.859 2.189 5.213l-0.002-0.002c1.411 1.271 3.247 2.095 5.271 2.235l0.028 0.002v5.036c-1.912-0.048-3.71-0.489-5.331-1.247l0.082 0.034c-0.784-0.377-1.447-0.764-2.077-1.196l0.052 0.034c-0.012 3.649 0.012 7.298-0.025 10.934-0.103 1.853-0.719 3.543-1.707 4.954l0.020-0.031c-1.652 2.366-4.328 3.919-7.371 4.011l-0.014 0c-0.123 0.006-0.268 0.009-0.414 0.009-1.73 0-3.347-0.482-4.725-1.319l0.040 0.023c-2.508-1.509-4.238-4.091-4.558-7.094l-0.004-0.041c-0.025-0.625-0.037-1.25-0.012-1.862 0.49-4.779 4.494-8.476 9.361-8.476 0.547 0 1.083 0.047 1.604 0.136l-0.056-0.008c0.025 1.849-0.050 3.699-0.050 5.548-0.423-0.153-0.911-0.242-1.42-0.242-1.868 0-3.457 1.194-4.045 2.861l-0.009 0.030c-0.133 0.427-0.21 0.918-0.21 1.426 0 0.206 0.013 0.41 0.037 0.61l-0.002-0.024c0.332 2.046 2.086 3.59 4.201 3.59 0.061 0 0.121-0.001 0.181-0.004l-0.009 0c1.463-0.044 2.733-0.831 3.451-1.994l0.010-0.018c0.267-0.372 0.45-0.822 0.511-1.311l0.001-0.014c0.125-2.237 0.075-4.461 0.087-6.698 0.012-5.036-0.012-10.060 0.025-15.083z' fill='#24ccb8'></path></svg>";
      renderSvg(svgContent);
      return;
    }
    try {
      String content = readAssetFile("svg/" + fileName);
      if (content != null) {
        renderSvg(content);
      }
    } catch (IOException e) {
      e.printStackTrace();
      Toast.makeText(this, "Error reading file: " + fileName, Toast.LENGTH_SHORT).show();
    }
  }

  private String readAssetFile(String path) throws IOException {
    StringBuilder sb = new StringBuilder();
    try (InputStream is = getAssets().open(path);
         BufferedReader reader = new BufferedReader(new InputStreamReader(is))) {
      String line;
      while ((line = reader.readLine()) != null) {
        sb.append(line).append("\n");
      }
    }
    return sb.toString();
  }

  private void renderSvg(String svgContent) {
    SVGRender render = new SVGRender();
    // Assuming a default size for rendering if not specified in SVG or for the view bounds
    // The previous example used Rect(0, 0, 500, 600)
    Picture picture = render.renderPicture(svgContent, new Rect(0, 0, 800, 600));

    // Calculate scaling to fit ImageView dimensions while maintaining aspect ratio
    // Note: In a real app, you might want to measure the ImageView first or use a custom View
    // Here we just use the fixed size 800x600 as the intrinsic size of the drawable
    
    SVGDrawable drawable = new SVGDrawable(picture);
    imageView.setImageDrawable(drawable);
    imageView.invalidate();
  }
}
