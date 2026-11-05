// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.RectF;
import android.os.Looper;
import android.view.View;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ServalMarkdownViewTest {
  private static final class ThreadCheckingMarkdownView
      extends ServalMarkdownView {
    int requestLayoutCount;

    ThreadCheckingMarkdownView(Context context) { super(context); }

    @Override
    protected CustomDrawView createCustomView() {
      assertEquals(Looper.getMainLooper(), Looper.myLooper());
      return super.createCustomView();
    }

    @Override
    protected CustomDrawView createRegionView() {
      assertEquals(Looper.getMainLooper(), Looper.myLooper());
      return super.createRegionView();
    }

    @Override
    protected void removeSubView(View view) {
      assertEquals(Looper.getMainLooper(), Looper.myLooper());
      super.removeSubView(view);
    }

    @Override
    public void requestLayout() {
      assertEquals(Looper.getMainLooper(), Looper.myLooper());
      ++requestLayoutCount;
      super.requestLayout();
    }
  }

  private Context getContext() {
    return InstrumentationRegistry.getInstrumentation().getTargetContext();
  }

  private void runOnLayoutThread(Runnable action) throws InterruptedException {
    AtomicReference<Throwable> error = new AtomicReference<>();
    Thread thread = new Thread(() -> {
      try {
        action.run();
      } catch (Throwable exception) {
        error.set(exception);
      }
    }, "Lynx_Layout0");
    thread.start();
    thread.join();
    if (error.get() != null) {
      throw new AssertionError(error.get());
    }
  }

  @Test
  @UiThreadTest
  public void updatesRegionViewsOnUIThread() throws InterruptedException {
    ThreadCheckingMarkdownView view =
        new ThreadCheckingMarkdownView(getContext());
    view.disableInternalVSync(true);
    MarkdownMeasurer measurer = view.getMarkdownMeasurer();
    measurer.setRequestMeasureCallback(() -> {});
    runOnLayoutThread(() -> {
      measurer.setContent("first\n\nsecond");
      measurer.measure(
          View.MeasureSpec.makeMeasureSpec(300, View.MeasureSpec.EXACTLY), 0);
    });

    assertEquals(0, view.getChildCount());
    view.setEnableRegionView(true);
    view.setExternalRenderRegion(true);
    view.setRenderRegion(new RectF(0, 0, 300, 200));
    view.onRendererFrame(0);
    assertTrue(view.getChildCount() > 0);
    view.setEnableRegionView(false);
    assertEquals(0, view.getChildCount());
    view.destroy();
  }

  @Test
  public void postsLayoutRequestsFromLayoutThread()
      throws InterruptedException {
    AtomicReference<ThreadCheckingMarkdownView> reference =
        new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      ThreadCheckingMarkdownView view =
          new ThreadCheckingMarkdownView(getContext());
      view.disableInternalVSync(true);
      view.requestLayoutCount = 0;
      reference.set(view);
    });
    ThreadCheckingMarkdownView view = reference.get();
    runOnLayoutThread(() -> view.getMarkdownMeasurer().markDirty());
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      assertTrue(view.requestLayoutCount > 0);
      view.destroy();
    });
  }

  @Test
  @UiThreadTest
  public void destroysInternallyCreatedMeasurer() {
    ServalMarkdownView view = new ServalMarkdownView(getContext());
    MarkdownMeasurer measurer = view.getMarkdownMeasurer();
    assertNotEquals(0, measurer.getNativeInstance());

    view.destroy();
    view.destroy();

    assertEquals(0, measurer.getNativeInstance());
  }

  @Test
  @UiThreadTest
  public void preservesExternallyCreatedMeasurer() {
    MarkdownMeasurer measurer = new MarkdownMeasurer(getContext());
    ServalMarkdownView view = new ServalMarkdownView(getContext(), false);
    view.setMarkdownMeasurer(measurer);
    long instance = measurer.getNativeInstance();

    view.destroy();

    assertEquals(instance, measurer.getNativeInstance());
    measurer.setContent("measured after view destruction");
    int widthSpec =
        View.MeasureSpec.makeMeasureSpec(300, View.MeasureSpec.EXACTLY);
    long result = measurer.measure(widthSpec, 0);
    assertTrue(MarkdownValuePack.unpackMeasureResultHeight(result) > 0);
    measurer.destroy();
  }

  @Test
  @UiThreadTest
  public void allowsExternalMeasurerToBeDestroyedBeforeView() {
    MarkdownMeasurer measurer = new MarkdownMeasurer(getContext());
    ServalMarkdownView view = new ServalMarkdownView(getContext(), false);
    view.setMarkdownMeasurer(measurer);

    measurer.destroy();
    view.destroy();
    view.destroy();

    assertEquals(0, measurer.getNativeInstance());
  }

  @Test
  @UiThreadTest
  public void rebindsExternalMeasurerAfterViewDestruction() {
    MarkdownMeasurer measurer = new MarkdownMeasurer(getContext());
    measurer.setContent("rebound markdown");
    ServalMarkdownView first = new ServalMarkdownView(getContext(), false);
    first.setMarkdownMeasurer(measurer);
    first.destroy();

    ServalMarkdownView second = new ServalMarkdownView(getContext(), false);
    second.setMarkdownMeasurer(measurer);
    int widthSpec =
        View.MeasureSpec.makeMeasureSpec(300, View.MeasureSpec.EXACTLY);
    second.measure(widthSpec, 0);

    assertTrue(second.getMeasuredHeight() > 0);
    second.destroy();
    measurer.destroy();
  }

  @Test
  @UiThreadTest
  public void destroysExternalMeasurerAndViewOnDifferentThreads()
      throws InterruptedException {
    for (int i = 0; i < 100; ++i) {
      MarkdownMeasurer measurer = new MarkdownMeasurer(getContext());
      ServalMarkdownView view = new ServalMarkdownView(getContext(), false);
      view.setMarkdownMeasurer(measurer);
      CountDownLatch start = new CountDownLatch(1);
      Thread layoutThread = new Thread(() -> {
        try {
          start.await();
        } catch (InterruptedException exception) {
          throw new AssertionError(exception);
        }
        measurer.destroy();
      });
      layoutThread.start();

      start.countDown();
      view.destroy();
      layoutThread.join();

      assertEquals(0, measurer.getNativeInstance());
    }
  }
}
