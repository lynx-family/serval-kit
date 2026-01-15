// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.callback;

import android.graphics.Canvas;

public interface IResourceProvider {
  public void request(Canvas canvas, String url,
                      IResourceRequestCallBack callBack);
}
