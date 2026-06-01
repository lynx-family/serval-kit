// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Picture;
import android.graphics.Rect;
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
      "<svg width=\"320\" height=\"220\" viewBox=\"0 0 320 220\" xmlns=\"http://www.w3.org/2000/svg\">\n"
      + "  <rect x=\"0\" y=\"0\" width=\"320\" height=\"220\" fill=\"#F7F8FA\"/>\n"
      + "  <g id=\"card\" data-click=\"card-group\" transform=\"translate(20 20)\">\n"
      + "    <rect x=\"0\" y=\"0\" width=\"280\" height=\"180\" rx=\"14\" fill=\"#FFFFFF\" stroke=\"#D0D7DE\" stroke-width=\"2\"/>\n"
      + "    <rect id=\"stream_target\" data-action=\"stream-rect\" x=\"28\" y=\"44\" width=\"88\" height=\"72\" rx=\"10\" fill=\"#3B82F6\">\n"
      + "      <animate attributeName=\"x\" from=\"28\" to=\"164\" dur=\"2s\" repeatCount=\"indefinite\"/>\n"
      + "      <animate attributeName=\"opacity\" from=\"0.45\" to=\"1\" dur=\"2s\" repeatCount=\"indefinite\"/>\n"
      + "    </rect>\n"
      + "    <circle id=\"pulse\" onclick=\"pulse-circle\" cx=\"70\" cy=\"142\" r=\"20\" fill=\"#22C55E\">\n"
      + "      <animate attributeName=\"r\" values=\"14;28;14\" dur=\"1.8s\" repeatCount=\"indefinite\"/>\n"
      + "    </circle>\n"
      + "    <rect id=\"rotator\" data-click=\"rotating-rect\" x=\"192\" y=\"122\" width=\"42\" height=\"42\" rx=\"6\" fill=\"#F59E0B\">\n"
      + "      <animateTransform attributeName=\"transform\" type=\"rotate\" from=\"0 213 143\" to=\"360 213 143\" dur=\"3s\" repeatCount=\"indefinite\"/>\n"
      + "    </rect>\n"
      + "  </g>\n"
      + "</svg>\n";

  private final SVGRender render = new SVGRender();
  private Rect renderRect = new Rect(0, 0, 1, 1);
  private long startTimeMillis;
  private StatusListener statusListener;
  private boolean streamChecked;

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
    checkStreamingParse();
    postInvalidateOnAnimation();
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    super.onSizeChanged(w, h, oldw, oldh);
    renderRect = new Rect(0, 0, Math.max(w, 1), Math.max(h, 1));
  }

  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    double seconds = (System.currentTimeMillis() - startTimeMillis) / 1000.0;
    SVGRender.SVGRenderResult result =
        render.renderPictureAtTimeWithResult(SVG_CONTENT, renderRect, seconds);
    Picture picture = result.picture;
    if (picture != null) {
      picture.draw(canvas);
    }
    postInvalidateOnAnimation();
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (event.getAction() == MotionEvent.ACTION_UP) {
      SVGRender.SVGHitTestResult result =
          render.hitTest(SVG_CONTENT, renderRect, event.getX(), event.getY());
      if (result.hit) {
        notifyStatus("hit id=" + result.id + " action=" + result.action);
      } else {
        notifyStatus("hit empty");
      }
      return true;
    }
    return true;
  }

  private void checkStreamingParse() {
    if (streamChecked) {
      return;
    }
    streamChecked = true;
    String[] chunks = SVG_CONTENT.split("\n", -1);
    List<SVGRender.SVGDiagnostic> diagnostics =
        render.parseStreamingWithDiagnostics(chunks);
    notifyStatus("stream chunks=" + chunks.length + " diagnostics=" +
                 diagnostics.size());
  }

  private void notifyStatus(String status) {
    if (statusListener != null) {
      statusListener.onStatusChanged(status);
    }
  }
}
