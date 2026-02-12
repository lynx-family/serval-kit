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
      String svgContent ="<svg style=\"color: #AFF122;\" xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 20 20\" fill=\"none\"><path d=\"M9.99963 2.34918C10.5438 2.34919 11.0694 2.41243 11.568 2.53472C11.828 2.59871 11.9995 2.83922 11.9996 3.10699C11.9996 3.55099 11.5414 3.85534 11.1071 3.76324C10.7551 3.68853 10.3848 3.64997 9.99963 3.64996C8.51193 3.65014 7.25994 4.19782 6.38342 5.16656C5.50377 6.13906 4.95524 7.58829 4.984 9.46246C5.01177 11.2686 5.66712 13.0185 6.64025 14.3072C7.62329 15.6086 8.85455 16.3489 9.99963 16.3492C11.1448 16.3491 12.3758 15.6087 13.359 14.3072C14.3323 13.0185 14.9875 11.2688 15.0153 9.46246C15.019 9.22093 15.0121 8.98579 14.9957 8.75738C14.9673 8.35983 15.269 7.99957 15.6676 7.99957C15.9854 7.9998 16.2572 8.23267 16.2838 8.54937C16.3093 8.85305 16.32 9.16447 16.3151 9.48297C16.2828 11.5762 15.5288 13.5906 14.3961 15.0904C13.2734 16.5767 11.7012 17.6499 9.99963 17.65C8.29819 17.6497 6.72573 16.5766 5.60315 15.0904C4.47057 13.5906 3.71644 11.576 3.6842 9.48297C3.65132 7.34383 4.27933 5.55502 5.41955 4.29449C6.56303 3.03069 8.1763 2.34936 9.99963 2.34918ZM2.40002 11.1724C2.62688 11.2034 2.80423 11.3843 2.82971 11.6119C2.82978 11.6125 2.82949 11.6139 2.82971 11.6158C2.83019 11.6197 2.83142 11.6262 2.83264 11.6353C2.83514 11.6541 2.83914 11.684 2.84533 11.7232C2.85773 11.8017 2.87796 11.9196 2.90979 12.0689C2.97357 12.3681 3.0828 12.7936 3.26135 13.2896C3.61947 14.2844 4.25244 15.5471 5.35217 16.647C5.54743 16.8423 5.54743 17.1588 5.35217 17.3541C5.1569 17.5492 4.84035 17.5493 4.64514 17.3541C3.4121 16.1208 2.71172 14.7168 2.31994 13.6285C2.28079 13.5197 2.24585 13.4134 2.21252 13.3111L2.12951 13.5201C2.02684 13.7762 1.73634 13.9008 1.4801 13.7984C1.224 13.6958 1.09953 13.4052 1.20178 13.149L1.86779 11.482C1.95293 11.2692 2.17288 11.1417 2.40002 11.1724ZM17.5983 11.1724C17.8255 11.1416 18.0453 11.2691 18.1305 11.482L18.7965 13.149C18.8989 13.4053 18.7745 13.6959 18.5182 13.7984C18.262 13.9007 17.9714 13.7762 17.8688 13.5201L17.7858 13.3111C17.7524 13.4134 17.7175 13.5197 17.6783 13.6285C17.2865 14.7168 16.5862 16.1209 15.3531 17.3541C15.1579 17.5492 14.8413 17.5493 14.6461 17.3541C14.451 17.1588 14.451 16.8423 14.6461 16.647C15.7459 15.5471 16.3788 14.2844 16.7369 13.2896C16.9155 12.7936 17.0247 12.3681 17.0885 12.0689C17.1203 11.9196 17.1406 11.8017 17.153 11.7232C17.1591 11.684 17.1631 11.6541 17.1656 11.6353C17.1669 11.6262 17.1681 11.6197 17.1686 11.6158C17.1688 11.614 17.1685 11.6125 17.1686 11.6119C17.194 11.3844 17.3715 11.2036 17.5983 11.1724ZM15.069 2.40875C15.1203 2.27 15.3165 2.27 15.3678 2.40875L15.6022 3.04058C15.7959 3.56401 16.2086 3.97676 16.7321 4.17047L17.3649 4.40484C17.5033 4.45629 17.5033 4.65222 17.3649 4.70367L16.7321 4.93804C16.2086 5.13176 15.7959 5.5445 15.6022 6.06793L15.3678 6.69976C15.3165 6.83851 15.1203 6.83851 15.069 6.69976L14.8356 6.06793C14.6419 5.54462 14.2289 5.13181 13.7057 4.93804L13.0729 4.70367C12.9341 4.65233 12.9341 4.45618 13.0729 4.40484L13.7057 4.17047C14.2289 3.9767 14.6419 3.56389 14.8356 3.04058L15.069 2.40875Z\" fill=\"#ADF01D\" style=\"fill:#ADF01D;fill:color(display-p3 0.6784 0.9412 0.1137);fill-opacity:1;\"/></svg>";
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

  private void renderSvg(String svgContent) {
    SVGRender render = new SVGRender();
    // Assuming a default size for rendering if not specified in SVG or for the view bounds
    // The previous example used Rect(0, 0, 500, 600)
    Picture picture =
        render.renderPicture(svgContent, new Rect(0, 0, 800, 600));

    // Calculate scaling to fit ImageView dimensions while maintaining aspect ratio
    // Note: In a real app, you might want to measure the ImageView first or use a custom View
    // Here we just use the fixed size 800x600 as the intrinsic size of the drawable

    SVGDrawable drawable = new SVGDrawable(picture);
    imageView.setImageDrawable(drawable);
    imageView.invalidate();
  }
}
