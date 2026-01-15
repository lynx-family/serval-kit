// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.model;

public class Box {
  public float minX, minY, width, height;

  public Box(float minX, float minY, float width, float height) {
    this.minX = minX;
    this.minY = minY;
    this.width = width;
    this.height = height;
  }
}
