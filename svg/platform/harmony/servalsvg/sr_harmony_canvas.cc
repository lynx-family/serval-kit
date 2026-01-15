// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/sr_harmony_canvas.h"
#include "platform/harmony/path_harmony_impl.h"
#include "utils/SrFloatComparison.h"
#include <native_drawing/drawing_color_filter.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_path_effect.h>
#include <native_drawing/drawing_shader_effect.h>
namespace serval {
namespace svg {
namespace harmony {

SrHarmonyCanvas::SrHarmonyCanvas(OH_Drawing_Canvas *context) {
    context_ = context;
    pen_ = OH_Drawing_PenCreate();
    brush_ = OH_Drawing_BrushCreate();
    path_factory_ = std::make_unique<harmony::PathFactoryHarmonyImpl>();
}
void SrHarmonyCanvas::Reset(OH_Drawing_Canvas *context) {
    context_ = context;
    lg_models_.clear();
    rg_models_.clear();
}

SrHarmonyCanvas::~SrHarmonyCanvas() {
    if (pen_) {
        OH_Drawing_PenDestroy(pen_);
    }
    if (brush_) {
        OH_Drawing_BrushDestroy(brush_);
    }
    if (shader_) {
        OH_Drawing_ShaderEffectDestroy(shader_);
    }
    if (path_effect_) {
        OH_Drawing_PathEffectDestroy(path_effect_);
    }
}

canvas::PathFactory *SrHarmonyCanvas::PathFactory() { return path_factory_.get(); }

void SrHarmonyCanvas::Save() { OH_Drawing_CanvasSave(context_); }

void SrHarmonyCanvas::Restore() { OH_Drawing_CanvasRestore(context_); }

void SrHarmonyCanvas::DrawLine(const char *, float x1, float y1, float x2, float y2,
                               const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreateLine(x1, y1, x2, y2);
    auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
    FillPath(path_impl->GetPath(), render_state);
    StrokePath(path_impl->GetPath(), render_state);
    Restore();
}

uint8_t static inline ConvertAlpha(float alpha) {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return static_cast<uint8_t>(std::round(alpha * 255.0f));
}

void SrHarmonyCanvas::DrawRect(const char *id, float x, float y, float rx, float ry, float width, float height,
                               const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreateRect(x, y, rx, ry, width, height);
    auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
    FillPath(path_impl->GetPath(), render_state);
    StrokePath(path_impl->GetPath(), render_state);
    Restore();
}

void SrHarmonyCanvas::DrawCircle(const char *, float cx, float cy, float r, const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreateCircle(cx, cy, r);
    auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
    FillPath(path_impl->GetPath(), render_state);
    StrokePath(path_impl->GetPath(), render_state);
    Restore();
}

void SrHarmonyCanvas::DrawPolygon(const char *, float *points, uint32_t n_points,
                                  const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreatePolygon(points, n_points);
    if (path) {
        auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
        FillPath(path_impl->GetPath(), render_state);
        StrokePath(path_impl->GetPath(), render_state);
    }
    Restore();
}

void SrHarmonyCanvas::DrawPolyline(const char *, float *points, uint32_t n_points,
                                   const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreatePolyline(points, n_points);
    if (path) {
        auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
        FillPath(path_impl->GetPath(), render_state);
        StrokePath(path_impl->GetPath(), render_state);
    }
    Restore();
}

void SrHarmonyCanvas::DrawEllipse(const char *, float center_x, float center_y, float radius_x, float radius_y,
                                  const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreateEllipse(center_x, center_y, radius_x, radius_y);
    auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
    FillPath(path_impl->GetPath(), render_state);
    StrokePath(path_impl->GetPath(), render_state);
    Restore();
}

void SrHarmonyCanvas::DrawPath(const char *, uint8_t *ops, uint32_t n_ops, float *args, uint32_t n_args,
                               const SrSVGRenderState &render_state) {
    Save();
    auto path = path_factory_->CreatePath(ops, n_ops, args, n_args);
    auto path_impl = static_cast<PathHarmonyImpl *>(path.get());
    FillPath(path_impl->GetPath(), render_state);
    StrokePath(path_impl->GetPath(), render_state);
    Restore();
}

void SrHarmonyCanvas::DrawUse(const char *href, float x, float y, float width, float height) {}

void SrHarmonyCanvas::DrawImage(const char *url, float x, float y, float width, float height,
                                const SrSVGPreserveAspectRatio &preserve_aspect_radio) {}

void SrHarmonyCanvas::SetViewBox(float x, float y, float width, float height) {}

void SrHarmonyCanvas::UpdateLinearGradient(const char *id, const float (&gradient_transform)[6],
                                           const GradientSpread spread, float x1, float x2, float y1, float y2,
                                           const std::vector<SrStop> &stops,
                                           SrSVGObjectBoundingBoxUnitType bounding_box_type) {
    if (strlen(id)) {
        lg_models_[std::string("#") + id] =
            canvas::LinearGradientModel(spread, x1, x2, y1, y2, gradient_transform, stops, bounding_box_type);
    }
}

void SrHarmonyCanvas::UpdateRadialGradient(const char *id, const float (&gradient_transform)[6],
                                           const GradientSpread spread, float cx, float cy, float fr, float fx,
                                           float fy, const std::vector<SrStop> &stops,
                                           SrSVGObjectBoundingBoxUnitType bounding_box_type) {
    if (strlen(id)) {
        rg_models_[std::string("#") + id] =
            canvas::RadialGradientModel(spread, cx, cy, fr, fx, fy, gradient_transform, stops, bounding_box_type);
    }
}

void SrHarmonyCanvas::Translate(float x, float y) { OH_Drawing_CanvasTranslate(context_, x, y); }

void SrHarmonyCanvas::Transform(const float (&form)[6]) {
    auto matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixSetMatrix(matrix, form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f);
    OH_Drawing_CanvasConcatMatrix(context_, matrix);
    OH_Drawing_MatrixDestroy(matrix);
}

void SrHarmonyCanvas::ClipPath(canvas::Path *path, SrSVGFillRule clip_rule) {
    auto *path_impl = static_cast<PathHarmonyImpl *>(path);
    if (path_impl) {
        if (clip_rule == SR_SVG_EO_FILL) {
            OH_Drawing_PathSetFillType(path_impl->GetPath(), PATH_FILL_TYPE_EVEN_ODD);
        }
        OH_Drawing_CanvasClipPath(context_, path_impl->GetPath(), INTERSECT, true);
    }
}

void SrHarmonyCanvas::InitStrokePaint(const SrSVGRenderState &render_state, bool anti_alias) {
    OH_Drawing_PenReset(pen_);
    OH_Drawing_PenSetAntiAlias(pen_, anti_alias);
    if (FloatsLarger(render_state.stroke_width, 0)) {
        OH_Drawing_PenSetWidth(pen_, render_state.stroke_width);
    }
    if (render_state.stroke_state) {
        auto stoke_cap = render_state.stroke_state->stroke_line_cap;
        switch (stoke_cap) {
        case SR_SVG_STROKE_CAP_BUTT:
            OH_Drawing_PenSetCap(pen_, LINE_FLAT_CAP);
            break;
        case SR_SVG_STROKE_CAP_ROUND:
            OH_Drawing_PenSetCap(pen_, LINE_ROUND_CAP);
            break;
        case SR_SVG_STROKE_CAP_SQUARE:
            OH_Drawing_PenSetCap(pen_, LINE_SQUARE_CAP);
            break;
        }
        auto stroke_join = render_state.stroke_state->stroke_line_join;
        switch (stroke_join) {
        case SR_SVG_STROKE_JOIN_MITER:
            OH_Drawing_PenSetJoin(pen_, LINE_MITER_JOIN);
            break;
        case SR_SVG_STROKE_JOIN_ROUND:
            OH_Drawing_PenSetJoin(pen_, LINE_ROUND_JOIN);
            break;
        case SR_SVG_STROKE_JOIN_BEVEL:
            OH_Drawing_PenSetJoin(pen_, LINE_BEVEL_JOIN);
            break;
        }
        auto stroke_miter = render_state.stroke_state->stroke_miter_limit;
        OH_Drawing_PenSetMiterLimit(pen_, stroke_miter);
        if (render_state.stroke_state->dash_array && render_state.stroke_state->dash_array_length > 0) {
            auto n = render_state.stroke_state->dash_array_length;
            auto dash_array = render_state.stroke_state->dash_array;
            auto length = (n % 2 == 0) ? n : n * 2;
            float interval_sum = 0.f;
            std::vector<float> intervals(length, 0);
            for (size_t i = 0; i < length; i++) {
                intervals[i] = dash_array[i % n];
                interval_sum += intervals[i];
            }
            if (FloatsEqual(interval_sum, 0)) {
                OH_Drawing_PenSetPathEffect(pen_, nullptr);
            } else {
                if (path_effect_) {
                    OH_Drawing_PathEffectDestroy(path_effect_);
                    path_effect_ = nullptr;
                }
                float offset = render_state.stroke_state->stroke_dash_offset;
                if (FloatLess(offset, 0)) {
                    offset = interval_sum + std::fmod(offset, interval_sum);
                }
                path_effect_ = OH_Drawing_CreateDashPathEffect(intervals.data(), intervals.size(), offset);
                OH_Drawing_PenSetPathEffect(pen_, path_effect_);
            }
        }
    }
}

void SrHarmonyCanvas::InitFillPaint(const SrSVGRenderState &render_state, bool anti_alias) {
    OH_Drawing_BrushReset(brush_);
    OH_Drawing_BrushSetAntiAlias(brush_, anti_alias);
}

void SrHarmonyCanvas::FillPath(OH_Drawing_Path *path, const SrSVGRenderState &render_state) {
    Save();
    InitFillPaint(render_state, anti_alias_);
    if (render_state.fill_rule == SR_SVG_EO_FILL) {
        OH_Drawing_PathSetFillType(path, PATH_FILL_TYPE_EVEN_ODD);
    } else {
        OH_Drawing_PathSetFillType(path, PATH_FILL_TYPE_WINDING);
    }
    if (!render_state.fill) {
        OH_Drawing_BrushSetColor(brush_, NSVG_RGB(0, 0, 0));
        if (FloatsNotEqual(render_state.fill_opacity, 1)) {
            OH_Drawing_BrushSetAlpha(brush_, ConvertAlpha(render_state.fill_opacity));
        }
        OH_Drawing_CanvasAttachBrush(context_, brush_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachBrush(context_);
    } else if (render_state.fill && render_state.fill->type == SERVAL_PAINT_COLOR) {
        OH_Drawing_BrushSetColor(brush_, render_state.fill->content.color.color);
        if (FloatsNotEqual(render_state.fill_opacity, 1)) {
            OH_Drawing_BrushSetAlpha(brush_, ConvertAlpha(render_state.fill_opacity));
        }
        OH_Drawing_CanvasAttachBrush(context_, brush_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachBrush(context_);
    } else if (render_state.fill && render_state.fill->type == SERVAL_PAINT_IRI) {
        const char *iri = render_state.fill->content.iri;
        auto it1 = lg_models_.find(iri);
        if (it1 != lg_models_.end()) {
            const canvas::LinearGradientModel &lgModel = it1->second;
            DrawLinearGradientShader(context_, lgModel, path, render_state, false);
        }
        auto it2 = rg_models_.find(iri);
        if (it2 != rg_models_.end()) {
            const canvas::RadialGradientModel &rgModel = it2->second;
            DrawRadialGradientShader(context_, rgModel, path, render_state, false);
        }
    }
    Restore();
}

static inline uint32_t MixColorWithOpacity(uint32_t color, float opacity) {
    auto alpha = static_cast<int32_t>(static_cast<float>(((color >> 24) & 0xff)) * opacity);
    alpha = std::max(0, std::min(alpha, 255));
    return static_cast<uint32_t>((alpha << 24) | (color & 0xffffff));
}

void SrHarmonyCanvas::DrawLinearGradientShader(OH_Drawing_Canvas *canvas, const canvas::LinearGradientModel &lg_model,
                                               OH_Drawing_Path *path, const SrSVGRenderState &render_state,
                                               bool is_stroke) {
    if (!canvas || !path || lg_model.stop_size() == 0) {
        return;
    }
    size_t stopSize = lg_model.stops_.size();
    std::vector<float> offsets;
    std::vector<uint32_t> colors;
    float lastOffset = -1.f;
    for (size_t i = 0; i < stopSize; ++i) {
        const SrStop &stop = lg_model.stops_[i];
        // prepare position
        float offset = stop.offset.value;
        if (i == 0 || FloatsLargerOrEqual(offset, lastOffset)) {
            offsets.emplace_back(offset);
            lastOffset = offset;
        } else {
            offsets.push_back(lastOffset);
        }
        colors.emplace_back(MixColorWithOpacity(stop.stopColor.color, stop.stopOpacity.value));
    }

    float x1 = lg_model.x1_;
    float y1 = lg_model.y1_;
    float x2 = lg_model.x2_;
    float y2 = lg_model.y2_;
    if (lg_model.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
        auto rect = OH_Drawing_RectCreate(0, 0, 0, 0);
        OH_Drawing_PathGetBounds(path, rect);
        auto width = OH_Drawing_RectGetWidth(rect);
        auto height = OH_Drawing_RectGetHeight(rect);
        auto left = OH_Drawing_RectGetLeft(rect);
        auto top = OH_Drawing_RectGetTop(rect);
        x1 = left + x1 * width;
        y1 = top + y1 * height;
        x2 = left + x2 * width;
        y2 = top + y2 * height;
        OH_Drawing_RectDestroy(rect);
    }
    // TODO(chengjunnan) impl transform later
    OH_Drawing_Point2D start_point{x1, y1};
    OH_Drawing_Point2D end_point{x2, y2};
    OH_Drawing_TileMode mode = CLAMP;
    if (lg_model.spread_mode_ == reflect) {
        mode = MIRROR;
    } else if (lg_model.spread_mode_ == repeat) {
        mode = REPEAT;
    }
    auto transform = OH_Drawing_MatrixCreate();
    auto form = lg_model.gradient_transformer_;
    OH_Drawing_MatrixSetMatrix(transform, form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f);

    if (shader_) {
        OH_Drawing_ShaderEffectDestroy(shader_);
        shader_ = nullptr;
    }
    shader_ = OH_Drawing_ShaderEffectCreateLinearGradientWithLocalMatrix(
        &start_point, &end_point, colors.data(), offsets.data(), colors.size(), mode, transform);
    OH_Drawing_MatrixDestroy(transform);
    Save();
    if (is_stroke) {
        InitStrokePaint(render_state, anti_alias_);
        if (FloatsNotEqual(render_state.stroke_opacity, 1)) {
            OH_Drawing_PenSetAlpha(pen_, ConvertAlpha(render_state.stroke_opacity));
        }
        OH_Drawing_PenSetShaderEffect(pen_, shader_);
        OH_Drawing_CanvasAttachPen(context_, pen_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachPen(context_);
    } else {
        InitFillPaint(render_state, anti_alias_);
        if (FloatsNotEqual(render_state.fill_opacity, 1)) {
            OH_Drawing_BrushSetAlpha(brush_, ConvertAlpha(render_state.fill_opacity));
        }
        OH_Drawing_BrushSetShaderEffect(brush_, shader_);
        OH_Drawing_CanvasAttachBrush(context_, brush_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachBrush(context_);
    }
    Restore();
}

void SrHarmonyCanvas::DrawRadialGradientShader(OH_Drawing_Canvas *canvas, const canvas::RadialGradientModel &rg_model,
                                               OH_Drawing_Path *path, const SrSVGRenderState &render_state,
                                               bool is_stroke) {
    if (!canvas || !path || rg_model.stop_size() == 0) {
        return;
    }
    Save();
    size_t stopSize = rg_model.stops_.size();
    std::vector<float> offsets;
    std::vector<uint32_t> colors;
    float lastOffset = -1.f;
    for (size_t i = 0; i < stopSize; ++i) {
        const SrStop &stop = rg_model.stops_[i];
        // prepare position
        float offset = stop.offset.value;
        if (i == 0 || FloatsLargerOrEqual(offset, lastOffset)) {
            offsets.emplace_back(offset);
            lastOffset = offset;
        } else {
            offsets.push_back(lastOffset);
        }
        colors.emplace_back(MixColorWithOpacity(stop.stopColor.color, stop.stopOpacity.value));
    }
    auto rect = OH_Drawing_RectCreate(0, 0, 0, 0);
    OH_Drawing_PathGetBounds(path, rect);
    auto width = OH_Drawing_RectGetWidth(rect);
    auto height = OH_Drawing_RectGetHeight(rect);
    auto left = OH_Drawing_RectGetLeft(rect);
    auto top = OH_Drawing_RectGetTop(rect);
    OH_Drawing_RectDestroy(rect);
    OH_Drawing_Point2D startCenter{rg_model.fx_, rg_model.fy_};
    OH_Drawing_Point2D endCenter{rg_model.cx_, rg_model.cy_};
    float startRadius = 0.f;
    float endRadius = rg_model.r_;
    if (rg_model.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
        auto max_size = std::max(width, height);
        startRadius = 0.0;
        endRadius = rg_model.r_ * max_size;
        startCenter = {left + rg_model.fx_ * max_size, top + rg_model.fy_ * max_size};
        endCenter = {left + rg_model.cx_ * max_size, top + rg_model.cy_ * max_size};
    }
    OH_Drawing_Matrix *matrix{nullptr};
    if (FloatsLarger(width, height)) {
        matrix = OH_Drawing_MatrixCreateScale(1.f, height / width, left, top);
    } else {
        matrix = OH_Drawing_MatrixCreateScale(width / height, 1.f, left, top);
    }
    if (shader_) {
        OH_Drawing_ShaderEffectDestroy(shader_);
        shader_ = nullptr;
    }
    OH_Drawing_TileMode mode = CLAMP;
    if (rg_model.spread_mode_ == reflect) {
        mode = MIRROR;
    } else if (rg_model.spread_mode_ == repeat) {
        mode = REPEAT;
    }
    auto transform = OH_Drawing_MatrixCreate();
    auto form = rg_model.gradient_transformer_;
    OH_Drawing_MatrixSetMatrix(transform, form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f);

    OH_Drawing_MatrixConcat(matrix, matrix, transform);
    shader_ = OH_Drawing_ShaderEffectCreateTwoPointConicalGradient(
        &startCenter, startRadius, &endCenter, endRadius, colors.data(), offsets.data(), colors.size(), mode, matrix);
    if (is_stroke) {
        InitStrokePaint(render_state, anti_alias_);
        if (FloatsNotEqual(render_state.stroke_opacity, 1)) {
            OH_Drawing_PenSetAlpha(pen_, ConvertAlpha(render_state.stroke_opacity));
        }
        OH_Drawing_PenSetShaderEffect(pen_, shader_);
        OH_Drawing_CanvasAttachPen(context_, pen_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachPen(context_);
    } else {
        InitFillPaint(render_state, anti_alias_);
        if (FloatsNotEqual(render_state.fill_opacity, 1)) {
            OH_Drawing_BrushSetAlpha(brush_, ConvertAlpha(render_state.fill_opacity));
        }
        OH_Drawing_BrushSetShaderEffect(brush_, shader_);
        OH_Drawing_CanvasAttachBrush(context_, brush_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachBrush(context_);
    }

    OH_Drawing_MatrixDestroy(transform);
    OH_Drawing_MatrixDestroy(matrix);
    Restore();
}

void SrHarmonyCanvas::StrokePath(OH_Drawing_Path *path, const SrSVGRenderState &render_state) {
    Save();
    InitStrokePaint(render_state, anti_alias_);
    if (render_state.fill_rule == SR_SVG_EO_FILL) {
        OH_Drawing_PathSetFillType(path, PATH_FILL_TYPE_EVEN_ODD);
    } else {
        OH_Drawing_PathSetFillType(path, PATH_FILL_TYPE_WINDING);
    }
    if (render_state.stroke && render_state.stroke->type == SERVAL_PAINT_COLOR) {
        OH_Drawing_PenSetColor(pen_, render_state.stroke->content.color.color);
        if (FloatsNotEqual(render_state.stroke_opacity, 1)) {
            OH_Drawing_PenSetAlpha(pen_, ConvertAlpha(render_state.stroke_opacity));
        }
        OH_Drawing_CanvasAttachPen(context_, pen_);
        OH_Drawing_CanvasDrawPath(context_, path);
        OH_Drawing_CanvasDetachPen(context_);
    } else if (render_state.stroke && render_state.stroke->type == SERVAL_PAINT_IRI) {
        const char *iri = render_state.stroke->content.iri;
        auto it1 = lg_models_.find(iri);
        if (it1 != lg_models_.end()) {
            const canvas::LinearGradientModel &lg_model = it1->second;
            DrawLinearGradientShader(context_, lg_model, path, render_state, true);
        }
        auto it2 = rg_models_.find(iri);
        if (it2 != rg_models_.end()) {
            const canvas::RadialGradientModel &rgModel = it2->second;
            DrawRadialGradientShader(context_, rgModel, path, render_state, true);
        }
    }
    Restore();
}

void SrHarmonyCanvas::SetAntiAlias(bool anti_alias) { anti_alias_ = anti_alias; }

} // namespace harmony
} // namespace svg
} // namespace serval
