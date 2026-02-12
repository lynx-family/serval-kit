// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/path_harmony_impl.h"
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_rect.h>
namespace serval {
namespace svg {
namespace harmony {

PathHarmonyImpl::PathHarmonyImpl(uint8_t ops[], uint64_t n_ops, float args[], uint64_t n_args) {
    path_ = OH_Drawing_PathCreate();
    uint64_t iArg = 0;
    float x = .0f, y = .0f;
    float cp1x = .0f, cp1y = .0f, cp2x = .0f, cp2y = .0f;
    for (uint64_t i = 0; i < n_ops; i++) {
        switch (ops[i]) {
        case SPO_MOVE_TO:
            x = args[iArg++];
            y = args[iArg++];
            OH_Drawing_PathMoveTo(path_, x, y);
            break;
        case SPO_LINE_TO:
            x = args[iArg++];
            y = args[iArg++];
            OH_Drawing_PathLineTo(path_, x, y);
            break;
        case SPO_CUBIC_BEZ:
            cp1x = args[iArg++];
            cp1y = args[iArg++];
            cp2x = args[iArg++];
            cp2y = args[iArg++];
            x = args[iArg++];
            y = args[iArg++];
            OH_Drawing_PathCubicTo(path_, cp1x, cp1y, cp2x, cp2y, x, y);
            break;
        case SPO_QUAD_ARC:
            cp1x = args[iArg++];
            cp1y = args[iArg++];
            x = args[iArg++];
            y = args[iArg++];
            OH_Drawing_PathQuadTo(path_, cp1x, cp1y, x, y);
            break;
        case SPO_ELLIPTICAL_ARC: {
            float c1x = args[iArg++], c1y = args[iArg++];
            float rx = args[iArg++];
            float ry = args[iArg++];
            float angle = args[iArg++];
            bool largeArc = fabs(args[iArg++]) > 1e-6;
            bool sweep = fabs(args[iArg++]) > 1e-6;
            x = args[iArg++];
            y = args[iArg++];
            SrSVGDrawArc(path_, c1x, c1y, x, y, rx, ry, angle, largeArc, sweep);
            break;
        }
        case SPO_CLOSE:
            OH_Drawing_PathClose(path_);
            break;
        default:
            break;
        }
    }
}

PathHarmonyImpl::PathHarmonyImpl(const PathHarmonyImpl &path) { path_ = OH_Drawing_PathCopy(path.GetPath()); }

static inline float ToRadians(float degrees) { return static_cast<float>(degrees * M_PI / 180.0); }

void PathHarmonyImpl::SrSVGDrawArc(OH_Drawing_Path *path, float x, float y, float x1, float y1, float a, float b,
                                   float theta, bool isMoreThanHalf, bool isPositiveArc) {
    float thetaD = ToRadians(theta);
    float cosTheta = cosf(thetaD);
    float sinTheta = sinf(thetaD);
    float x0p = (x * cosTheta + y * sinTheta) / a;
    float y0p = (-x * sinTheta + y * cosTheta) / b;
    float x1p = (x1 * cosTheta + y1 * sinTheta) / a;
    float y1p = (-x1 * sinTheta + y1 * cosTheta) / b;

    float dx = x0p - x1p;
    float dy = y0p - y1p;
    float xm = (x0p + x1p) / 2;
    float ym = (y0p + y1p) / 2;

    float dCircle = dx * dx + dy * dy;
    if (fabsf(dCircle) < 1e-6) {
        // Path parse error in elliptical arc: all points are coincident
        return;
    }
    float disc = 1.0f / dCircle - 1.0f / 4.0f;
    if (disc < 0) {
        float adjust = sqrtf(dCircle) / 1.99999f;
        SrSVGDrawArc(path, x, y, x1, y1, a * adjust, b * adjust, theta, isMoreThanHalf, isPositiveArc);
        return;
    }
    float s = sqrtf(disc);
    float sDx = s * dx;
    float sDy = s * dy;
    float cx;
    float cy;
    if (isMoreThanHalf == isPositiveArc) {
        cx = xm - sDy;
        cy = ym + sDx;
    } else {
        cx = xm + sDy;
        cy = ym - sDx;
    }
    float eta0 = atan2((y0p - cy), (x0p - cx));
    float eta1 = atan2((y1p - cy), (x1p - cx));
    float sweep = (eta1 - eta0);
    if (isPositiveArc != (sweep >= 0)) {
        if (sweep > 0) {
            sweep -= 2 * M_PI;
        } else {
            sweep += 2 * M_PI;
        }
    }
    cx *= a;
    cy *= b;
    float tCx = cx;
    cx = cx * cosTheta - cy * sinTheta;
    cy = tCx * sinTheta + cy * cosTheta;
    SRSVGArcToBezier(path, cx, cy, a, b, x, y, thetaD, eta0, sweep);
}

void PathHarmonyImpl::SRSVGArcToBezier(OH_Drawing_Path *path, float cx, float cy, float a, float b, float e1x,
                                       float e1y, float theta, float start, float sweep) {
    // Maximum of 45 degrees per cubic Bezier segment
    int numSegments = (int)ceil(fabs(sweep * 4 / M_PI));

    float eta1 = start;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    float cosEta1 = cos(eta1);
    float sinEta1 = sin(eta1);
    float ep1x = (-a * cosTheta * sinEta1) - (b * sinTheta * cosEta1);
    float ep1y = (-a * sinTheta * sinEta1) + (b * cosTheta * cosEta1);

    float anglePerSegment = sweep / numSegments;
    for (int i = 0; i < numSegments; i++) {
        float eta2 = eta1 + anglePerSegment;
        float sinEta2 = sin(eta2);
        float cosEta2 = cos(eta2);
        float e2x = cx + (a * cosTheta * cosEta2) - (b * sinTheta * sinEta2);
        float e2y = cy + (a * sinTheta * cosEta2) + (b * cosTheta * sinEta2);
        float ep2x = -a * cosTheta * sinEta2 - b * sinTheta * cosEta2;
        float ep2y = -a * sinTheta * sinEta2 + b * cosTheta * cosEta2;
        float tanDiff2 = tan((eta2 - eta1) / 2);
        float alpha = sin(eta2 - eta1) * (sqrt(4 + (3 * tanDiff2 * tanDiff2)) - 1) / 3;
        float q1x = e1x + alpha * ep1x;
        float q1y = e1y + alpha * ep1y;
        float q2x = e2x - alpha * ep2x;
        float q2y = e2y - alpha * ep2y;
        OH_Drawing_PathCubicTo(path, q1x, q1y, q2x, q2y, e2x, e2y);
        eta1 = eta2;
        e1x = e2x;
        e1y = e2y;
        ep1x = ep2x;
        ep1y = ep2y;
    }
}

void PathHarmonyImpl::AddPath(canvas::Path *path) {
    auto *path_impl = static_cast<PathHarmonyImpl *>(path);
    if (path_impl) {
        OH_Drawing_PathAddPathWithMode(path_, path_impl->GetPath(), PATH_ADD_MODE_APPEND);
    }
}

SrSVGBox PathHarmonyImpl::GetBounds() const {
    auto rect = OH_Drawing_RectCreate(0, 0, 0, 0);
    OH_Drawing_PathGetBounds(path_, rect);
    auto width = OH_Drawing_RectGetWidth(rect);
    auto height = OH_Drawing_RectGetHeight(rect);
    auto left = OH_Drawing_RectGetLeft(rect);
    auto top = OH_Drawing_RectGetTop(rect);
    SrSVGBox box{left, top, width, height};
    OH_Drawing_RectDestroy(rect);
    return box;
}

std::unique_ptr<canvas::Path> PathHarmonyImpl::CreateTransformCopy(const float (&xform)[6]) const {
    auto result = std::make_unique<PathHarmonyImpl>(*this);
    auto matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixSetMatrix(matrix, xform[0], xform[2], xform[4], xform[1], xform[3], xform[5], 0.f, 0.f, 1.f);
    OH_Drawing_PathTransform(result->GetPath(), matrix);
    OH_Drawing_MatrixDestroy(matrix);
    return std::move(result);
}

void PathHarmonyImpl::Transform(const float (&xform)[6]) {
    auto matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixSetMatrix(matrix, xform[0], xform[2], xform[4], xform[1], xform[3], xform[5], 0.f, 0.f, 1.f);
    OH_Drawing_PathTransform(path_, matrix);
    OH_Drawing_MatrixDestroy(matrix);
}

OH_Drawing_Path *PathHarmonyImpl::GetPath() const { return path_; }

void PathHarmonyImpl::SetFillType(SrSVGFillRule rule) {
    if (rule == SR_SVG_EO_FILL) {
        OH_Drawing_PathSetFillType(path_, PATH_FILL_TYPE_EVEN_ODD);
    } else {
        OH_Drawing_PathSetFillType(path_, PATH_FILL_TYPE_WINDING);
    }
}

PathHarmonyImpl::~PathHarmonyImpl() {
    if (path_) {
        OH_Drawing_PathDestroy(path_);
    }
}

} // namespace harmony
} // namespace svg
} // namespace serval
