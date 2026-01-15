// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.servalkit;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import com.lynx.servalkit.demoMarkdown.MarkdownDemoActivity;

public class MainActivity extends AppCompatActivity {

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    EditText inputEditText = findViewById(R.id.inputEditText);
    Button button = findViewById(R.id.button);

    // 为按钮设置点击事件监听器
    button.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        String content = inputEditText.getText().toString();

        Intent intent =
            new Intent(MainActivity.this, MarkdownDemoActivity.class);
        intent.putExtra("content", content);
        startActivity(intent);
      }
    });
  }
}
