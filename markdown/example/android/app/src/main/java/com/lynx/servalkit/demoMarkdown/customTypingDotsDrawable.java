// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.servalkit.demoMarkdown;

import android.graphics.*;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class customTypingDotsDrawable extends Drawable {
  private static final int DOT_COUNT = 3;
  private static final int DOT_RADIUS = 8;
  private static final int DOT_SPACING = 20;
  private static final int ANIMATION_DURATION = 600;  // 动画周期600ms

  private final Paint[] dotPaints;
  private final long startTime;

  public customTypingDotsDrawable() {
    // 初始化三个点的画笔
    dotPaints = new Paint[DOT_COUNT];
    int[] colors = {Color.RED, Color.YELLOW, Color.BLUE};

    for (int i = 0; i < DOT_COUNT; i++) {
      dotPaints[i] = new Paint(Paint.ANTI_ALIAS_FLAG);
      dotPaints[i].setColor(colors[i]);
      dotPaints[i].setStyle(Paint.Style.FILL);
    }

    startTime = SystemClock.uptimeMillis();
  }

  @Override
  public void draw(@NonNull Canvas canvas) {
    // 计算动画进度
    long elapsed =
        (SystemClock.uptimeMillis() - startTime) % ANIMATION_DURATION;
    float progress = elapsed / (float)ANIMATION_DURATION;

    // 获取drawable的边界
    Rect bounds = getBounds();
    float centerY = bounds.centerY();
    float startX = bounds.centerX() - (DOT_COUNT - 1) * DOT_SPACING / 2f;

    // 绘制三个点
    for (int i = 0; i < DOT_COUNT; i++) {
      float phase = (progress + i / (float)DOT_COUNT) % 1f;
      // 使用正弦函数创建上下运动
      float yOffset = (float)Math.sin(phase * 2 * Math.PI) * 10;

      canvas.drawCircle(startX + i * DOT_SPACING, centerY + yOffset, DOT_RADIUS,
                        dotPaints[i]);
    }

    // 触发下一帧动画
    invalidateSelf();
  }

  @Override
  public void setAlpha(int alpha) {
    for (Paint paint : dotPaints) {
      paint.setAlpha(alpha);
    }
  }

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter) {
    for (Paint paint : dotPaints) {
      paint.setColorFilter(colorFilter);
    }
  }

  @Override
  public int getOpacity() {
    return PixelFormat.TRANSLUCENT;
  }
}
