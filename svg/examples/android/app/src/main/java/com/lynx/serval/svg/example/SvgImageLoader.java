// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.serval.svg.example;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.Log;
import com.lynx.serval.svg.SVGRender;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

// Cache access and resource-ready notifications run on the main thread. Only
// downloading/decoding runs in the pool; drawing callbacks stay synchronous.
final class SvgImageLoader implements AutoCloseable {
  interface BitmapLoader {
    Bitmap load(String url) throws IOException;
  }

  private static final String TAG = "SvgImageLoader";
  private final Handler mainHandler = new Handler(Looper.getMainLooper());
  private final ExecutorService executor = Executors.newFixedThreadPool(2);
  private final BitmapLoader bitmapLoader;
  private final Map<String, Bitmap> cache = new HashMap<>();
  private final Map<String, Set<Runnable>> pending = new HashMap<>();
  private final Set<String> failed = new HashSet<>();
  private boolean closed;

  SvgImageLoader() { this(SvgImageLoader::loadBitmap); }

  SvgImageLoader(BitmapLoader bitmapLoader) {
    this.bitmapLoader = bitmapLoader;
  }

  SVGRender.ResourceManager resourceManager(Runnable onLoaded) {
    return (url, callback) -> {
      if (closed || failed.contains(url)) {
        callback.onFailed();
        return;
      }
      Bitmap bitmap = cache.get(url);
      if (bitmap != null) {
        callback.onSuccess(bitmap);
        return;
      }

      Set<Runnable> listeners = pending.get(url);
      if (listeners == null) {
        listeners = new HashSet<>();
        pending.put(url, listeners);
        executor.execute(() -> load(url));
      }
      listeners.add(onLoaded);
      callback.onFailed();
    };
  }

  void clearCallbacks() {
    for (Set<Runnable> listeners : pending.values()) {
      listeners.clear();
    }
    // A new preview selection can retry images that previously failed.
    failed.clear();
  }

  @Override
  public void close() {
    closed = true;
    executor.shutdownNow();
    mainHandler.removeCallbacksAndMessages(null);
    pending.clear();
    failed.clear();
    cache.clear();
  }

  private void load(String url) {
    Bitmap bitmap = null;
    Exception error = null;
    try {
      bitmap = bitmapLoader.load(url);
    } catch (IOException | RuntimeException e) {
      error = e;
    }
    Bitmap loaded = bitmap;
    Exception failure = error;
    mainHandler.post(() -> complete(url, loaded, failure));
  }

  private void complete(String url, Bitmap bitmap, Exception error) {
    if (closed) {
      return;
    }
    Set<Runnable> listeners = pending.remove(url);
    if (bitmap == null) {
      failed.add(url);
      Log.w(TAG, "Failed to load SVG image: " + url, error);
      return;
    }
    cache.put(url, bitmap);
    for (Runnable listener : listeners) {
      listener.run();
    }
  }

  private static Bitmap loadBitmap(String url) throws IOException {
    Bitmap bitmap;
    if (url.startsWith("data:image/")) {
      int separator = url.indexOf(";base64,");
      if (separator < 0) {
        throw new IOException("Only base64 data images are supported.");
      }
      byte[] data = Base64.decode(url.substring(separator + 8), Base64.DEFAULT);
      bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
    } else {
      HttpURLConnection connection =
          (HttpURLConnection) new URL(url).openConnection();
      connection.setConnectTimeout(10000);
      connection.setReadTimeout(10000);
      try {
        int status = connection.getResponseCode();
        if (status != HttpURLConnection.HTTP_OK) {
          throw new IOException("HTTP " + status);
        }
        try (InputStream stream = connection.getInputStream()) {
          bitmap = BitmapFactory.decodeStream(stream);
        }
      } finally {
        connection.disconnect();
      }
    }
    if (bitmap == null) {
      throw new IOException("Image decoding failed.");
    }
    return bitmap;
  }
}
