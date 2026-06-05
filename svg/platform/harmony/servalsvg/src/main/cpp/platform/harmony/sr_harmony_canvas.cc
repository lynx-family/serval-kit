// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/sr_harmony_canvas.h"
#include "element/SrSVGNode.h"
#include "platform/harmony/path_harmony_impl.h"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGPatternUtils.h"
#include <deviceinfo.h>
#include <dlfcn.h>
#include <native_drawing/drawing_color_filter.h>
#include <native_drawing/drawing_filter.h>
#include <native_drawing/drawing_image_filter.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_path_effect.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>
#include <native_drawing/drawing_shader_effect.h>
namespace serval {
namespace svg {
namespace harmony {

constexpr int kHarmonyImageFilterOffsetMinApi = 20;
using CreateImageFilterOffsetProc = OH_Drawing_ImageFilter *(*)(float, float, OH_Drawing_ImageFilter *);

static inline CreateImageFilterOffsetProc ResolveImageFilterCreateOffset() {
    if (OH_GetSdkApiVersion() < kHarmonyImageFilterOffsetMinApi) {
        return nullptr;
    }
    static auto create_offset =
        reinterpret_cast<CreateImageFilterOffsetProc>(dlsym(RTLD_DEFAULT, "OH_Drawing_ImageFilterCreateOffset"));
    return create_offset;
}

static inline bool HasUnavailableHarmonyOffsetFilter(const canvas::SrFilterModel &filter) {
    // The common filter model treats feOffset as supported. Harmony exposes the
    // native offset image filter only from API 20, so gate non-zero offsets here.
    if (ResolveImageFilterCreateOffset()) {
        return false;
    }
    for (const auto &primitive : filter.primitives) {
        if (primitive.type == canvas::SrFilterPrimitiveType::kOffset && (primitive.dx != 0.f || primitive.dy != 0.f)) {
            return true;
        }
    }
    return false;
}

static inline void MultiplyTransformArray(const float (&lhs)[6], const float (&rhs)[6], float (&out)[6]) {
    out[0] = lhs[0] * rhs[0] + lhs[2] * rhs[1];
    out[1] = lhs[1] * rhs[0] + lhs[3] * rhs[1];
    out[2] = lhs[0] * rhs[2] + lhs[2] * rhs[3];
    out[3] = lhs[1] * rhs[2] + lhs[3] * rhs[3];
    out[4] = lhs[0] * rhs[4] + lhs[2] * rhs[5] + lhs[4];
    out[5] = lhs[1] * rhs[4] + lhs[3] * rhs[5] + lhs[5];
}

static inline void MultiplyTransformPointer(const float *lhs, const float (&rhs)[6], float (&out)[6]) {
    out[0] = lhs[0] * rhs[0] + lhs[2] * rhs[1];
    out[1] = lhs[1] * rhs[0] + lhs[3] * rhs[1];
    out[2] = lhs[0] * rhs[2] + lhs[2] * rhs[3];
    out[3] = lhs[1] * rhs[2] + lhs[3] * rhs[3];
    out[4] = lhs[0] * rhs[4] + lhs[2] * rhs[5] + lhs[4];
    out[5] = lhs[1] * rhs[4] + lhs[3] * rhs[5] + lhs[5];
}

static inline void ResolveObjectBoundingBoxTransform(const float (&form)[6], float left, float top, float width,
                                                     float height, float (&out)[6]) {
    if (width == 0.f || height == 0.f) {
        for (int i = 0; i < 6; i++) {
            out[i] = form[i];
        }
        return;
    }
    const float bboxToUser[6] = {width, 0.f, 0.f, height, left, top};
    const float userToBbox[6] = {1.f / width, 0.f, 0.f, 1.f / height, -left / width, -top / height};
    float temp[6];
    MultiplyTransformArray(bboxToUser, form, temp);
    MultiplyTransformArray(temp, userToBbox, out);
}

static inline void CopyTransformArray(const std::array<float, 6> &src, float (&out)[6]) {
    for (size_t i = 0; i < src.size(); ++i) {
        out[i] = src[i];
    }
}

OH_Drawing_ColorFilter *BuildHarmonyColorFilter(const canvas::SrFilterPrimitiveModel &primitive) {
    if (primitive.color_matrix_type == "luminanceToAlpha") {
        return OH_Drawing_ColorFilterCreateLuma();
    }
    if (primitive.color_matrix_values.size() != 20) {
        return nullptr;
    }
    return OH_Drawing_ColorFilterCreateMatrix(primitive.color_matrix_values.data());
}

bool SupportsHarmonyLinearSourceGraphicFilterModel(const canvas::SrFilterModel &filter) {
    const bool supports_linear_source_graphic = canvas::SrSupportsLinearSourceGraphicFilterModel(filter);
    if (!supports_linear_source_graphic || filter.region.width <= 0.f || filter.region.height <= 0.f) {
        return supports_linear_source_graphic;
    }
    return !HasUnavailableHarmonyOffsetFilter(filter);
}

OH_Drawing_ImageFilter *BuildHarmonyImageFilter(const canvas::SrFilterModel &filter,
                                                std::vector<OH_Drawing_ImageFilter *> *image_filters,
                                                std::vector<OH_Drawing_ColorFilter *> *color_filters) {
    OH_Drawing_ImageFilter *current_filter = nullptr;
    for (const auto &primitive : filter.primitives) {
        OH_Drawing_ImageFilter *next_filter = nullptr;
        switch (primitive.type) {
        case canvas::SrFilterPrimitiveType::kGaussianBlur:
            if (primitive.std_deviation_x <= 0.f && primitive.std_deviation_y <= 0.f) {
                continue;
            }
            next_filter = OH_Drawing_ImageFilterCreateBlur(primitive.std_deviation_x, primitive.std_deviation_y, CLAMP,
                                                           current_filter);
            break;
        case canvas::SrFilterPrimitiveType::kOffset:
            if (primitive.dx == 0.f && primitive.dy == 0.f) {
                continue;
            }
            if (auto create_offset = ResolveImageFilterCreateOffset()) {
                next_filter = create_offset(primitive.dx, primitive.dy, current_filter);
            } else {
                return nullptr;
            }
            break;
        case canvas::SrFilterPrimitiveType::kColorMatrix: {
            OH_Drawing_ColorFilter *color_filter = BuildHarmonyColorFilter(primitive);
            if (!color_filter) {
                return nullptr;
            }
            color_filters->push_back(color_filter);
            next_filter = OH_Drawing_ImageFilterCreateFromColorFilter(color_filter, current_filter);
            break;
        }
        case canvas::SrFilterPrimitiveType::kComposite:
        case canvas::SrFilterPrimitiveType::kBlend:
        case canvas::SrFilterPrimitiveType::kFlood:
            return nullptr;
        }
        if (next_filter) {
            image_filters->push_back(next_filter);
            current_filter = next_filter;
        }
    }
    return current_filter;
}

SrHarmonyCanvas::SrHarmonyCanvas(OH_Drawing_Canvas *context) {
    context_ = context;
    pen_ = OH_Drawing_PenCreate();
    brush_ = OH_Drawing_BrushCreate();
    image_sampling_ = OH_Drawing_SamplingOptionsCreate(FILTER_MODE_LINEAR, MIPMAP_MODE_NONE);
    path_factory_ = std::make_unique<harmony::PathFactoryHarmonyImpl>();
}
void SrHarmonyCanvas::Reset(OH_Drawing_Canvas *context) {
    context_ = context;
    lg_models_.clear();
    rg_models_.clear();
    current_transform_ = {1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
    transform_stack_.clear();
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
    if (image_sampling_) {
        OH_Drawing_SamplingOptionsDestroy(image_sampling_);
    }
}

canvas::PathFactory *SrHarmonyCanvas::PathFactory() { return path_factory_.get(); }

void SrHarmonyCanvas::Save() {
    OH_Drawing_CanvasSave(context_);
    transform_stack_.push_back(current_transform_);
}

void SrHarmonyCanvas::Restore() {
    OH_Drawing_CanvasRestore(context_);
    if (!transform_stack_.empty()) {
        current_transform_ = transform_stack_.back();
        transform_stack_.pop_back();
    }
}

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
                                const SrSVGPreserveAspectRatio &preserve_aspect_radio) {
    if (url == nullptr || image_provider_ == nullptr || width <= 0.f || height <= 0.f) {
        return;
    }
    const auto *image = image_provider_(std::string(url));
    if (image == nullptr || image->draw_pixel_map == nullptr || image->width == 0 || image->height == 0) {
        return;
    }

    float form[6];
    SrSVGBox view_port{x, y, width, height};
    SrSVGBox view_box{0.f, 0.f, static_cast<float>(image->width), static_cast<float>(image->height)};
    calculate_view_box_transform(&view_port, &view_box, preserve_aspect_radio, form);

    auto *src_rect =
        OH_Drawing_RectCreate(0.f, 0.f, static_cast<float>(image->width), static_cast<float>(image->height));
    auto *dst_rect =
        OH_Drawing_RectCreate(0.f, 0.f, static_cast<float>(image->width), static_cast<float>(image->height));
    auto *matrix = OH_Drawing_MatrixCreate();
    if (src_rect == nullptr || dst_rect == nullptr || matrix == nullptr) {
        if (matrix != nullptr) {
            OH_Drawing_MatrixDestroy(matrix);
        }
        if (src_rect != nullptr) {
            OH_Drawing_RectDestroy(src_rect);
        }
        if (dst_rect != nullptr) {
            OH_Drawing_RectDestroy(dst_rect);
        }
        return;
    }
    OH_Drawing_MatrixSetMatrix(matrix, form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f);

    OH_Drawing_CanvasSave(context_);
    OH_Drawing_CanvasConcatMatrix(context_, matrix);
    OH_Drawing_CanvasDrawPixelMapRect(context_, image->draw_pixel_map, src_rect, dst_rect, image_sampling_);
    OH_Drawing_CanvasRestore(context_);

    OH_Drawing_MatrixDestroy(matrix);
    OH_Drawing_RectDestroy(src_rect);
    OH_Drawing_RectDestroy(dst_rect);
}

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

void SrHarmonyCanvas::Translate(float x, float y) {
    OH_Drawing_CanvasTranslate(context_, x, y);
    float translation[6];
    float current[6] = {current_transform_[0], current_transform_[1], current_transform_[2],
                        current_transform_[3], current_transform_[4], current_transform_[5]};
    float out[6];
    xform_set_translation(translation, x, y);
    MultiplyTransformArray(current, translation, out);
    current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
}

void SrHarmonyCanvas::Transform(const float (&form)[6]) {
    auto matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixSetMatrix(matrix, form[0], form[2], form[4], form[1], form[3], form[5], 0.f, 0.f, 1.f);
    OH_Drawing_CanvasConcatMatrix(context_, matrix);
    OH_Drawing_MatrixDestroy(matrix);
    float current[6] = {current_transform_[0], current_transform_[1], current_transform_[2],
                        current_transform_[3], current_transform_[4], current_transform_[5]};
    float out[6];
    MultiplyTransformArray(current, form, out);
    current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
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

void SrHarmonyCanvas::ClipRect(float left, float top, float right, float bottom) {
    auto rect_path = path_factory_->CreateRect(left, top, 0.f, 0.f, right - left, bottom - top);
    if (rect_path) {
        ClipPath(rect_path.get(), SR_SVG_FILL);
    }
}

bool SrHarmonyCanvas::CalculatePathBounds(OH_Drawing_Path *path, SrSVGBox *bounds) {
    if (!path || !bounds) {
        return false;
    }
    auto rect = OH_Drawing_RectCreate(0, 0, 0, 0);
    OH_Drawing_PathGetBounds(path, rect);
    bounds->left = OH_Drawing_RectGetLeft(rect);
    bounds->top = OH_Drawing_RectGetTop(rect);
    bounds->width = OH_Drawing_RectGetWidth(rect);
    bounds->height = OH_Drawing_RectGetHeight(rect);
    OH_Drawing_RectDestroy(rect);
    return true;
}

void SrHarmonyCanvas::RenderPatternTiles(const element::ResolvedPattern &resolved_pattern,
                                         const SrSVGBox &target_bounds) {
    bool inserted = active_pattern_ids_.insert(resolved_pattern.id).second;

    SrSVGBox pattern_area = target_bounds;
    if (!element::IsIdentityTransform(resolved_pattern.pattern_transform)) {
        Transform(resolved_pattern.pattern_transform);
        float inverse[6];
        if (element::InvertAffineTransform(resolved_pattern.pattern_transform, inverse)) {
            pattern_area = element::MapBounds(target_bounds, inverse);
        }
    }

    float origin_x =
        resolved_pattern.x +
        std::floor((pattern_area.left - resolved_pattern.x) / resolved_pattern.width) * resolved_pattern.width;
    float origin_y =
        resolved_pattern.y +
        std::floor((pattern_area.top - resolved_pattern.y) / resolved_pattern.height) * resolved_pattern.height;
    float right = pattern_area.left + pattern_area.width;
    float bottom = pattern_area.top + pattern_area.height;
    const bool has_resolved_view_box = resolved_pattern.has_view_box &&
                                       FloatsLarger(resolved_pattern.view_box.width, 0.f) &&
                                       FloatsLarger(resolved_pattern.view_box.height, 0.f);
    const bool uses_object_bounding_box_content_units =
        resolved_pattern.pattern_content_units == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    SrSVGRenderContext base_tile_context = *current_render_context_;

    for (float step_y = origin_y; step_y < bottom; step_y += resolved_pattern.height) {
        for (float step_x = origin_x; step_x < right; step_x += resolved_pattern.width) {
            Save();
            ClipRect(step_x, step_y, step_x + resolved_pattern.width, step_y + resolved_pattern.height);

            SrSVGRenderContext tile_context = base_tile_context;
            if (has_resolved_view_box) {
                SrSVGBox tile_view_port{step_x, step_y, resolved_pattern.width, resolved_pattern.height};
                tile_context.view_port = tile_view_port;
                tile_context.view_box = resolved_pattern.view_box;
                float view_box_xform[6];
                calculate_view_box_transform(&tile_view_port, &resolved_pattern.view_box,
                                             resolved_pattern.preserve_aspect_ratio, view_box_xform);
                Transform(view_box_xform);
            } else {
                if (uses_object_bounding_box_content_units) {
                    tile_context.view_port = SrSVGBox{0.f, 0.f, 1.f, 1.f};
                    tile_context.view_box = SrSVGBox{0.f, 0.f, 0.f, 0.f};
                    Translate(step_x, step_y);
                    float scale_xform[6];
                    xform_set_scale(scale_xform, target_bounds.width, target_bounds.height);
                    Transform(scale_xform);
                } else {
                    tile_context.view_port = resolved_pattern.view_port;
                    tile_context.view_box = resolved_pattern.view_port;
                    Translate(step_x, step_y);
                }
            }

            auto *previous_render_context = current_render_context_;
            resolved_pattern.content_pattern->RenderContent(this, tile_context);
            current_render_context_ = previous_render_context;
            Restore();
        }
    }

    if (inserted) {
        active_pattern_ids_.erase(resolved_pattern.id);
    }
}

bool SrHarmonyCanvas::RenderPatternFill(OH_Drawing_Path *path, const SrSVGRenderState &render_state, const char *iri) {
    if (!current_render_context_ || !path || !iri || iri[0] != '#' ||
        !element::IsPatternIri(iri, *current_render_context_)) {
        return false;
    }
    SrSVGBox bounds{0.f, 0.f, 0.f, 0.f};
    if (!CalculatePathBounds(path, &bounds)) {
        return false;
    }
    element::ResolvedPattern resolved_pattern;
    if (!element::ResolvePatternFromIri(iri, *current_render_context_, bounds, active_pattern_ids_,
                                        &resolved_pattern)) {
        return false;
    }

    Save();
    auto clip_path = std::make_unique<PathHarmonyImpl>(OH_Drawing_PathCopy(path));
    ClipPath(clip_path.get(), render_state.fill_rule);
    RenderPatternTiles(resolved_pattern, bounds);
    Restore();
    return true;
}

bool SrHarmonyCanvas::RenderPatternStroke(OH_Drawing_Path *path, const SrSVGRenderState &render_state,
                                          const char *iri) {
    if (!current_render_context_ || !path || !iri || iri[0] != '#' || !render_state.stroke ||
        !FloatsLarger(render_state.stroke_width, 0.f) || !element::IsPatternIri(iri, *current_render_context_)) {
        return false;
    }

    SrSVGBox object_bounds{0.f, 0.f, 0.f, 0.f};
    if (!CalculatePathBounds(path, &object_bounds)) {
        return false;
    }
    element::ResolvedPattern resolved_pattern;
    if (!element::ResolvePatternFromIri(iri, *current_render_context_, object_bounds, active_pattern_ids_,
                                        &resolved_pattern)) {
        return false;
    }

    SrSVGStrokeCap stroke_line_cap = SR_SVG_STROKE_CAP_BUTT;
    SrSVGStrokeJoin stroke_line_join = SR_SVG_STROKE_JOIN_MITER;
    float stroke_miter_limit = element::SrSVGNode::s_stroke_miter_limit;
    float stroke_dash_offset = 0.f;
    float *dash_array = nullptr;
    size_t dash_array_length = 0;
    if (render_state.stroke_state) {
        stroke_line_cap = render_state.stroke_state->stroke_line_cap;
        stroke_line_join = render_state.stroke_state->stroke_line_join;
        stroke_miter_limit = render_state.stroke_state->stroke_miter_limit;
        stroke_dash_offset = render_state.stroke_state->stroke_dash_offset;
        dash_array = render_state.stroke_state->dash_array;
        dash_array_length = render_state.stroke_state->dash_array_length;
    }

    float current[6];
    float inverse[6];
    bool use_non_scaling_stroke = false;
    if (render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
        CopyTransformArray(current_transform_, current);
        use_non_scaling_stroke = element::InvertAffineTransform(current, inverse);
    }

    auto source_path = std::make_unique<PathHarmonyImpl>(OH_Drawing_PathCopy(path));
    std::unique_ptr<canvas::Path> transformed_path;
    canvas::Path *stroke_source_path = source_path.get();
    if (use_non_scaling_stroke) {
        transformed_path = source_path->CreateTransformCopy(current);
        stroke_source_path = transformed_path.get();
    }

    auto *path_factory = static_cast<PathFactoryHarmonyImpl *>(path_factory_.get());
    std::unique_ptr<canvas::Path> stroke_clip_path =
        path_factory->CreateStrokePath(stroke_source_path, render_state.stroke_width, stroke_line_cap, stroke_line_join,
                                       stroke_miter_limit, stroke_dash_offset, dash_array, dash_array_length);
    if (!stroke_clip_path) {
        return false;
    }

    SrSVGBox pattern_bounds = stroke_clip_path->GetBounds();
    if (use_non_scaling_stroke) {
        pattern_bounds = element::MapBounds(pattern_bounds, inverse);
    }

    Save();
    if (use_non_scaling_stroke) {
        Transform(inverse);
    }
    ClipPath(stroke_clip_path.get(), SR_SVG_FILL);
    if (use_non_scaling_stroke) {
        Transform(current);
    }
    RenderPatternTiles(resolved_pattern, pattern_bounds);
    Restore();
    return true;
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
        if (!iri) {
            Restore();
            return;
        }
        if (RenderPatternFill(path, render_state, iri)) {
            Restore();
            return;
        }
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
                                               bool is_stroke, OH_Drawing_Path *bounds_path,
                                               const float *extra_transform) {
    if (!canvas || !path || lg_model.stop_size() == 0) {
        return;
    }
    OH_Drawing_Path *geometry_path = bounds_path ? bounds_path : path;
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
        OH_Drawing_PathGetBounds(geometry_path, rect);
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
    const auto &form = lg_model.gradient_transformer_;
    float resolved_form[6];
    if (lg_model.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
        auto rect = OH_Drawing_RectCreate(0, 0, 0, 0);
        OH_Drawing_PathGetBounds(geometry_path, rect);
        auto width = OH_Drawing_RectGetWidth(rect);
        auto height = OH_Drawing_RectGetHeight(rect);
        auto left = OH_Drawing_RectGetLeft(rect);
        auto top = OH_Drawing_RectGetTop(rect);
        OH_Drawing_RectDestroy(rect);
        ResolveObjectBoundingBoxTransform(form, left, top, width, height, resolved_form);
    } else {
        for (size_t i = 0; i < 6; ++i) {
            resolved_form[i] = form[i];
        }
    }
    float combined_form[6];
    const float *matrix_form = resolved_form;
    if (extra_transform) {
        MultiplyTransformPointer(extra_transform, resolved_form, combined_form);
        matrix_form = combined_form;
    }
    OH_Drawing_MatrixSetMatrix(transform, matrix_form[0], matrix_form[2], matrix_form[4], matrix_form[1],
                               matrix_form[3], matrix_form[5], 0.f, 0.f, 1.f);

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
                                               bool is_stroke, OH_Drawing_Path *bounds_path,
                                               const float *extra_transform) {
    if (!canvas || !path || rg_model.stop_size() == 0) {
        return;
    }
    Save();
    OH_Drawing_Path *geometry_path = bounds_path ? bounds_path : path;
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
    OH_Drawing_PathGetBounds(geometry_path, rect);
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
    const auto &form = rg_model.gradient_transformer_;
    float resolved_form[6];
    if (rg_model.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
        auto max_size = std::max(width, height);
        ResolveObjectBoundingBoxTransform(form, left, top, max_size, max_size, resolved_form);
    } else {
        for (size_t i = 0; i < 6; ++i) {
            resolved_form[i] = form[i];
        }
    }
    float combined_form[6];
    const float *matrix_form = resolved_form;
    if (extra_transform) {
        MultiplyTransformPointer(extra_transform, resolved_form, combined_form);
        matrix_form = combined_form;
    }
    OH_Drawing_MatrixSetMatrix(transform, matrix_form[0], matrix_form[2], matrix_form[4], matrix_form[1],
                               matrix_form[3], matrix_form[5], 0.f, 0.f, 1.f);

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
        OH_Drawing_Path *stroke_path = path;
        std::unique_ptr<canvas::Path> transformed_path;
        if (render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
            float current[6] = {current_transform_[0], current_transform_[1], current_transform_[2],
                                current_transform_[3], current_transform_[4], current_transform_[5]};
            float inverse[6];
            if (element::InvertAffineTransform(current, inverse)) {
                auto source_path = std::make_unique<PathHarmonyImpl>(OH_Drawing_PathCopy(path));
                transformed_path = source_path->CreateTransformCopy(current);
                stroke_path = static_cast<PathHarmonyImpl *>(transformed_path.get())->GetPath();
                Transform(inverse);
            }
        }
        OH_Drawing_PenSetColor(pen_, render_state.stroke->content.color.color);
        if (FloatsNotEqual(render_state.stroke_opacity, 1)) {
            OH_Drawing_PenSetAlpha(pen_, ConvertAlpha(render_state.stroke_opacity));
        }
        OH_Drawing_CanvasAttachPen(context_, pen_);
        OH_Drawing_CanvasDrawPath(context_, stroke_path);
        OH_Drawing_CanvasDetachPen(context_);
    } else if (render_state.stroke && render_state.stroke->type == SERVAL_PAINT_IRI) {
        const char *iri = render_state.stroke->content.iri;
        if (!iri) {
            Restore();
            return;
        }
        if (RenderPatternStroke(path, render_state, iri)) {
            Restore();
            return;
        }
        auto it1 = lg_models_.find(iri);
        if (it1 != lg_models_.end()) {
            const canvas::LinearGradientModel &lg_model = it1->second;
            if (render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
                float current[6];
                CopyTransformArray(current_transform_, current);
                float inverse[6];
                if (element::InvertAffineTransform(current, inverse)) {
                    auto source_path = std::make_unique<PathHarmonyImpl>(OH_Drawing_PathCopy(path));
                    auto transformed_path = source_path->CreateTransformCopy(current);
                    OH_Drawing_Path *stroke_path = static_cast<PathHarmonyImpl *>(transformed_path.get())->GetPath();
                    Transform(inverse);
                    DrawLinearGradientShader(context_, lg_model, stroke_path, render_state, true, path, current);
                } else {
                    DrawLinearGradientShader(context_, lg_model, path, render_state, true);
                }
            } else {
                DrawLinearGradientShader(context_, lg_model, path, render_state, true);
            }
        }
        auto it2 = rg_models_.find(iri);
        if (it2 != rg_models_.end()) {
            const canvas::RadialGradientModel &rgModel = it2->second;
            if (render_state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE) {
                float current[6];
                CopyTransformArray(current_transform_, current);
                float inverse[6];
                if (element::InvertAffineTransform(current, inverse)) {
                    auto source_path = std::make_unique<PathHarmonyImpl>(OH_Drawing_PathCopy(path));
                    auto transformed_path = source_path->CreateTransformCopy(current);
                    OH_Drawing_Path *stroke_path = static_cast<PathHarmonyImpl *>(transformed_path.get())->GetPath();
                    Transform(inverse);
                    DrawRadialGradientShader(context_, rgModel, stroke_path, render_state, true, path, current);
                } else {
                    DrawRadialGradientShader(context_, rgModel, path, render_state, true);
                }
            } else {
                DrawRadialGradientShader(context_, rgModel, path, render_state, true);
            }
        }
    }
    Restore();
}

void SrHarmonyCanvas::SaveLayer(const SrSVGBox *bounds) {
    // OH_Drawing_CanvasSaveLayer creates an off-screen compositing layer.
    // All drawing until RestoreLayer() goes to this temporary buffer.
    OH_Drawing_Rect *rect = nullptr;
    if (bounds) {
        rect = OH_Drawing_RectCreate(bounds->left, bounds->top, bounds->left + bounds->width,
                                     bounds->top + bounds->height);
    }
    OH_Drawing_CanvasSaveLayer(context_, rect, nullptr);
    transform_stack_.push_back(current_transform_);
    if (rect) {
        OH_Drawing_RectDestroy(rect);
    }
}

void SrHarmonyCanvas::RestoreLayer() {
    OH_Drawing_CanvasRestore(context_);
    if (!transform_stack_.empty()) {
        current_transform_ = transform_stack_.back();
        transform_stack_.pop_back();
    }
}

bool SrHarmonyCanvas::SupportsFilterModel(const canvas::SrFilterModel &filter) const {
    return SupportsHarmonyLinearSourceGraphicFilterModel(filter);
}

void SrHarmonyCanvas::BeginFilterLayer(const SrSVGBox *bounds, const canvas::SrFilterModel &filter) {
    std::vector<OH_Drawing_ImageFilter *> image_filters;
    std::vector<OH_Drawing_ColorFilter *> color_filters;
    OH_Drawing_ImageFilter *image_filter = BuildHarmonyImageFilter(filter, &image_filters, &color_filters);
    if (!image_filter) {
        OH_Drawing_Rect *rect = nullptr;
        if (bounds) {
            rect = OH_Drawing_RectCreate(bounds->left, bounds->top, bounds->left + bounds->width,
                                         bounds->top + bounds->height);
        }
        OH_Drawing_CanvasSaveLayer(context_, rect, nullptr);
        transform_stack_.push_back(current_transform_);
        if (rect) {
            OH_Drawing_RectDestroy(rect);
        }
        return;
    }

    OH_Drawing_Rect *rect = nullptr;
    if (bounds) {
        rect = OH_Drawing_RectCreate(bounds->left, bounds->top, bounds->left + bounds->width,
                                     bounds->top + bounds->height);
    }
    OH_Drawing_Filter *drawing_filter = OH_Drawing_FilterCreate();
    OH_Drawing_Brush *filter_brush = OH_Drawing_BrushCreate();
    OH_Drawing_FilterSetImageFilter(drawing_filter, image_filter);
    OH_Drawing_BrushSetFilter(filter_brush, drawing_filter);
    OH_Drawing_CanvasSaveLayer(context_, rect, filter_brush);
    transform_stack_.push_back(current_transform_);

    if (rect) {
        OH_Drawing_RectDestroy(rect);
    }
    OH_Drawing_BrushDestroy(filter_brush);
    OH_Drawing_FilterDestroy(drawing_filter);
    for (auto *created_filter : image_filters) {
        OH_Drawing_ImageFilterDestroy(created_filter);
    }
    for (auto *created_filter : color_filters) {
        OH_Drawing_ColorFilterDestroy(created_filter);
    }
}

void SrHarmonyCanvas::EndFilterLayer() { RestoreLayer(); }

void SrHarmonyCanvas::BeginMaskLayer(const SrSVGBox *bounds, bool is_luminance) {
    mask_is_luminance_ = is_luminance;
    SaveLayer(bounds);
}

void SrHarmonyCanvas::BeginMaskContentLayer() {
    if (mask_content_layer_active_) {
        return;
    }
    OH_Drawing_Brush *blend_brush = OH_Drawing_BrushCreate();
    OH_Drawing_Filter *blend_filter = nullptr;
    OH_Drawing_ColorFilter *color_filter = nullptr;
    OH_Drawing_BrushSetBlendMode(blend_brush, BLEND_MODE_DST_IN);
    if (mask_is_luminance_) {
        blend_filter = OH_Drawing_FilterCreate();
        color_filter = OH_Drawing_ColorFilterCreateLuma();
        OH_Drawing_FilterSetColorFilter(blend_filter, color_filter);
        OH_Drawing_BrushSetFilter(blend_brush, blend_filter);
    }
    OH_Drawing_CanvasSaveLayer(context_, nullptr, blend_brush);
    if (color_filter) {
        OH_Drawing_ColorFilterDestroy(color_filter);
    }
    if (blend_filter) {
        OH_Drawing_FilterDestroy(blend_filter);
    }
    OH_Drawing_BrushDestroy(blend_brush);
    mask_content_layer_active_ = true;
}

void SrHarmonyCanvas::EndMaskContentLayer() {
    if (mask_content_layer_active_) {
        OH_Drawing_CanvasRestore(context_);
        mask_content_layer_active_ = false;
    }
}

void SrHarmonyCanvas::EndMaskLayer() {
    if (mask_content_layer_active_) {
        EndMaskContentLayer();
    }
    RestoreLayer();
    mask_is_luminance_ = false;
}

void SrHarmonyCanvas::SetAntiAlias(bool anti_alias) { anti_alias_ = anti_alias; }

}  // namespace harmony
}  // namespace svg
}  // namespace serval
