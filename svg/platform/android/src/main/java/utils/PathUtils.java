// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.svg.utils;

import android.graphics.Matrix;
import android.graphics.Path;

public class PathUtils {
  public static final float BEZIER_ARC_FACTOR = 0.5522847498f;
  public static final byte SPO_MOVE_TO = 0;
  public static final byte SPO_LINE_TO = 1;
  public static final byte SPO_CUBIC_BEZ = 2;
  public static final byte SPO_QUAD_ARC = 3;
  public static final byte SPO_ELLIPTICAL_ARC = 4;
  public static final byte SPO_CLOSE = 5;

  public static Path makeRectPath(float x, float y, float rx, float ry, float w,
                                  float h, float right, float bottom) {
    Path p = new Path();
    if (rx == 0 || ry == 0) {
      // Simple rect
      p.addRect(x, y, right, bottom, Path.Direction.CW);
    } else {
      // Rounded rect
      float cpx = rx * BEZIER_ARC_FACTOR;
      float cpy = ry * BEZIER_ARC_FACTOR;
      p.moveTo(x, y + ry);
      p.cubicTo(x, y + ry - cpy, x + rx - cpx, y, x + rx, y);
      p.lineTo(right - rx, y);
      p.cubicTo(right - rx + cpx, y, right, y + ry - cpy, right, y + ry);
      p.lineTo(right, bottom - ry);
      p.cubicTo(right, bottom - ry + cpy, right - rx + cpx, bottom, right - rx,
                bottom);
      p.lineTo(x + rx, bottom);
      p.cubicTo(x + rx - cpx, bottom, x, bottom - ry + cpy, x, bottom - ry);
      p.lineTo(x, y + ry);
      p.close();
    }
    return p;
  }

  public static Path makeCirclePath(float cx, float cy, float r) {
    float left = cx - r;
    float top = cy - r;
    float right = cx + r;
    float bottom = cy + r;
    float cp = r * BEZIER_ARC_FACTOR;
    Path p = new Path();
    p.moveTo(right, cy);
    p.cubicTo(right, cy + cp, cx + cp, bottom, cx, bottom);
    p.cubicTo(cx - cp, bottom, left, cy + cp, left, cy);
    p.cubicTo(left, cy - cp, cx - cp, top, cx, top);
    p.cubicTo(cx + cp, top, right, cy - cp, right, cy);
    p.close();
    return p;
  }

  public static Path makeLinePath(float startX, float startY, float endX,
                                  float endY) {
    Path p = new Path();
    p.moveTo(startX, startY);
    p.lineTo(endX, endY);
    return p;
  }

  public static Path makeEllipsePath(float cx, float cy, float rx, float ry) {
    float left = cx - rx;
    float top = cy - ry;
    float right = cx + rx;
    float bottom = cy + ry;
    float cpx = rx * BEZIER_ARC_FACTOR;
    float cpy = ry * BEZIER_ARC_FACTOR;
    Path p = new Path();
    p.moveTo(cx, top);
    p.cubicTo(cx + cpx, top, right, cy - cpy, right, cy);
    p.cubicTo(right, cy + cpy, cx + cpx, bottom, cx, bottom);
    p.cubicTo(cx - cpx, bottom, left, cy + cpy, left, cy);
    p.cubicTo(left, cy - cpy, cx - cpx, top, cx, top);
    p.close();
    return p;
  }

  public static Path makePolygonPath(float[] points, boolean close) {
    Path path = new Path();
    if (points == null || points.length < 2) {
      return path;
    }
    path.moveTo(points[0], points[1]);
    for (int i = 2; i < points.length; i += 2) {
      path.lineTo(points[i], points[i + 1]);
    }
    if (close) {
      path.close();
    }
    return path;
  }

  public static Path makePath(byte[] ops, float[] values) {
    Path path = new Path();
    int pos = 0;
    float x, y, x1, y1, x2, y2;
    for (int i = 0; i < ops.length; i++) {
      int command = ops[i];
      switch (command) {
        case SPO_MOVE_TO:
          x = values[pos++];
          y = values[pos++];
          path.moveTo(x, y);
          break;
        case SPO_LINE_TO:
          x = values[pos++];
          y = values[pos++];
          path.lineTo(x, y);
          break;
        case SPO_CUBIC_BEZ:
          x1 = values[pos++];
          y1 = values[pos++];
          x2 = values[pos++];
          y2 = values[pos++];
          float x3 = values[pos++];
          float y3 = values[pos++];
          path.cubicTo(x1, y1, x2, y2, x3, y3);
          break;
        case SPO_QUAD_ARC:
          x1 = values[pos++];
          y1 = values[pos++];
          x2 = values[pos++];
          y2 = values[pos++];
          path.quadTo(x1, y1, x2, y2);
          break;
        case SPO_CLOSE:
          path.close();
          break;
        default:
          float lastX = values[pos++];
          float lastY = values[pos++];
          float rx = values[pos++];
          float ry = values[pos++];
          float angle = values[pos++];
          boolean largeArcFlag = (int)values[pos++] != 0;
          boolean sweepFlag = (int)values[pos++] != 0;
          x = values[pos++];
          y = values[pos++];
          arcTo(lastX, lastY, rx, ry, angle, largeArcFlag, sweepFlag, x, y,
                path);
          break;
      }
    }
    return path;
  }

  // todo(hujing.1) Sink the method into the SVGAndroidRenderer class
  private static void arcTo(float lastX, float lastY, float rx, float ry,
                            float angle, boolean largeArcFlag,
                            boolean sweepFlag, float x, float y, Path path) {
    if (lastX == x && lastY == y) {
      // If the endpoints (x, y) and (x0, y0) are identical, then this
      // is equivalent to omitting the elliptical arc segment entirely.
      // (behaviour specified by the spec)
      return;
    }

    // Handle degenerate case (behaviour specified by the spec)
    if (rx == 0 || ry == 0) {
      path.lineTo(x, y);
      return;
    }

    // Sign of the radii is ignored (behaviour specified by the spec)
    rx = Math.abs(rx);
    ry = Math.abs(ry);

    // Convert angle from degrees to radians
    double angleRad = Math.toRadians(angle % 360.0);
    double cosAngle = Math.cos(angleRad);
    double sinAngle = Math.sin(angleRad);

    // We simplify the calculations by transforming the arc so that the origin is at the
    // midpoint calculated above followed by a rotation to line up the coordinate axes
    // with the axes of the ellipse.

    // Compute the midpoint of the line between the current and the end point
    double dx2 = (lastX - x) / 2.0;
    double dy2 = (lastY - y) / 2.0;

    // Step 1 : Compute (x1', y1')
    // x1,y1 is the midpoint vector rotated to take the arc's angle out of consideration
    double x1 = (cosAngle * dx2 + sinAngle * dy2);
    double y1 = (-sinAngle * dx2 + cosAngle * dy2);

    double rx_sq = rx * rx;
    double ry_sq = ry * ry;
    double x1_sq = x1 * x1;
    double y1_sq = y1 * y1;

    // Check that radii are large enough.
    // If they are not, the spec says to scale them up so they are.
    // This is to compensate for potential rounding errors/differences between SVG implementations.
    double radiiCheck = x1_sq / rx_sq + y1_sq / ry_sq;
    if (radiiCheck > 0.99999) {
      double radiiScale = Math.sqrt(radiiCheck) * 1.00001;
      rx = (float)(radiiScale * rx);
      ry = (float)(radiiScale * ry);
      rx_sq = rx * rx;
      ry_sq = ry * ry;
    }

    // Step 2 : Compute (cx1, cy1) - the transformed centre point
    double sign = (largeArcFlag == sweepFlag) ? -1 : 1;
    double sq = ((rx_sq * ry_sq) - (rx_sq * y1_sq) - (ry_sq * x1_sq)) /
                ((rx_sq * y1_sq) + (ry_sq * x1_sq));
    sq = (sq < 0) ? 0 : sq;
    double coef = (sign * Math.sqrt(sq));
    double cx1 = coef * ((rx * y1) / ry);
    double cy1 = coef * -((ry * x1) / rx);

    // Step 3 : Compute (cx, cy) from (cx1, cy1)
    double sx2 = (lastX + x) / 2.0;
    double sy2 = (lastY + y) / 2.0;
    double cx = sx2 + (cosAngle * cx1 - sinAngle * cy1);
    double cy = sy2 + (sinAngle * cx1 + cosAngle * cy1);

    // Step 4 : Compute the angleStart (angle1) and the angleExtent (dangle)
    double ux = (x1 - cx1) / rx;
    double uy = (y1 - cy1) / ry;
    double vx = (-x1 - cx1) / rx;
    double vy = (-y1 - cy1) / ry;
    double p, n;

    // Angle betwen two vectors is +/- acos( u.v / len(u) * len(v))
    // Where '.' is the dot product. And +/- is calculated from the sign of the cross product (u x
    // v)

    final double TWO_PI = Math.PI * 2.0;

    // Compute the start angle
    // The angle between (ux,uy) and the 0deg angle (1,0)
    n = Math.sqrt((ux * ux) + (uy * uy));  // len(u) * len(1,0) == len(u)
    p = ux;  // u.v == (ux,uy).(1,0) == (1 * ux) + (0 * uy) == ux
    sign = (uy < 0) ? -1.0 : 1.0;  // u x v == (1 * uy - ux * 0) == uy
    double angleStart =
        sign *
        Math.acos(
            p /
            n);  // No need for checkedArcCos() here. (p >= n) should always be true.

    // Compute the angle extent
    n = Math.sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
    p = ux * vx + uy * vy;
    sign = (ux * vy - uy * vx < 0) ? -1.0f : 1.0f;
    double angleExtent = sign * checkedArcCos(p / n);

    // Catch angleExtents of 0, which will cause problems later in arcToBeziers
    if (angleExtent == 0f) {
      path.lineTo(x, y);
      return;
    }

    if (!sweepFlag && angleExtent > 0) {
      angleExtent -= TWO_PI;
    } else if (sweepFlag && angleExtent < 0) {
      angleExtent += TWO_PI;
    }
    angleExtent %= TWO_PI;
    angleStart %= TWO_PI;

    // Many elliptical arc implementations including the Java2D and Android ones, only
    // support arcs that are axis aligned.  Therefore we need to substitute the arc
    // with bezier curves.  The following method call will generate the beziers for
    // a unit circle that covers the arc angles we want.
    float[] bezierPoints = arcToBeziers(angleStart, angleExtent);

    // Calculate a transformation matrix that will move and scale these bezier points to the correct
    // position.
    Matrix m = new Matrix();
    m.postScale(rx, ry);
    m.postRotate(angle);
    m.postTranslate((float)cx, (float)cy);
    m.mapPoints(bezierPoints);

    // The last point in the bezier set should match exactly the last coord pair in the arc (ie:
    // x,y). But considering all the mathematical manipulation we have been doing, it is bound to be
    // off by a tiny fraction. Experiments show that it can be up to around 0.00002.  So why don't
    // we just set it to exactly what it ought to be.
    bezierPoints[bezierPoints.length - 2] = x;
    bezierPoints[bezierPoints.length - 1] = y;

    // Final step is to add the bezier curves to the path
    for (int i = 0; i < bezierPoints.length; i += 6) {
      path.cubicTo(bezierPoints[i], bezierPoints[i + 1], bezierPoints[i + 2],
                   bezierPoints[i + 3], bezierPoints[i + 4],
                   bezierPoints[i + 5]);
    }
  }

  // Check input to Math.acos() in case rounding or other errors result in a val < -1 or > +1.
  // For example, see the possible KitKat JIT error described in issue #62.
  protected static double checkedArcCos(double val) {
    return (val < -1.0) ? Math.PI : (val > 1.0) ? 0 : Math.acos(val);
  }

  /*
   * Generate the control points and endpoints for a set of bezier curves that match
   * a circular arc starting from angle 'angleStart' and sweep the angle 'angleExtent'.
   * The circle the arc follows will be centred on (0,0) and have a radius of 1.0.
   *
   * Each bezier can cover no more than 90 degrees, so the arc will be divided evenly
   * into a maximum of four curves.
   *
   * The resulting control points will later be scaled and rotated to match the final
   * arc required.
   *
   * The returned array has the format [x0,y0, x1,y1,...] and excludes the start point
   * of the arc.
   */
  protected static float[] arcToBeziers(double angleStart, double angleExtent) {
    int numSegments = (int)Math.ceil(Math.abs(angleExtent) * 2.0 /
                                     Math.PI);  // (angleExtent / 90deg)

    double angleIncrement = angleExtent / numSegments;

    // The length of each control point vector is given by the following formula.
    double controlLength = 4.0 / 3.0 * Math.sin(angleIncrement / 2.0) /
                           (1.0 + Math.cos(angleIncrement / 2.0));

    float[] coords = new float[numSegments * 6];
    int pos = 0;

    for (int i = 0; i < numSegments; i++) {
      double angle = angleStart + i * angleIncrement;
      // Calculate the control vector at this angle
      double dx = Math.cos(angle);
      double dy = Math.sin(angle);
      // First control point
      coords[pos++] = (float)(dx - controlLength * dy);
      coords[pos++] = (float)(dy + controlLength * dx);
      // Second control point
      angle += angleIncrement;
      dx = Math.cos(angle);
      dy = Math.sin(angle);
      coords[pos++] = (float)(dx + controlLength * dy);
      coords[pos++] = (float)(dy - controlLength * dx);
      // Endpoint of bezier
      coords[pos++] = (float)dx;
      coords[pos++] = (float)dy;
    }
    return coords;
  }
}
