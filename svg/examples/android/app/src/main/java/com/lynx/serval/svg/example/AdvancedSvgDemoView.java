// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Picture;
import android.graphics.Rect;
import android.os.Build;
import android.os.Trace;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import com.lynx.serval.svg.SVGRender;
import java.util.List;

public class AdvancedSvgDemoView extends View {
  public interface StatusListener {
    void onStatusChanged(String status);
  }

  private static final String SVG_CONTENT =
      "<svg viewBox=\"0 0 800 800\" xmlns=\"http://www.w3.org/2000/svg\">\n" +
              "    <defs>\n" +
              "      <!-- 背景渐变 -->\n" +
              "      <radialGradient id=\"bgGlow\" cx=\"50%\" cy=\"50%\" r=\"60%\">\n" +
              "        <stop offset=\"0%\" stop-color=\"#101a3a\" />\n" +
              "        <stop offset=\"45%\" stop-color=\"#0a1026\" />\n" +
              "        <stop offset=\"100%\" stop-color=\"#03060f\" />\n" +
              "      </radialGradient>\n" +
              "\n" +
              "      <!-- 核心渐变 -->\n" +
              "      <radialGradient id=\"coreGrad\" cx=\"50%\" cy=\"50%\" r=\"50%\">\n" +
              "        <stop offset=\"0%\" stop-color=\"#ffffff\" />\n" +
              "        <stop offset=\"20%\" stop-color=\"#b8f2ff\" />\n" +
              "        <stop offset=\"55%\" stop-color=\"#4cc9f0\" />\n" +
              "        <stop offset=\"100%\" stop-color=\"#4361ee\" stop-opacity=\"0.15\" />\n" +
              "      </radialGradient>\n" +
              "\n" +
              "      <!-- 环形渐变 -->\n" +
              "      <linearGradient id=\"ringGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">\n" +
              "        <stop offset=\"0%\" stop-color=\"#72efdd\" />\n" +
              "        <stop offset=\"50%\" stop-color=\"#4cc9f0\" />\n" +
              "        <stop offset=\"100%\" stop-color=\"#6930c3\" />\n" +
              "      </linearGradient>\n" +
              "\n" +
              "      <!-- 流光渐变 -->\n" +
              "      <linearGradient id=\"trailGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"0%\">\n" +
              "        <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n" +
              "        <stop offset=\"50%\" stop-color=\"#72efdd\" stop-opacity=\"1\" />\n" +
              "        <stop offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n" +
              "      </linearGradient>\n" +
              "\n" +
              "      <!-- 发光滤镜 -->\n" +
              "      <filter id=\"softGlow\" x=\"-50%\" y=\"-50%\" width=\"200%\" height=\"200%\">\n" +
              "        <feGaussianBlur stdDeviation=\"6\" result=\"blur\" />\n" +
              "        <feMerge>\n" +
              "          <feMergeNode in=\"blur\" />\n" +
              "          <feMergeNode in=\"SourceGraphic\" />\n" +
              "        </feMerge>\n" +
              "      </filter>\n" +
              "\n" +
              "      <filter id=\"strongGlow\" x=\"-80%\" y=\"-80%\" width=\"260%\" height=\"260%\">\n" +
              "        <feGaussianBlur stdDeviation=\"12\" result=\"blur1\" />\n" +
              "        <feGaussianBlur in=\"SourceGraphic\" stdDeviation=\"2\" result=\"blur2\" />\n" +
              "        <feMerge>\n" +
              "          <feMergeNode in=\"blur1\" />\n" +
              "          <feMergeNode in=\"blur2\" />\n" +
              "          <feMergeNode in=\"SourceGraphic\" />\n" +
              "        </feMerge>\n" +
              "      </filter>\n" +
              "\n" +
              "      <!-- 圆形轨道路径 -->\n" +
              "      <path id=\"orbit1\" d=\"M 400,400 m -180,0 a 180,180 0 1,1 360,0 a 180,180 0 1,1 -360,0\" />\n" +
              "      <path id=\"orbit2\" d=\"M 400,400 m -250,0 a 250,250 0 1,1 500,0 a 250,250 0 1,1 -500,0\" />\n" +
              "      <path id=\"orbit3\" d=\"M 400,400 m -120,0 a 120,120 0 1,1 240,0 a 120,120 0 1,1 -240,0\" />\n" +
              "    </defs>\n" +
              "\n" +
              "    <!-- 背景 -->\n" +
              "    <rect width=\"800\" height=\"800\" fill=\"url(#bgGlow)\" />\n" +
              "\n" +
              "    <!-- 背景粒子 -->\n" +
              "    <g opacity=\"0.8\">\n" +
              "      <circle cx=\"120\" cy=\"140\" r=\"2\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.2;1;0.2\" dur=\"3.2s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "      <circle cx=\"200\" cy=\"680\" r=\"1.8\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.1;0.8;0.1\" dur=\"2.6s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "      <circle cx=\"660\" cy=\"160\" r=\"2.2\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.2;1;0.2\" dur=\"4.1s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "      <circle cx=\"700\" cy=\"620\" r=\"1.6\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.15;0.9;0.15\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "      <circle cx=\"540\" cy=\"90\" r=\"1.4\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.2;0.7;0.2\" dur=\"3.8s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "      <circle cx=\"88\" cy=\"480\" r=\"2.4\" fill=\"#d9faff\">\n" +
              "        <animate attributeName=\"opacity\" values=\"0.15;1;0.15\" dur=\"4.4s\" repeatCount=\"indefinite\" />\n" +
              "      </circle>\n" +
              "    </g>\n" +
              "\n" +
              "    <!-- 整体漂浮 -->\n" +
              "    <g>\n" +
              "      <animateTransform\n" +
              "        attributeName=\"transform\"\n" +
              "        type=\"translate\"\n" +
              "        values=\"0 0; 0 -8; 0 0; 0 8; 0 0\"\n" +
              "        dur=\"8s\"\n" +
              "        repeatCount=\"indefinite\"\n" +
              "      />\n" +
              "\n" +
              "      <!-- 外层旋转环 -->\n" +
              "      <g opacity=\"0.8\">\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"250\"\n" +
              "                fill=\"none\"\n" +
              "                stroke=\"url(#ringGrad)\"\n" +
              "                stroke-width=\"2.5\"\n" +
              "                stroke-dasharray=\"6 10\"\n" +
              "                filter=\"url(#softGlow)\">\n" +
              "          <animateTransform\n" +
              "            attributeName=\"transform\"\n" +
              "            type=\"rotate\"\n" +
              "            from=\"0 400 400\"\n" +
              "            to=\"360 400 400\"\n" +
              "            dur=\"26s\"\n" +
              "            repeatCount=\"indefinite\"\n" +
              "          />\n" +
              "        </circle>\n" +
              "\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"180\"\n" +
              "                fill=\"none\"\n" +
              "                stroke=\"#72efdd\"\n" +
              "                stroke-opacity=\"0.35\"\n" +
              "                stroke-width=\"1.5\"\n" +
              "                stroke-dasharray=\"2 12\">\n" +
              "          <animateTransform\n" +
              "            attributeName=\"transform\"\n" +
              "            type=\"rotate\"\n" +
              "            from=\"360 400 400\"\n" +
              "            to=\"0 400 400\"\n" +
              "            dur=\"18s\"\n" +
              "            repeatCount=\"indefinite\"\n" +
              "          />\n" +
              "        </circle>\n" +
              "\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"120\"\n" +
              "                fill=\"none\"\n" +
              "                stroke=\"#4cc9f0\"\n" +
              "                stroke-opacity=\"0.45\"\n" +
              "                stroke-width=\"1.2\"\n" +
              "                stroke-dasharray=\"3 8\">\n" +
              "          <animateTransform\n" +
              "            attributeName=\"transform\"\n" +
              "            type=\"rotate\"\n" +
              "            from=\"0 400 400\"\n" +
              "            to=\"360 400 400\"\n" +
              "            dur=\"12s\"\n" +
              "            repeatCount=\"indefinite\"\n" +
              "          />\n" +
              "        </circle>\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- 波纹脉冲 -->\n" +
              "      <g opacity=\"0.7\">\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"40\" fill=\"none\" stroke=\"#72efdd\" stroke-width=\"2\">\n" +
              "          <animate attributeName=\"r\" values=\"40;120;220\" dur=\"4s\" repeatCount=\"indefinite\" />\n" +
              "          <animate attributeName=\"opacity\" values=\"0.9;0.35;0\" dur=\"4s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"40\" fill=\"none\" stroke=\"#4cc9f0\" stroke-width=\"2\">\n" +
              "          <animate attributeName=\"r\" values=\"40;120;220\" dur=\"4s\" begin=\"1.3s\" repeatCount=\"indefinite\" />\n" +
              "          <animate attributeName=\"opacity\" values=\"0.8;0.3;0\" dur=\"4s\" begin=\"1.3s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"40\" fill=\"none\" stroke=\"#80ffdb\" stroke-width=\"2\">\n" +
              "          <animate attributeName=\"r\" values=\"40;120;220\" dur=\"4s\" begin=\"2.6s\" repeatCount=\"indefinite\" />\n" +
              "          <animate attributeName=\"opacity\" values=\"0.75;0.25;0\" dur=\"4s\" begin=\"2.6s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- 路径流光 -->\n" +
              "      <g opacity=\"0.8\">\n" +
              "        <use href=\"#orbit1\" fill=\"none\" stroke=\"url(#trailGrad)\" stroke-width=\"4\" stroke-linecap=\"round\"\n" +
              "             stroke-dasharray=\"60 500\">\n" +
              "          <animate attributeName=\"stroke-dashoffset\" values=\"0;-560\" dur=\"6s\" repeatCount=\"indefinite\" />\n" +
              "        </use>\n" +
              "        <use href=\"#orbit2\" fill=\"none\" stroke=\"url(#trailGrad)\" stroke-width=\"4\" stroke-linecap=\"round\"\n" +
              "             stroke-dasharray=\"80 760\">\n" +
              "          <animate attributeName=\"stroke-dashoffset\" values=\"0;840\" dur=\"8s\" repeatCount=\"indefinite\" />\n" +
              "        </use>\n" +
              "        <use href=\"#orbit3\" fill=\"none\" stroke=\"url(#trailGrad)\" stroke-width=\"3\" stroke-linecap=\"round\"\n" +
              "             stroke-dasharray=\"40 360\">\n" +
              "          <animate attributeName=\"stroke-dashoffset\" values=\"0;-400\" dur=\"4.5s\" repeatCount=\"indefinite\" />\n" +
              "        </use>\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- 轨道运动小球 -->\n" +
              "      <g filter=\"url(#softGlow)\">\n" +
              "        <circle r=\"7\" fill=\"#ffffff\">\n" +
              "          <animateMotion dur=\"6s\" repeatCount=\"indefinite\" rotate=\"auto\">\n" +
              "            <mpath href=\"#orbit1\" />\n" +
              "          </animateMotion>\n" +
              "          <animate attributeName=\"fill\" values=\"#ffffff;#72efdd;#ffffff\" dur=\"2s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "\n" +
              "        <circle r=\"9\" fill=\"#72efdd\">\n" +
              "          <animateMotion dur=\"8s\" repeatCount=\"indefinite\" rotate=\"auto-reverse\">\n" +
              "            <mpath href=\"#orbit2\" />\n" +
              "          </animateMotion>\n" +
              "          <animate attributeName=\"r\" values=\"7;10;7\" dur=\"2.4s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "\n" +
              "        <circle r=\"5\" fill=\"#4cc9f0\">\n" +
              "          <animateMotion dur=\"4.5s\" repeatCount=\"indefinite\" rotate=\"auto\">\n" +
              "            <mpath href=\"#orbit3\" />\n" +
              "          </animateMotion>\n" +
              "          <animate attributeName=\"opacity\" values=\"0.4;1;0.4\" dur=\"1.8s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- 中心核心 -->\n" +
              "      <g filter=\"url(#strongGlow)\">\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"52\" fill=\"url(#coreGrad)\">\n" +
              "          <animate attributeName=\"r\" values=\"48;56;48\" dur=\"3.5s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "\n" +
              "        <circle cx=\"400\" cy=\"400\" r=\"24\" fill=\"#ffffff\" opacity=\"0.95\">\n" +
              "          <animate attributeName=\"r\" values=\"20;26;20\" dur=\"2.2s\" repeatCount=\"indefinite\" />\n" +
              "          <animate attributeName=\"opacity\" values=\"0.85;1;0.85\" dur=\"2.2s\" repeatCount=\"indefinite\" />\n" +
              "        </circle>\n" +
              "\n" +
              "        <!-- 十字能量线 -->\n" +
              "        <g stroke-linecap=\"round\">\n" +
              "          <line x1=\"400\" y1=\"330\" x2=\"400\" y2=\"470\" stroke=\"#72efdd\" stroke-width=\"3\" opacity=\"0.8\">\n" +
              "            <animate attributeName=\"y1\" values=\"340;325;340\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "            <animate attributeName=\"y2\" values=\"460;475;460\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "            <animate attributeName=\"opacity\" values=\"0.4;0.95;0.4\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "          </line>\n" +
              "          <line x1=\"330\" y1=\"400\" x2=\"470\" y2=\"400\" stroke=\"#4cc9f0\" stroke-width=\"3\" opacity=\"0.8\">\n" +
              "            <animate attributeName=\"x1\" values=\"340;325;340\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "            <animate attributeName=\"x2\" values=\"460;475;460\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "            <animate attributeName=\"opacity\" values=\"0.4;0.95;0.4\" dur=\"2.8s\" repeatCount=\"indefinite\" />\n" +
              "          </line>\n" +
              "        </g>\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- 中心旋转菱形组 -->\n" +
              "      <g transform-origin=\"400 400\">\n" +
              "        <animateTransform\n" +
              "          attributeName=\"transform\"\n" +
              "          type=\"rotate\"\n" +
              "          from=\"0\"\n" +
              "          to=\"360\"\n" +
              "          dur=\"10s\"\n" +
              "          repeatCount=\"indefinite\"\n" +
              "        />\n" +
              "        <rect x=\"372\" y=\"372\" width=\"56\" height=\"56\"\n" +
              "              fill=\"none\" stroke=\"#80ffdb\" stroke-width=\"2\"\n" +
              "              transform=\"rotate(45 400 400)\" opacity=\"0.8\" />\n" +
              "        <rect x=\"352\" y=\"352\" width=\"96\" height=\"96\"\n" +
              "              fill=\"none\" stroke=\"#4cc9f0\" stroke-width=\"1.5\"\n" +
              "              transform=\"rotate(45 400 400)\" opacity=\"0.35\" />\n" +
              "      </g>\n" +
              "\n" +
              "      <!-- SMIL 标准能力探针 -->\n" +
              "      <g opacity=\"0.85\">\n" +
              "        <rect id=\"smilHrefTarget\" x=\"70\" y=\"710\" width=\"26\" height=\"26\" rx=\"4\" fill=\"#4cc9f0\">\n" +
              "          <animateColor attributeName=\"fill\" values=\"#4cc9f0;#72efdd;#4cc9f0\" dur=\"3s\" repeatCount=\"indefinite\" />\n" +
              "          <animate attributeName=\"opacity\" values=\"0.35;1;0.35\" keyTimes=\"0;0.35;1\"\n" +
              "                   calcMode=\"spline\" keySplines=\"0.42 0 0.58 1;0.42 0 0.58 1\"\n" +
              "                   dur=\"3s\" repeatCount=\"indefinite\" />\n" +
              "        </rect>\n" +
              "        <animate href=\"#smilHrefTarget\" attributeName=\"x\" values=\"70;116;70\" keyTimes=\"0;0.5;1\"\n" +
              "                 calcMode=\"linear\" dur=\"4s\" repeatCount=\"indefinite\" />\n" +
              "        <circle cx=\"70\" cy=\"760\" r=\"3\" fill=\"#ffffff\">\n" +
              "          <animate attributeName=\"cx\" by=\"38\" dur=\"1.4s\" repeatCount=\"3\" accumulate=\"sum\" fill=\"freeze\" />\n" +
              "        </circle>\n" +
              "        <circle r=\"5\" fill=\"#80ffdb\">\n" +
              "          <animateMotion path=\"M 190 724 C 230 690 260 770 300 724\" keyPoints=\"0;0.25;1\"\n" +
              "                         keyTimes=\"0;0.6;1\" dur=\"5s\" repeatCount=\"indefinite\" rotate=\"auto\" />\n" +
              "          <set attributeName=\"opacity\" to=\"0.95\" begin=\"0s\" dur=\"3s\" />\n" +
              "        </circle>\n" +
              "      </g>\n" +
              "    </g>\n" +
              "  </svg>";

  private final SVGRender render = new SVGRender();
  private Rect renderRect = new Rect(0, 0, 1, 1);
  private long startTimeMillis;
  private StatusListener statusListener;
  private SVGRender.StreamingSVGSession streamingSession;
  private String[] streamChunks;
  private int nextChunkIndex;

  private static void beginTraceSection(String name) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      Trace.beginSection(name);
    }
  }

  private static void endTraceSection() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      Trace.endSection();
    }
  }

  private final Runnable appendNextChunkRunnable =
      new Runnable() {
        @Override
        public void run() {
          appendNextChunk();
        }
      };

  public AdvancedSvgDemoView(Context context) {
    super(context);
    init();
  }

  public AdvancedSvgDemoView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }

  private void init() {
    setClickable(true);
    startTimeMillis = System.currentTimeMillis();
  }

  public void setStatusListener(StatusListener listener) {
    statusListener = listener;
    notifyStatus("ready");
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    startTimeMillis = System.currentTimeMillis();
    startStreamingSession();
    postInvalidateOnAnimation();
  }

  @Override
  protected void onDetachedFromWindow() {
    removeCallbacks(appendNextChunkRunnable);
    if (streamingSession != null) {
      streamingSession.close();
      streamingSession = null;
    }
    super.onDetachedFromWindow();
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    super.onSizeChanged(w, h, oldw, oldh);
    renderRect = new Rect(0, 0, Math.max(w, 1), Math.max(h, 1));
  }

  @Override
  protected void onDraw(Canvas canvas) {
    beginTraceSection("SVGDemo.onDraw");
    try {
      super.onDraw(canvas);
      canvas.drawColor(Color.WHITE);
      double seconds = (System.currentTimeMillis() - startTimeMillis) / 1000.0;
      if (streamingSession != null && streamingSession.isValid()) {
        SVGRender.SVGRenderResult result;
        beginTraceSection("SVGDemo.renderPictureAtTime");
        try {
          result =
              streamingSession.renderPictureAtTimeWithResult(renderRect, seconds);
        } finally {
          endTraceSection();
        }
        Picture picture = result.picture;
        if (picture != null) {
          beginTraceSection("SVGDemo.drawPicture");
          try {
            picture.draw(canvas);
          } finally {
            endTraceSection();
          }
        }
      }
    } finally {
      endTraceSection();
    }
    postInvalidateOnAnimation();
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (event.getAction() == MotionEvent.ACTION_UP) {
      if (streamingSession == null) {
        notifyStatus("hit empty");
        return true;
      }
      SVGRender.SVGHitTestResult result =
          streamingSession.hitTest(renderRect, event.getX(), event.getY());
      if (result.hit) {
        notifyStatus("hit id=" + result.id + " action=" + result.action);
      } else {
        notifyStatus("hit empty");
      }
      return true;
    }
    return true;
  }

  private void startStreamingSession() {
    removeCallbacks(appendNextChunkRunnable);
    if (streamingSession != null) {
      streamingSession.close();
    }
    streamingSession = render.createStreamingSession();
    streamChunks = createLineChunks(SVG_CONTENT);
    nextChunkIndex = 0;
    notifyStatus("streaming 0/" + streamChunks.length);
    post(appendNextChunkRunnable);
  }

  private void appendNextChunk() {
    if (streamingSession == null || streamChunks == null ||
        nextChunkIndex >= streamChunks.length) {
      return;
    }
    final int chunkIndex = nextChunkIndex;
    final SVGRender.StreamingSVGSession session = streamingSession;
    boolean scheduled = session.appendAsync(
        streamChunks[chunkIndex],
        new SVGRender.StreamingCallback() {
          @Override
          public void onComplete(List<SVGRender.SVGDiagnostic> diagnostics) {
            if (streamingSession != session) {
              return;
            }
            nextChunkIndex = chunkIndex + 1;
            postInvalidateOnAnimation();
            if (streamingSession == null || streamChunks == null) {
              return;
            }
            if (nextChunkIndex < streamChunks.length) {
              notifyStatus("streaming " + nextChunkIndex + "/" +
                           streamChunks.length + " diagnostics=" +
                           diagnostics.size());
              postDelayed(appendNextChunkRunnable, 100);
              return;
            }
            finishStreamingSession();
          }
        });
    if (!scheduled) {
      notifyStatus("streaming append skipped");
    }
  }

  private void finishStreamingSession() {
    if (streamingSession == null) {
      return;
    }
    final SVGRender.StreamingSVGSession session = streamingSession;
    boolean scheduled = session.finishAsync(
        new SVGRender.StreamingCallback() {
          @Override
          public void onComplete(List<SVGRender.SVGDiagnostic> diagnostics) {
            if (streamingSession != session) {
              return;
            }
            postInvalidateOnAnimation();
            notifyStatus("final DOM cached diagnostics=" + diagnostics.size());
          }
        });
    if (!scheduled) {
      notifyStatus("final DOM skipped");
    }
  }

  private String[] createLineChunks(String content) {
    String[] lines = content.split("\n", -1);
    for (int i = 0; i < lines.length - 1; ++i) {
      lines[i] = lines[i] + "\n";
    }
    return lines;
  }

  private void notifyStatus(String status) {
    if (statusListener != null) {
      statusListener.onStatusChanged(status);
    }
  }
}
