// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.markdown;

import android.util.Log;
import androidx.annotation.Keep;
import com.lynx.textra.TTText;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@Keep
public class Markdown {
  public static void init(INativeLibraryLoader loader) {
    if (mInitializeStarted) {
      return;
    }
    synchronized (Markdown.class) {
      if (mInitializeStarted) {
        return;
      }
      mInitializeStarted = true;
    }

    try {
      if (loader != null) {
        loader.loadLibrary("serval_markdown");
      } else {
        System.loadLibrary("serval_markdown");
      }
      TTText.Initial();
      initialClassCache();
    } catch (Exception e) {
      Log.e("Markdown", "load library fail: " + e.toString());
    } finally {
      mInitialized = true;
      mInitializedLatch.countDown();
    }
  }

  private static volatile boolean mInitialized = false;
  private static volatile boolean mInitializeStarted = false;
  private static final CountDownLatch mInitializedLatch = new CountDownLatch(1);
  private static final long TIMEOUT = 5;
  public static void ensureInitialized() {
    if (!mInitialized) {
      try {
        init(null);
        boolean completed = mInitializedLatch.await(TIMEOUT, TimeUnit.SECONDS);
        if (!completed) {
          throw new RuntimeException("Markdown initialization timed out.");
        }
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
    }
  }

  private static native void initialClassCache();
}
