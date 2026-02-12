// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/path_factory_harmony_impl.h"
#include "canvas/SrCanvas.h"
#include "platform/harmony/path_harmony_impl.h"
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>

namespace serval {
namespace svg {
namespace harmony {
std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateCircle(float cx, float cy, float r) {
    auto path = std::make_unique<PathHarmonyImpl>();
    OH_Drawing_PathAddCircle(path->GetPath(), cx, cy, r, PATH_DIRECTION_CW);
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateMutable() {
    auto path = std::make_unique<PathHarmonyImpl>();
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreatePath(uint8_t *ops, uint64_t n_ops, float *args,
                                                                 uint64_t n_args) {
    auto path = std::make_unique<PathHarmonyImpl>(ops, n_ops, args, n_args);
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateRect(float x, float y, float rx, float ry, float width,
                                                                 float height) {
    auto path = std::make_unique<PathHarmonyImpl>();
    OH_Drawing_PathMoveTo(path->GetPath(), x, y + ry);
    OH_Drawing_PathArcTo(path->GetPath(), x, y, x + 2 * rx, y + 2 * ry, 180, 90);
    OH_Drawing_PathLineTo(path->GetPath(), x + width - rx, y);
    OH_Drawing_PathArcTo(path->GetPath(), x + width - 2 * rx, y, x + width, y + 2 * ry, -90, 90);
    OH_Drawing_PathLineTo(path->GetPath(), x + width, y + height - ry);
    OH_Drawing_PathArcTo(path->GetPath(), x + width - 2 * rx, y + height - 2 * ry, x + width, y + height, 0, 90);
    OH_Drawing_PathLineTo(path->GetPath(), x + rx, y + height);
    OH_Drawing_PathArcTo(path->GetPath(), x, y + height - 2 * ry, x + 2 * rx, y + height, 90, 90);
    OH_Drawing_PathLineTo(path->GetPath(), x, y + ry);
    OH_Drawing_PathClose(path->GetPath());
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateLine(float start_x, float start_y, float end_x,
                                                                 float end_y) {
    auto path = std::make_unique<PathHarmonyImpl>();
    OH_Drawing_PathMoveTo(path->GetPath(), start_x, start_y);
    OH_Drawing_PathLineTo(path->GetPath(), end_x, end_y);
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateEllipse(float center_x, float center_y, float radius_x,
                                                                    float radius_y) {

    auto path = std::make_unique<PathHarmonyImpl>();
    auto rect =
        OH_Drawing_RectCreate(center_x - radius_x, center_y - radius_y, center_x + radius_x, center_y + radius_y);
    OH_Drawing_PathAddOval(path->GetPath(), rect, PATH_DIRECTION_CW);
    OH_Drawing_RectDestroy(rect);
    return std::move(path);
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreatePolygon(float points[], uint32_t n_points) {
    if (n_points > 1) {
        auto path = std::make_unique<PathHarmonyImpl>();
        std::vector<OH_Drawing_Point2D> polygon_points;
        for (int i = 0; i < n_points; i++) {
            OH_Drawing_Point2D point{points[2 * i], points[2 * i + 1]};
            polygon_points.emplace_back(point);
        }
        OH_Drawing_PathAddPolygon(path->GetPath(), polygon_points.data(), n_points, true);
        return std::move(path);
    }
    return nullptr;
}

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreatePolyline(float points[], uint32_t n_points) {
    if (n_points > 1) {
        auto path = std::make_unique<PathHarmonyImpl>();
        std::vector<OH_Drawing_Point2D> polygon_points;
        for (int i = 0; i < n_points; i++) {
            OH_Drawing_Point2D point{points[2 * i], points[2 * i + 1]};
            polygon_points.emplace_back(point);
        }
        OH_Drawing_PathAddPolygon(path->GetPath(), polygon_points.data(), n_points, false);
        return std::move(path);
    }
    return nullptr;
}

void PathFactoryHarmonyImpl::Op(canvas::Path *path1, canvas::Path *path2, canvas::OP type) {
    if (!path1 || !path2) {
        return;
    }
    auto *path_impl1 = static_cast<PathHarmonyImpl *>(path1);
    auto *path_impl2 = static_cast<PathHarmonyImpl *>(path2);
    auto op_mode = PATH_OP_MODE_DIFFERENCE;
    switch (type) {
    case canvas::DIFFERENCE:
        op_mode = PATH_OP_MODE_DIFFERENCE;
        break;
    case canvas::INTERSECT:
        op_mode = PATH_OP_MODE_INTERSECT;
        break;
    case canvas::UNION:
        op_mode = PATH_OP_MODE_UNION;
        break;
    case canvas::XOR:
        op_mode = PATH_OP_MODE_XOR;
        break;
    case canvas::REVERSE_DIFFERENCE:
        op_mode = PATH_OP_MODE_REVERSE_DIFFERENCE;
        break;
    default:
        break;
    }
    OH_Drawing_PathOp(path_impl1->GetPath(), path_impl2->GetPath(), op_mode);
};

std::unique_ptr<canvas::Path> PathFactoryHarmonyImpl::CreateStrokePath(const canvas::Path *path, float width,
                                                                       SrSVGStrokeCap cap, SrSVGStrokeJoin join,
                                                                       float miter_limit) {
    auto *harmony_path = static_cast<const PathHarmonyImpl *>(path);
    if (!harmony_path || !harmony_path->GetPath()) {
        return nullptr;
    }

    OH_Drawing_Pen *pen = OH_Drawing_PenCreate();
    OH_Drawing_PenSetWidth(pen, width);
    OH_Drawing_PenSetMiterLimit(pen, miter_limit);

    switch (cap) {
    case SR_SVG_STROKE_CAP_BUTT:
        OH_Drawing_PenSetCap(pen, LINE_FLAT_CAP);
        break;
    case SR_SVG_STROKE_CAP_ROUND:
        OH_Drawing_PenSetCap(pen, LINE_ROUND_CAP);
        break;
    case SR_SVG_STROKE_CAP_SQUARE:
        OH_Drawing_PenSetCap(pen, LINE_SQUARE_CAP);
        break;
    }

    switch (join) {
    case SR_SVG_STROKE_JOIN_MITER:
        OH_Drawing_PenSetJoin(pen, LINE_MITER_JOIN);
        break;
    case SR_SVG_STROKE_JOIN_ROUND:
        OH_Drawing_PenSetJoin(pen, LINE_ROUND_JOIN);
        break;
    case SR_SVG_STROKE_JOIN_BEVEL:
        OH_Drawing_PenSetJoin(pen, LINE_BEVEL_JOIN);
        break;
    }

    OH_Drawing_Path *dst_path = OH_Drawing_PathCreate();
    bool ret = OH_Drawing_PenGetFillPath(pen, harmony_path->GetPath(), dst_path, nullptr, nullptr);
    OH_Drawing_PenDestroy(pen);

    if (ret) {
        return std::make_unique<PathHarmonyImpl>(dst_path);
    }
    OH_Drawing_PathDestroy(dst_path);
    return nullptr;
}

} // namespace harmony
} // namespace svg
} // namespace serval
