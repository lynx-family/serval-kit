// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Canvas;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import com.lynx.serval.svg.SVGDrawable;
import com.lynx.serval.svg.SVGRender;

public class MainActivity extends AppCompatActivity {
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

    ImageView imageView = findViewById(R.id.svg);
    findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View view) {
        Toast.makeText(MainActivity.this, "button on click", Toast.LENGTH_SHORT)
            .show();
        SVGRender render = new SVGRender();
        Picture picture = render.renderPicture(
            "<svg\n"
                + "  width=\"200\"\n"
                + "  height=\"200\"\n"
                + "  viewBox=\"0 0 200 200\"\n"
                + "  xmlns=\"http://www.w3.org/2000/svg\"\n"
                + ">\n"
                +
                "  <circle cx=\"100\" cy=\"100\" r=\"90\" fill=\"#FFF0F5\" />\n"
                +
                "  <circle cx=\"100\" cy=\"100\" r=\"85\" fill=\"#FFB6C1\" />\n"
                +
                "  <circle cx=\"100\" cy=\"100\" r=\"80\" fill=\"#FFC0CB\" />\n"
                + "  <circle cx=\"70\" cy=\"85\" r=\"15\" fill=\"white\" />\n"
                + "  <circle cx=\"130\" cy=\"85\" r=\"15\" fill=\"white\" />\n"
                + "  <circle cx=\"70\" cy=\"85\" r=\"10\" fill=\"black\" />\n"
                + "  <circle cx=\"130\" cy=\"85\" r=\"10\" fill=\"black\" />\n"
                + "  <path\n"
                + "    d=\"M90 115 Q100 125 110 115\"\n"
                + "    stroke=\"black\"\n"
                + "    stroke-width=\"3\"\n"
                + "    fill=\"none\"\n"
                + "  />\n"
                + "  <path d=\"M30 30 Q65 55 70 80\" fill=\"#FFC0CB\" />\n"
                + "  <path d=\"M170 30 Q135 55 130 80\" fill=\"#FFC0CB\" />\n"
                + "  <text\n"
                + "    x=\"135\"\n"
                + "    y=\"39.37822173508929\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >1</text>\n"
                + "  <text\n"
                + "    x=\"160.6217782649107\"\n"
                + "    y=\"65\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >2</text>\n"
                + "  <text\n"
                + "    x=\"170\"\n"
                + "    y=\"100\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >3</text>\n"
                + "  <text\n"
                + "    x=\"160.62177826491072\"\n"
                + "    y=\"135\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >4</text>\n"
                + "  <text\n"
                + "    x=\"135\"\n"
                + "    y=\"160.62177826491072\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >5</text>\n"
                + "  <text\n"
                + "    x=\"100.00000000000001\"\n"
                + "    y=\"170\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >6</text>\n"
                + "  <text\n"
                + "    x=\"65\"\n"
                + "    y=\"160.6217782649107\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >7</text>\n"
                + "  <text\n"
                + "    x=\"39.37822173508931\"\n"
                + "    y=\"135.00000000000003\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >8</text>\n"
                + "  <text\n"
                + "    x=\"30\"\n"
                + "    y=\"100.00000000000001\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >9</text>\n"
                + "  <text\n"
                + "    x=\"39.3782217350893\"\n"
                + "    y=\"65\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >10</text>\n"
                + "  <text\n"
                + "    x=\"64.99999999999997\"\n"
                + "    y=\"39.37822173508931\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >11</text>\n"
                + "  <text\n"
                + "    x=\"99.99999999999999\"\n"
                + "    y=\"30\"\n"
                + "    fill=\"#800080\"\n"
                + "    font-size=\"12\"\n"
                + "    fontWeight=\"bold\"\n"
                + "    text-anchor=\"middle\"\n"
                + "    alignment-baseline=\"central\"\n"
                + "  >12</text>\n"
                + "  <line\n"
                + "    x1=\"100\"\n"
                + "    y1=\"100\"\n"
                + "    x2=\"133.54682271781698\"\n"
                + "    y2=\"121.78556140060107\"\n"
                + "    stroke=\"#800080\"\n"
                + "    stroke-width=\"4\"\n"
                + "    stroke-linecap=\"round\"\n"
                + "  />\n"
                + "  <line\n"
                + "    x1=\"100\"\n"
                + "    y1=\"100\"\n"
                + "    x2=\"132.32818887608602\"\n"
                + "    y2=\"55.50406530937789\"\n"
                + "    stroke=\"#9370DB\"\n"
                + "    stroke-width=\"3\"\n"
                + "    stroke-linecap=\"round\"\n"
                + "  />\n"
                + "  <line\n"
                + "    x1=\"100\"\n"
                + "    y1=\"100\"\n"
                + "    x2=\"43.70834875401149\"\n"
                + "    y2=\"67.5\"\n"
                + "    stroke=\"#FF69B4\"\n"
                + "    stroke-width=\"2\"\n"
                + "    stroke-linecap=\"round\"\n"
                + "  />\n"
                +
                "  <circle cx=\"100\" cy=\"100\" r=\"5\" fill=\"#800080\" />\n"
                + "</svg>",
            new Rect(0, 0, 500, 600));

        SVGDrawable drawable = new SVGDrawable(picture);
        imageView.setImageDrawable(drawable);
        imageView.invalidate();
        ;
      }
    });
  }
}
