// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/path_harmony_impl.h"
#include "utils/SrSVGArc.h"
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

void PathHarmonyImpl::SrSVGDrawArc(OH_Drawing_Path *path, float x, float y, float x1, float y1, float a, float b,
                                   float theta, bool large_arc, bool sweep) {
    const float args[9] = {x, y, a, b, theta, (float)large_arc, (float)sweep, x1, y1};
    SrSVGArc arc;
    if (!SrSVGArcToCubics(args, &arc))
        return;
    if (arc.line)
        OH_Drawing_PathLineTo(path, x1, y1);
    for (size_t i = 0; i < arc.count; ++i) {
        const double *c = arc.cubics[i];
        OH_Drawing_PathCubicTo(path, c[0], c[1], c[2], c[3], c[4], c[5]);
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

}  // namespace harmony
}  // namespace svg
}  // namespace serval
