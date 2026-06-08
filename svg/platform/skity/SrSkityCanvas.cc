// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/skity/SrSkityCanvas.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include "element/SrSVGNode.h"
#include "element/SrSVGPattern.h"
#include "skity/effect/path_effect.hpp"
#include "skity/geometry/stroke.hpp"
#include "skity/skity.hpp"
#include "utils/SrFloatComparison.h"
#include "utils/SrSVGPatternUtils.h"

#include <cstdint>
#include <string>

#define M_PI 3.14159265358979323846264338327950288 /* pi             */
#define LYNX_DEGREE_TO_RADIANS(X) ((M_PI * X) / 180)

namespace serval {
namespace svg {
namespace skity {

::skity::Matrix CreateAffineMatrix(const float* xform) {
  return {xform[0], xform[2], xform[4], xform[1], xform[3],
          xform[5], 0.f,      0.f,      1.f};
}

static inline void MultiplyTransformArray(const float (&lhs)[6],
                                          const float (&rhs)[6],
                                          float (&out)[6]) {
  out[0] = lhs[0] * rhs[0] + lhs[2] * rhs[1];
  out[1] = lhs[1] * rhs[0] + lhs[3] * rhs[1];
  out[2] = lhs[0] * rhs[2] + lhs[2] * rhs[3];
  out[3] = lhs[1] * rhs[2] + lhs[3] * rhs[3];
  out[4] = lhs[0] * rhs[4] + lhs[2] * rhs[5] + lhs[4];
  out[5] = lhs[1] * rhs[4] + lhs[3] * rhs[5] + lhs[5];
}

std::shared_ptr<::skity::ColorFilter> BuildSkityColorFilter(
    const canvas::SrFilterPrimitiveModel& primitive) {
  if (primitive.color_matrix_type == "luminanceToAlpha") {
    static const float kLumaToAlpha[20] = {
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.2126f, 0.7152f, 0.0722f, 0.f, 0.f,
    };
    return ::skity::ColorFilters::Matrix(kLumaToAlpha);
  }
  if (primitive.color_matrix_values.size() != 20) {
    return nullptr;
  }
  return ::skity::ColorFilters::Matrix(primitive.color_matrix_values.data());
}

std::shared_ptr<::skity::ImageFilter> BuildSkityImageFilter(
    const canvas::SrFilterModel& filter) {
  std::shared_ptr<::skity::ImageFilter> current_filter;
  for (const auto& primitive : filter.primitives) {
    std::shared_ptr<::skity::ImageFilter> next_filter;
    switch (primitive.type) {
      case canvas::SrFilterPrimitiveType::kGaussianBlur: {
        if (primitive.std_deviation_x <= 0.f &&
            primitive.std_deviation_y <= 0.f) {
          continue;
        }
        next_filter = ::skity::ImageFilters::Blur(primitive.std_deviation_x,
                                                  primitive.std_deviation_y);
        break;
      }
      case canvas::SrFilterPrimitiveType::kOffset: {
        if (primitive.dx == 0.f && primitive.dy == 0.f) {
          continue;
        }
        next_filter = ::skity::ImageFilters::MatrixTransform(
            ::skity::Matrix::Translate(primitive.dx, primitive.dy));
        break;
      }
      case canvas::SrFilterPrimitiveType::kColorMatrix: {
        auto color_filter = BuildSkityColorFilter(primitive);
        if (!color_filter) {
          return nullptr;
        }
        next_filter = ::skity::ImageFilters::ColorFilter(color_filter);
        break;
      }
      case canvas::SrFilterPrimitiveType::kComposite:
      case canvas::SrFilterPrimitiveType::kBlend:
      case canvas::SrFilterPrimitiveType::kFlood:
        return nullptr;
    }

    if (!next_filter) {
      continue;
    }
    current_filter = current_filter ? ::skity::ImageFilters::Compose(
                                          next_filter, current_filter)
                                    : next_filter;
  }
  return current_filter;
}

static inline void CopyTransformArray(const float (&src)[6], float (&out)[6]) {
  for (size_t i = 0; i < 6; ++i) {
    out[i] = src[i];
  }
}

static inline void CopyTransformArray(const std::array<float, 6>& src,
                                      float (&out)[6]) {
  for (size_t i = 0; i < src.size(); ++i) {
    out[i] = src[i];
  }
}

static inline void ResolveObjectBoundingBoxTransform(const float (&form)[6],
                                                     float left, float top,
                                                     float width, float height,
                                                     float (&out)[6]) {
  if (width == 0.f || height == 0.f) {
    CopyTransformArray(form, out);
    return;
  }
  const float bbox_to_user[6] = {width, 0.f, 0.f, height, left, top};
  const float user_to_bbox[6] = {1.f / width,  0.f,           0.f,
                                 1.f / height, -left / width, -top / height};
  float temp[6];
  MultiplyTransformArray(bbox_to_user, form, temp);
  MultiplyTransformArray(temp, user_to_bbox, out);
}

static ::skity::Matrix CreateRadialGradientAspectMatrix(::skity::Rect bound) {
  float form[6] = {1.f, 0.f, 0.f, 1.f, 0.f, 0.f};
  if (bound.Width() > bound.Height() && bound.Width() != 0.f) {
    float scale = bound.Height() / bound.Width();
    form[3] = scale;
    form[5] = bound.Top() * (1.f - scale);
  } else if (bound.Height() > bound.Width() && bound.Height() != 0.f) {
    float scale = bound.Width() / bound.Height();
    form[0] = scale;
    form[4] = bound.Left() * (1.f - scale);
  }
  return CreateAffineMatrix(form);
}

static void SRSVGArcToBezier(::skity::Path* path, double cx, double cy,
                             double a, double b, double e1x, double e1y,
                             double theta, double start, double sweep) {
  // Maximum of 45 degrees per cubic Bezier segment
  int numSegments = (int)ceil(fabs(sweep * 4 / M_PI));

  double eta1 = start;
  double cosTheta = cos(theta);
  double sinTheta = sin(theta);
  double cosEta1 = cos(eta1);
  double sinEta1 = sin(eta1);
  double ep1x = (-a * cosTheta * sinEta1) - (b * sinTheta * cosEta1);
  double ep1y = (-a * sinTheta * sinEta1) + (b * cosTheta * cosEta1);

  double anglePerSegment = sweep / numSegments;
  for (int i = 0; i < numSegments; i++) {
    double eta2 = eta1 + anglePerSegment;
    double sinEta2 = sin(eta2);
    double cosEta2 = cos(eta2);
    double e2x = cx + (a * cosTheta * cosEta2) - (b * sinTheta * sinEta2);
    double e2y = cy + (a * sinTheta * cosEta2) + (b * cosTheta * sinEta2);
    double ep2x = -a * cosTheta * sinEta2 - b * sinTheta * cosEta2;
    double ep2y = -a * sinTheta * sinEta2 + b * cosTheta * cosEta2;
    double tanDiff2 = tan((eta2 - eta1) / 2);
    double alpha =
        sin(eta2 - eta1) * (sqrt(4 + (3 * tanDiff2 * tanDiff2)) - 1) / 3;
    double q1x = e1x + alpha * ep1x;
    double q1y = e1y + alpha * ep1y;
    double q2x = e2x - alpha * ep2x;
    double q2y = e2y - alpha * ep2y;
    path->CubicTo(q1x, q1y, q2x, q2y, e2x, e2y);
    eta1 = eta2;
    e1x = e2x;
    e1y = e2y;
    ep1x = ep2x;
    ep1y = ep2y;
  }
}

static void SrSVGDrawArc(::skity::Path* path, float x, float y, float x1,
                         float y1, float a, float b, float theta,
                         const bool isMoreThanHalf, const bool isPositiveArc) {
  float thetaD = LYNX_DEGREE_TO_RADIANS(theta);
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
    SrSVGDrawArc(path, x, y, x1, y1, a * adjust, b * adjust, theta,
                 isMoreThanHalf, isPositiveArc);
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
  double tCx = cx;
  cx = cx * cosTheta - cy * sinTheta;
  cy = tCx * sinTheta + cy * cosTheta;
  SRSVGArcToBezier(path, cx, cy, a, b, x, y, thetaD, eta0, sweep);
}

::skity::Paint::Cap ConvertStrokeCap(SrSVGStrokeCap cap) {
  switch (cap) {
    case SR_SVG_STROKE_CAP_ROUND:
      return ::skity::Paint::kRound_Cap;
    case SR_SVG_STROKE_CAP_SQUARE:
      return ::skity::Paint::kSquare_Cap;
    case SR_SVG_STROKE_CAP_BUTT:
    default:
      return ::skity::Paint::kButt_Cap;
  }
}

::skity::Paint::Join ConvertStrokeJoin(SrSVGStrokeJoin join) {
  switch (join) {
    case SR_SVG_STROKE_JOIN_ROUND:
      return ::skity::Paint::kRound_Join;
    case SR_SVG_STROKE_JOIN_BEVEL:
      return ::skity::Paint::kBevel_Join;
    case SR_SVG_STROKE_JOIN_MITER:
    default:
      return ::skity::Paint::kMiter_Join;
  }
}

SrWinPath::~SrWinPath() {}

void SrWinPath::AddPath(canvas::Path* path) {
  if (auto win_path = static_cast<SrWinPath*>(path)) {
    path_.AddPath(win_path->path_);
  }
}

SrSVGBox SrWinPath::GetBounds() const {
  auto bounds = path_.GetBounds();
  return (SrSVGBox){.left = bounds.X(),
                    .top = bounds.Y(),
                    .width = bounds.Width(),
                    .height = bounds.Height()};
}

std::unique_ptr<canvas::Path> SrWinPath::CreateTransformCopy(
    const float (&xform)[6]) const {
  return std::make_unique<SrWinPath>(
      path_.CopyWithMatrix(CreateAffineMatrix(xform)));
}

void SrWinPath::Transform(const float (&xform)[6]) {
  path_ = path_.CopyWithMatrix(CreateAffineMatrix(xform));
}

void SrWinPath::SetFillType(SrSVGFillRule rule) {
  if (rule == SR_SVG_EO_FILL) {
    path_.SetFillType(::skity::Path::PathFillType::kEvenOdd);
  } else {
    path_.SetFillType(::skity::Path::PathFillType::kWinding);
  }
}

// skity path factory

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateCircle(float cx,
                                                               float cy,
                                                               float r) {
  auto path = std::make_unique<SrWinPath>();
  path->GetSkityPath()->AddCircle(cx, cy, r);
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateMutable() {
  return std::make_unique<SrWinPath>();
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateRect(float x, float y,
                                                             float rx, float ry,
                                                             float width,
                                                             float height) {
  auto path = std::make_unique<SrWinPath>();
  auto rect = ::skity::Rect(x, y, x + width, y + height);
  path->GetSkityPath()->AddRoundRect(rect, rx, ry);
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateLine(float start_x,
                                                             float start_y,
                                                             float end_x,
                                                             float end_y) {
  auto path = std::make_unique<SrWinPath>();
  path->GetSkityPath()->MoveTo(start_x, start_y);
  path->GetSkityPath()->LineTo(end_x, end_y);
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateEllipse(
    float center_x, float center_y, float radius_x, float radius_y) {
  auto path = std::make_unique<SrWinPath>();
  path->GetSkityPath()->AddCircle(0.f, 0.f, 1.f);
  const float xform[6] = {radius_x, 0.f, 0.f, radius_y, center_x, center_y};
  path->Transform(xform);
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreatePolygon(
    float points[], uint32_t n_points) {
  if (!points || n_points < 2) {
    return nullptr;
  }
  auto path = std::make_unique<SrWinPath>();
  path->GetSkityPath()->MoveTo(points[0], points[1]);
  for (uint32_t i = 1; i < n_points; ++i) {
    path->GetSkityPath()->LineTo(points[2 * i], points[2 * i + 1]);
  }
  path->GetSkityPath()->Close();
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreatePolyline(
    float points[], uint32_t n_points) {
  if (!points || n_points < 2) {
    return nullptr;
  }
  auto path = std::make_unique<SrWinPath>();
  path->GetSkityPath()->MoveTo(points[0], points[1]);
  for (uint32_t i = 1; i < n_points; ++i) {
    path->GetSkityPath()->LineTo(points[2 * i], points[2 * i + 1]);
  }
  return path;
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreatePath(uint8_t ops[],
                                                             uint64_t n_ops,
                                                             float args[],
                                                             uint64_t n_args) {
  auto path = std::make_unique<SrWinPath>();
  uint64_t iArg = 0;
  float x = .0f, y = .0f;
  float cp1x = .0f, cp1y = .0f, cp2x = .0f, cp2y = .0f;
  for (uint64_t i = 0; i < n_ops; i++) {
    switch (ops[i]) {
      case SPO_MOVE_TO:
        x = args[iArg++];
        y = args[iArg++];
        path->GetSkityPath()->MoveTo(x, y);
        break;
      case SPO_LINE_TO:
        x = args[iArg++];
        y = args[iArg++];
        path->GetSkityPath()->LineTo(x, y);
        break;
      case SPO_CUBIC_BEZ:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        cp2x = args[iArg++];
        cp2y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        path->GetSkityPath()->CubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
        break;
      case SPO_QUAD_ARC:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        path->GetSkityPath()->QuadTo(cp1x, cp1y, x, y);
        break;
      case SPO_ELLIPTICAL_ARC: {
        float c1x = args[iArg++], c1y = args[iArg++];
        float rx = args[iArg++];
        float ry = args[iArg++];
        float angle = args[iArg++];
        bool largeArc = fabs(args[iArg++]) > 1e-6 ? true : false;
        bool sweep = fabs(args[iArg++]) > 1e-6 ? true : false;
        float x = args[iArg++];
        float y = args[iArg++];
        SrSVGDrawArc(path->GetSkityPath(), c1x, c1y, x, y, rx, ry, angle,
                     largeArc, sweep);
        break;
      }
      case SPO_CLOSE:
        path->GetSkityPath()->Close();
        break;
      default:
        break;
    }
  }
  return std::move(path);
}

void SrPathFactorySkity::Op(canvas::Path* path1, canvas::Path* path2,
                            canvas::OP type) {
  auto path_q2d_1 = static_cast<SrWinPath*>(path1);
  auto path_q2d_2 = static_cast<SrWinPath*>(path2);
  if (!path_q2d_1 || !path_q2d_2) {
    return;
  }
  switch (type) {
    case canvas::DIFFERENCE:
      break;
    case canvas::INTERSECT:
      break;
    case canvas::UNION:
      path_q2d_1->AddPath(path_q2d_2);
      break;
    case canvas::XOR:
      break;
    case canvas::REVERSE_DIFFERENCE:
      break;
    default:
      break;
  }
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateStrokePath(
    const canvas::Path* path, float width, SrSVGStrokeCap cap,
    SrSVGStrokeJoin join, float miter_limit) {
  return CreateStrokePath(path, width, cap, join, miter_limit, 0.f, nullptr, 0);
}

std::unique_ptr<canvas::Path> SrPathFactorySkity::CreateStrokePath(
    const canvas::Path* path, float width, SrSVGStrokeCap cap,
    SrSVGStrokeJoin join, float miter_limit, float dash_offset,
    float* dash_array, size_t dash_array_length) {
  const auto* skity_path = static_cast<const SrWinPath*>(path);
  if (!skity_path || !FloatsLarger(width, 0.f)) {
    return nullptr;
  }

  ::skity::Paint paint;
  paint.SetStyle(::skity::Paint::kStroke_Style);
  paint.SetStrokeWidth(width);
  paint.SetStrokeCap(ConvertStrokeCap(cap));
  paint.SetStrokeJoin(ConvertStrokeJoin(join));
  paint.SetStrokeMiter(miter_limit);

  const ::skity::Path* stroke_source = skity_path->GetSkityPath();
  ::skity::Path dashed_path;
  if (dash_array && dash_array_length > 0) {
    auto path_effect = ::skity::PathEffect::MakeDashPathEffect(
        dash_array, static_cast<int>(dash_array_length), dash_offset);
    if (path_effect &&
        path_effect->FilterPath(&dashed_path, *stroke_source, true, paint)) {
      stroke_source = &dashed_path;
    }
  }

  ::skity::Stroke stroke(paint);
  ::skity::Path quad_path;
  ::skity::Path stroke_path;
  stroke.QuadPath(*stroke_source, &quad_path);
  stroke.StrokePath(quad_path, &stroke_path);
  stroke_path.SetFillType(::skity::Path::PathFillType::kWinding);
  return std::make_unique<SrWinPath>(std::move(stroke_path));
}

/// sr canvas

SrSkityCanvas::SrSkityCanvas(::skity::Canvas* canvas, ImageCallback callback)
    : canvas_(canvas),
      image_callback_(std::move(callback)),
      path_factory_(std::make_unique<SrPathFactorySkity>()) {}

SrSkityCanvas::~SrSkityCanvas() {}

canvas::PathFactory* SrSkityCanvas::PathFactory() {
  return path_factory_.get();
}

void SrSkityCanvas::PushTransformState() {
  transform_stack_.push_back(current_transform_);
}

void SrSkityCanvas::PopTransformState() {
  if (!transform_stack_.empty()) {
    current_transform_ = transform_stack_.back();
    transform_stack_.pop_back();
  }
}

void SrSkityCanvas::Save() {
  canvas_->Save();
  PushTransformState();
}

void SrSkityCanvas::Restore() {
  canvas_->Restore();
  PopTransformState();
}

void SrSkityCanvas::SaveLayer(const SrSVGBox* bounds) {
  ::skity::Rect layer_bounds = canvas_->GetLocalClipBounds();
  if (bounds) {
    layer_bounds = ::skity::Rect::MakeXYWH(bounds->left, bounds->top,
                                           bounds->width, bounds->height);
  }
  canvas_->SaveLayer(layer_bounds, ::skity::Paint());
  PushTransformState();
  canvas_->DrawColor(0, ::skity::BlendMode::kSrc);
}

void SrSkityCanvas::RestoreLayer() {
  canvas_->Restore();
  PopTransformState();
}

void SrSkityCanvas::BeginOpacityLayer(const SrSVGBox* bounds, float opacity) {
  ::skity::Rect layer_bounds = canvas_->GetLocalClipBounds();
  if (bounds) {
    layer_bounds = ::skity::Rect::MakeXYWH(bounds->left, bounds->top,
                                           bounds->width, bounds->height);
  }
  ::skity::Paint paint;
  paint.SetAlpha(static_cast<uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f));
  canvas_->SaveLayer(layer_bounds, paint);
  PushTransformState();
  canvas_->DrawColor(0, ::skity::BlendMode::kSrc);
}

void SrSkityCanvas::EndOpacityLayer() {
  RestoreLayer();
}

bool SrSkityCanvas::SupportsFilterModel(
    const canvas::SrFilterModel& filter) const {
  return canvas::SrSupportsLinearSourceGraphicFilterModel(filter);
}

void SrSkityCanvas::BeginFilterLayer(const SrSVGBox* bounds,
                                     const canvas::SrFilterModel& filter) {
  auto image_filter = BuildSkityImageFilter(filter);
  ::skity::Paint paint;
  if (image_filter) {
    paint.SetImageFilter(image_filter);
  }

  ::skity::Rect layer_bounds = canvas_->GetLocalClipBounds();
  if (bounds && bounds->width > 0.f && bounds->height > 0.f) {
    layer_bounds = ::skity::Rect::MakeXYWH(bounds->left, bounds->top,
                                           bounds->width, bounds->height);
  }
  canvas_->SaveLayer(layer_bounds, paint);
  PushTransformState();
  canvas_->DrawColor(0, ::skity::BlendMode::kSrc);
}

void SrSkityCanvas::EndFilterLayer() {
  RestoreLayer();
}

void SrSkityCanvas::BeginMaskLayer(const SrSVGBox* bounds, bool is_luminance) {
  mask_is_luminance_ = is_luminance;
  SaveLayer(bounds);
}

void SrSkityCanvas::BeginMaskContentLayer() {
  if (dst_in_layer_active_) {
    return;
  }

  ::skity::Paint paint;
  paint.SetBlendMode(::skity::BlendMode::kDstIn);
  if (mask_is_luminance_) {
    static const float kLumaToAlpha[20] = {
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.f,     0.f,     0.f,     0.f, 0.f,  //
        0.2126f, 0.7152f, 0.0722f, 0.f, 0.f,
    };
    paint.SetColorFilter(::skity::ColorFilters::Matrix(kLumaToAlpha));
  }
  canvas_->SaveLayer(canvas_->GetLocalClipBounds(), paint);
  PushTransformState();
  dst_in_layer_active_ = true;
}

void SrSkityCanvas::EndMaskContentLayer() {
  blend_mode_override_.reset();
  if (dst_in_layer_active_) {
    canvas_->Restore();
    PopTransformState();
    dst_in_layer_active_ = false;
  }
}

void SrSkityCanvas::EndMaskLayer() {
  RestoreLayer();
  mask_is_luminance_ = false;
}

void SrSkityCanvas::RenderPatternTiles(
    const element::ResolvedPattern& resolved_pattern,
    const SrSVGBox& target_bounds) {
  bool inserted = active_pattern_ids_.insert(resolved_pattern.id).second;

  SrSVGBox pattern_area = target_bounds;
  if (!element::IsIdentityTransform(resolved_pattern.pattern_transform)) {
    Transform(resolved_pattern.pattern_transform);
    float inverse[6];
    if (element::InvertAffineTransform(resolved_pattern.pattern_transform,
                                       inverse)) {
      pattern_area = element::MapBounds(target_bounds, inverse);
    }
  }

  float origin_x =
      resolved_pattern.x + std::floor((pattern_area.left - resolved_pattern.x) /
                                      resolved_pattern.width) *
                               resolved_pattern.width;
  float origin_y =
      resolved_pattern.y + std::floor((pattern_area.top - resolved_pattern.y) /
                                      resolved_pattern.height) *
                               resolved_pattern.height;
  float right = pattern_area.left + pattern_area.width;
  float bottom = pattern_area.top + pattern_area.height;
  const bool has_resolved_view_box =
      resolved_pattern.has_view_box &&
      FloatsLarger(resolved_pattern.view_box.width, 0.f) &&
      FloatsLarger(resolved_pattern.view_box.height, 0.f);
  const bool uses_object_bounding_box_content_units =
      resolved_pattern.pattern_content_units ==
      SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
  SrSVGRenderContext base_tile_context = *current_render_context_;

  for (float step_y = origin_y; step_y < bottom;
       step_y += resolved_pattern.height) {
    for (float step_x = origin_x; step_x < right;
         step_x += resolved_pattern.width) {
      Save();
      canvas_->ClipRect(::skity::Rect::MakeXYWH(
          step_x, step_y, resolved_pattern.width, resolved_pattern.height));

      SrSVGRenderContext tile_context = base_tile_context;
      if (has_resolved_view_box) {
        SrSVGBox tile_view_port{step_x, step_y, resolved_pattern.width,
                                resolved_pattern.height};
        tile_context.view_port = tile_view_port;
        tile_context.view_box = resolved_pattern.view_box;
        float view_box_xform[6];
        calculate_view_box_transform(
            &tile_view_port, &resolved_pattern.view_box,
            resolved_pattern.preserve_aspect_ratio, view_box_xform);
        Transform(view_box_xform);
      } else {
        if (uses_object_bounding_box_content_units) {
          tile_context.view_port = SrSVGBox{0.f, 0.f, 1.f, 1.f};
          tile_context.view_box = SrSVGBox{0.f, 0.f, 0.f, 0.f};
          Translate(step_x, step_y);
          float scale_xform[6];
          xform_set_scale(scale_xform, target_bounds.width,
                          target_bounds.height);
          Transform(scale_xform);
        } else {
          tile_context.view_port = resolved_pattern.view_port;
          tile_context.view_box = resolved_pattern.view_port;
          Translate(step_x, step_y);
        }
      }

      auto* previous_render_context = current_render_context_;
      resolved_pattern.content_pattern->RenderContent(this, tile_context);
      current_render_context_ = previous_render_context;
      Restore();
    }
  }

  if (inserted) {
    active_pattern_ids_.erase(resolved_pattern.id);
  }
}

bool SrSkityCanvas::RenderPatternFill(const ::skity::Path& path,
                                      const SrSVGRenderState& render_state,
                                      const char* iri) {
  if (!current_render_context_ || !iri || iri[0] != '#' ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  ::skity::Rect path_bounds = path.GetBounds();
  SrSVGBox bounds{path_bounds.X(), path_bounds.Y(), path_bounds.Width(),
                  path_bounds.Height()};
  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_, bounds,
                                      active_pattern_ids_, &resolved_pattern)) {
    return false;
  }

  Save();
  SrWinPath clip_path(path);
  ClipPath(&clip_path, render_state.fill_rule);
  RenderPatternTiles(resolved_pattern, bounds);
  Restore();
  return true;
}

bool SrSkityCanvas::RenderPatternStroke(const ::skity::Path& path,
                                        const SrSVGRenderState& render_state,
                                        const char* iri) {
  if (!current_render_context_ || !iri || iri[0] != '#' ||
      !render_state.stroke || !FloatsLarger(render_state.stroke_width, 0.f) ||
      !element::IsPatternIri(iri, *current_render_context_)) {
    return false;
  }

  ::skity::Rect path_bounds = path.GetBounds();
  SrSVGBox object_bounds{path_bounds.X(), path_bounds.Y(), path_bounds.Width(),
                         path_bounds.Height()};
  element::ResolvedPattern resolved_pattern;
  if (!element::ResolvePatternFromIri(iri, *current_render_context_,
                                      object_bounds, active_pattern_ids_,
                                      &resolved_pattern)) {
    return false;
  }

  SrSVGStrokeCap stroke_line_cap = SR_SVG_STROKE_CAP_BUTT;
  SrSVGStrokeJoin stroke_line_join = SR_SVG_STROKE_JOIN_MITER;
  float stroke_miter_limit = element::SrSVGNode::s_stroke_miter_limit;
  float stroke_dash_offset = 0.f;
  float* dash_array = nullptr;
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

  ::skity::Path transformed_path;
  const ::skity::Path* stroke_source_path = &path;
  if (use_non_scaling_stroke) {
    transformed_path = path.CopyWithMatrix(CreateAffineMatrix(current));
    stroke_source_path = &transformed_path;
  }

  SrWinPath source_path(*stroke_source_path);
  auto stroke_clip_path = path_factory_->CreateStrokePath(
      &source_path, render_state.stroke_width, stroke_line_cap,
      stroke_line_join, stroke_miter_limit, stroke_dash_offset, dash_array,
      dash_array_length);
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

bool SrSkityCanvas::DrawNonScalingStroke(::skity::Path& path,
                                         const SrSVGRenderState& render_state) {
  if (render_state.vector_effect != SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE ||
      !render_state.stroke || !FloatsLarger(render_state.stroke_width, 0.f)) {
    return false;
  }

  float current[6];
  float inverse[6];
  CopyTransformArray(current_transform_, current);
  if (!element::InvertAffineTransform(current, inverse)) {
    return false;
  }

  ::skity::Path transformed_path =
      path.CopyWithMatrix(CreateAffineMatrix(current));
  SrSVGStrokeCap stroke_line_cap = SR_SVG_STROKE_CAP_BUTT;
  SrSVGStrokeJoin stroke_line_join = SR_SVG_STROKE_JOIN_MITER;
  float stroke_miter_limit = element::SrSVGNode::s_stroke_miter_limit;
  float stroke_dash_offset = 0.f;
  float* dash_array = nullptr;
  size_t dash_array_length = 0;
  if (render_state.stroke_state) {
    stroke_line_cap = render_state.stroke_state->stroke_line_cap;
    stroke_line_join = render_state.stroke_state->stroke_line_join;
    stroke_miter_limit = render_state.stroke_state->stroke_miter_limit;
    stroke_dash_offset = render_state.stroke_state->stroke_dash_offset;
    dash_array = render_state.stroke_state->dash_array;
    dash_array_length = render_state.stroke_state->dash_array_length;
  }

  SrWinPath source_path(transformed_path);
  auto stroke_path = path_factory_->CreateStrokePath(
      &source_path, render_state.stroke_width, stroke_line_cap,
      stroke_line_join, stroke_miter_limit, stroke_dash_offset, dash_array,
      dash_array_length);
  auto* skity_stroke_path =
      stroke_path ? static_cast<SrWinPath*>(stroke_path.get()) : nullptr;
  if (!skity_stroke_path) {
    return false;
  }

  Save();
  Transform(inverse);
  SrSVGRenderState stroke_fill_state = render_state;
  stroke_fill_state.fill = render_state.stroke;
  stroke_fill_state.fill_opacity = render_state.stroke_opacity;
  stroke_fill_state.stroke = nullptr;
  canvas_->DrawPath(
      *skity_stroke_path->GetSkityPath(),
      ConvertToPaint(stroke_fill_state, path.GetBounds(), false, current));
  Restore();
  return true;
}

void SrSkityCanvas::DrawPathWithRenderState(
    ::skity::Path& path, const SrSVGRenderState& render_state) {
  if (render_state.fill_rule == SR_SVG_EO_FILL) {
    path.SetFillType(::skity::Path::PathFillType::kEvenOdd);
  } else {
    path.SetFillType(::skity::Path::PathFillType::kWinding);
  }

  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    const bool rendered_pattern_fill =
        render_state.fill->type == SrSVGPaintType::SERVAL_PAINT_IRI &&
        RenderPatternFill(path, render_state, render_state.fill->content.iri);
    const bool has_gradient_fill =
        render_state.fill->type == SrSVGPaintType::SERVAL_PAINT_IRI &&
        (lg_models_.find(render_state.fill->content.iri) != lg_models_.end() ||
         rg_models_.find(render_state.fill->content.iri) != rg_models_.end());
    if (!rendered_pattern_fill) {
      if (render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_IRI ||
          has_gradient_fill) {
        canvas_->DrawPath(
            path, ConvertToPaint(render_state, path.GetBounds(), false));
      }
    }
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    const bool is_pattern_stroke =
        render_state.stroke->type == SrSVGPaintType::SERVAL_PAINT_IRI &&
        current_render_context_ && render_state.stroke->content.iri &&
        element::IsPatternIri(render_state.stroke->content.iri,
                              *current_render_context_);
    if (is_pattern_stroke) {
      if (RenderPatternStroke(path, render_state,
                              render_state.stroke->content.iri)) {
        return;
      }
      return;
    }
    if (DrawNonScalingStroke(path, render_state)) {
      return;
    }
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), true));
  }
}

void SrSkityCanvas::DrawLine(const char*, float x1, float y1, float x2,
                             float y2, const SrSVGRenderState& render_state) {
  ::skity::Path path;
  path.MoveTo(x1, y1);
  path.LineTo(x2, y2);
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawRect(const char* id, float x, float y, float rx,
                             float ry, float width, float height,
                             const SrSVGRenderState& render_state) {
  ::skity::Path path;
  path.AddRoundRect({x, y, x + width, y + height}, rx, ry);
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawCircle(const char*, float cx, float cy, float r,
                               const SrSVGRenderState& render_state) {
  ::skity::Path path;
  path.AddCircle(cx, cy, r);
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawPolygon(const char*, float* points, uint32_t n_points,
                                const SrSVGRenderState& render_state) {
  if (n_points < 2)
    return;
  ::skity::Path path;
  path.MoveTo(points[0], points[1]);
  for (uint32_t i = 1; i < n_points; ++i) {
    path.LineTo(points[2 * i], points[2 * i + 1]);
  }
  path.Close();
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawPolyline(const char*, float* points, uint32_t n_points,
                                 const SrSVGRenderState& render_state) {
  if (n_points < 2)
    return;
  ::skity::Path path;
  path.MoveTo(points[0], points[1]);
  for (uint32_t i = 1; i < n_points; ++i) {
    path.LineTo(points[2 * i], points[2 * i + 1]);
  }
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawEllipse(const char*, float center_x, float center_y,
                                float radius_x, float radius_y,
                                const SrSVGRenderState& render_state) {
  ::skity::Path path;
  path.AddCircle(0.f, 0.f, 1.f);
  path = path.CopyWithMatrix(::skity::Matrix{
      radius_x, 0.f, center_x, 0.f, radius_y, center_y, 0.f, 0.f, 1.f});
  DrawPathWithRenderState(path, render_state);
}

void SrSkityCanvas::DrawPath(const char*, uint8_t* ops, uint32_t n_ops,
                             float* args, uint32_t n_args,
                             const SrSVGRenderState& render_state) {
  canvas_->Save();
  ::skity::Path path;
  uint64_t iArg = 0;
  float x = 0.0f, y = 0.0f;
  float cp1x = 0.0f, cp1y = 0.0f, cp2x = 0.0f, cp2y = 0.0f;
  for (uint64_t i = 0; i < n_ops; i++) {
    switch (ops[i]) {
      case SPO_MOVE_TO:
        x = args[iArg++];
        y = args[iArg++];
        path.MoveTo(x, y);
        break;
      case SPO_LINE_TO:
        x = args[iArg++];
        y = args[iArg++];
        path.LineTo(x, y);
        break;
      case SPO_CUBIC_BEZ:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        cp2x = args[iArg++];
        cp2y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        path.CubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
        break;
      case SPO_QUAD_ARC:
        cp1x = args[iArg++];
        cp1y = args[iArg++];
        x = args[iArg++];
        y = args[iArg++];
        path.QuadTo(cp1x, cp1y, x, y);
        break;
      case SPO_ELLIPTICAL_ARC: {
        float c1x = args[iArg++], c1y = args[iArg++];
        float rx = args[iArg++];
        float ry = args[iArg++];
        float angle = args[iArg++];
        bool largeArc = fabs(args[iArg++]) > 1e-6 ? true : false;
        bool sweep = fabs(args[iArg++]) > 1e-6 ? true : false;
        float x = args[iArg++];
        float y = args[iArg++];
        SrSVGDrawArc(&path, c1x, c1y, x, y, rx, ry, angle, largeArc, sweep);
        break;
      }
      case SPO_CLOSE:
        path.Close();
        break;
      default:
        break;
    }
  }
  DrawPathWithRenderState(path, render_state);
  canvas_->Restore();
}

void SrSkityCanvas::DrawUse(const char* href, float x, float y, float width,
                            float height) {}

void SrSkityCanvas::DrawImage(
    const char* url, float x, float y, float width, float height,
    const SrSVGPreserveAspectRatio& preserve_aspect_radio) {
  if (url) {
    std::string href(url);

    if (image_callback_) {
      auto image = image_callback_(href);
      if (image) {
        float form[6];
        SrSVGBox view_port{x, y, width, height};
        SrSVGBox view_box{0, 0, static_cast<float>(image->Width()),
                          static_cast<float>(image->Height())};
        calculate_view_box_transform(&view_port, &view_box,
                                     preserve_aspect_radio, form);
        canvas_->Save();
        ::skity::Matrix box_transform{
            form[0], form[2], form[4], form[1], form[3], form[5], 0, 0, 1};
        canvas_->Concat(box_transform);
        ::skity::Matrix flipY;
        flipY.Scale(1, -1);
        canvas_->Concat(flipY);
        ::skity::SamplingOptions options{};
        options.filter = ::skity::FilterMode::kLinear;
        canvas_->DrawImage(image, ::skity::Rect::MakeXYWH(x, y, width, height),
                           options);
        canvas_->Restore();
      }
    }
  }
}

void SrSkityCanvas::SetViewBox(float x, float y, float width, float height) {
  canvas_->Save();
  canvas_->ResetMatrix();
  ::skity::Paint paint;
  paint.SetBlendMode(::skity::BlendMode::kSrc);
  paint.SetColor(0);
  auto rect = ::skity::Rect::MakeXYWH(x, y, width, height);
  canvas_->DrawRect(rect, paint);
  canvas_->Restore();
}

void SrSkityCanvas::UpdateLinearGradient(
    const char* id, const float (&gradient_transform)[6],
    const GradientSpread spread, float x1, float x2, float y1, float y2,
    const std::vector<SrStop>& stops,
    SrSVGObjectBoundingBoxUnitType bounding_box_type) {
  if (strlen(id)) {
    lg_models_[std::string("#") + id] = canvas::LinearGradientModel(
        spread, x1, x2, y1, y2, gradient_transform, stops, bounding_box_type);
  }
}

void SrSkityCanvas::UpdateRadialGradient(
    const char* id, const float (&gradient_transform)[6],
    const GradientSpread spread, float cx, float cy, float fr, float fx,
    float fy, const std::vector<SrStop>& stops,
    SrSVGObjectBoundingBoxUnitType bounding_box_type) {
  if (strlen(id)) {
    rg_models_[std::string("#") + id] = canvas::RadialGradientModel(
        spread, cx, cy, fr, fx, fy, gradient_transform, stops,
        bounding_box_type);
  }
}

void SrSkityCanvas::Translate(float x, float y) {
  canvas_->Translate(x, y);
  float translation[6];
  float current[6];
  float out[6];
  xform_set_translation(translation, x, y);
  CopyTransformArray(current_transform_, current);
  MultiplyTransformArray(current, translation, out);
  current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
}

void SrSkityCanvas::Transform(const float (&form)[6]) {
  canvas_->Concat(CreateAffineMatrix(form));
  float current[6];
  float out[6];
  CopyTransformArray(current_transform_, current);
  MultiplyTransformArray(current, form, out);
  current_transform_ = {out[0], out[1], out[2], out[3], out[4], out[5]};
}

void SrSkityCanvas::ClipPath(canvas::Path* path, SrSVGFillRule clip_rule) {
  if (auto skity_path = static_cast<SrWinPath*>(path)) {
    if (clip_rule == SR_SVG_EO_FILL) {
      skity_path->GetSkityPath()->SetFillType(
          ::skity::Path::PathFillType::kEvenOdd);
    } else {
      skity_path->GetSkityPath()->SetFillType(
          ::skity::Path::PathFillType::kWinding);
    }
    canvas_->ClipPath(*skity_path->GetSkityPath(),
                      ::skity::Canvas::ClipOp::kIntersect);
  }
}

std::shared_ptr<::skity::Shader> ConvertToLinearGradientShader(
    const canvas::LinearGradientModel& linear, ::skity::Rect bound) {
  float x1 = linear.x1_;
  float x2 = linear.x2_;
  float y1 = linear.y1_;
  float y2 = linear.y2_;
  if (linear.obb_type_ == SrSVGObjectBoundingBoxUnitType::
                              SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    x1 = bound.Left() + x1 * bound.Width();
    y1 = bound.Top() + y1 * bound.Height();
    x2 = bound.Left() + x2 * bound.Width();
    y2 = bound.Top() + y2 * bound.Height();
  }

  ::skity::Point pts[2];
  pts[0] = {x1, y1, 0.0, 1.0};
  pts[1] = {x2, y2, 0.0, 1.0};

  std::vector<::skity::Vec4> colors;
  std::vector<float> offsets;
  for (auto& stop : linear.stops_) {
    auto color = ::skity::Color4fFromColor(stop.stopColor.color);
    color.a *= stop.stopOpacity.value;
    colors.emplace_back(std::move(color));
    offsets.emplace_back(stop.offset.value);
  }
  if (colors.size() < 2) {
    return nullptr;
  }
  ::skity::TileMode mode = ::skity::TileMode::kClamp;
  if (linear.spread_mode_ == GradientSpread::reflect) {
    mode = ::skity::TileMode::kMirror;
  } else if (linear.spread_mode_ == GradientSpread::repeat) {
    mode = ::skity::TileMode::kRepeat;
  }
  auto lgs = ::skity::Shader::MakeLinear(pts, colors.data(), offsets.data(),
                                         linear.stop_size(), mode, 0);
  if (!lgs) {
    return nullptr;
  }
  float resolved_transform[6];
  if (linear.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    ResolveObjectBoundingBoxTransform(linear.gradient_transformer_,
                                      bound.Left(), bound.Top(), bound.Width(),
                                      bound.Height(), resolved_transform);
  } else {
    CopyTransformArray(linear.gradient_transformer_, resolved_transform);
  }
  lgs->SetLocalMatrix(CreateAffineMatrix(resolved_transform));
  return lgs;
}

std::shared_ptr<::skity::Shader> ConvertToRadialGradientShader(
    const canvas::RadialGradientModel& radial, ::skity::Rect bound) {
  ::skity::Point startCenter{radial.fx_, radial.fy_, 0.0f, 1.0f};
  ::skity::Point endCenter{radial.cx_, radial.cy_, 0.0f, 1.0f};
  float startRadius = 0.f;
  float endRadius = radial.r_;
  if (radial.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    auto max_size = std::max(bound.Width(), bound.Height());
    endRadius = radial.r_ * max_size;
    startCenter = {bound.Left() + radial.fx_ * max_size,
                   bound.Top() + radial.fy_ * max_size, 0.0f, 1.0f};
    endCenter = {bound.Left() + radial.cx_ * max_size,
                 bound.Top() + radial.cy_ * max_size, 0.0f, 1.0f};
  }

  std::vector<::skity::Vec4> colors;
  std::vector<float> offsets;
  for (auto& stop : radial.stops_) {
    auto color = ::skity::Color4fFromColor(stop.stopColor.color);
    color.a *= stop.stopOpacity.value;
    colors.emplace_back(std::move(color));
    offsets.emplace_back(stop.offset.value);
  }
  if (colors.size() < 2) {
    return nullptr;
  }
  ::skity::TileMode mode = ::skity::TileMode::kClamp;
  if (radial.spread_mode_ == GradientSpread::reflect) {
    mode = ::skity::TileMode::kMirror;
  } else if (radial.spread_mode_ == GradientSpread::repeat) {
    mode = ::skity::TileMode::kRepeat;
  }
  auto lgs = ::skity::Shader::MakeTwoPointConical(
      startCenter, startRadius, endCenter, endRadius, colors.data(),
      offsets.data(), radial.stop_size(), mode, 0);
  if (!lgs) {
    return nullptr;
  }
  float resolved_transform[6];
  if (radial.obb_type_ == SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX) {
    auto max_size = std::max(bound.Width(), bound.Height());
    ResolveObjectBoundingBoxTransform(radial.gradient_transformer_,
                                      bound.Left(), bound.Top(), max_size,
                                      max_size, resolved_transform);
    lgs->SetLocalMatrix(CreateRadialGradientAspectMatrix(bound) *
                        CreateAffineMatrix(resolved_transform));
  } else {
    CopyTransformArray(radial.gradient_transformer_, resolved_transform);
    lgs->SetLocalMatrix(CreateAffineMatrix(resolved_transform));
  }
  return lgs;
}

::skity::Paint SrSkityCanvas::ConvertToPaint(
    const SrSVGRenderState& render_state, ::skity::Rect bound, bool is_stroke,
    const float* shader_transform) {
  ::skity::Paint paint;
  paint.SetStyle(is_stroke ? ::skity::Paint::kStroke_Style
                           : ::skity::Paint::kFill_Style);
  float alpha =
      is_stroke ? render_state.stroke_opacity : render_state.fill_opacity;
  paint.SetAlpha(alpha * 255);
  if (is_stroke) {
    paint.SetStrokeWidth(render_state.stroke_width);
  }

  auto do_paint = [&](SrSVGPaint* sr_paint) {
    if (sr_paint) {
      if (sr_paint->type == SrSVGPaintType::SERVAL_PAINT_COLOR) {
        uint32_t color = sr_paint->content.color.color;
        uint32_t color_alpha = (color >> 24) & 0xFF;
        color_alpha = static_cast<uint32_t>(color_alpha * alpha);
        color = (color & 0x00FFFFFF) | (color_alpha << 24);

        if (is_stroke) {
          paint.SetStrokeColor(color);
        } else {
          paint.SetFillColor(color);
        }
      } else if (sr_paint->type == SrSVGPaintType::SERVAL_PAINT_IRI) {
        do {
          auto ret_l = lg_models_.find(sr_paint->content.iri);
          if (ret_l != lg_models_.end()) {
            auto shader = ConvertToLinearGradientShader(ret_l->second, bound);
            if (shader && shader_transform) {
              shader->SetLocalMatrix(CreateAffineMatrix(shader_transform) *
                                     shader->GetLocalMatrix());
            }
            paint.SetShader(shader);
            break;
          }

          auto ret_r = rg_models_.find(sr_paint->content.iri);
          if (ret_r != rg_models_.end()) {
            auto shader = ConvertToRadialGradientShader(ret_r->second, bound);
            if (shader && shader_transform) {
              shader->SetLocalMatrix(CreateAffineMatrix(shader_transform) *
                                     shader->GetLocalMatrix());
            }
            paint.SetShader(shader);
            break;
          }
        } while (false);
      }
    }
  };

  if (is_stroke) {
    do_paint(render_state.stroke);
    if (render_state.stroke_state) {
      paint.SetStrokeCap(
          ConvertStrokeCap(render_state.stroke_state->stroke_line_cap));
      paint.SetStrokeJoin(
          ConvertStrokeJoin(render_state.stroke_state->stroke_line_join));
      paint.SetStrokeMiter(render_state.stroke_state->stroke_miter_limit);
      if (render_state.stroke_state->dash_array &&
          render_state.stroke_state->dash_array_length > 0) {
        std::vector<float> pattern;
        for (size_t i = 0; i < render_state.stroke_state->dash_array_length;
             ++i) {
          pattern.emplace_back(render_state.stroke_state->dash_array[i]);
        }
        paint.SetPathEffect(::skity::PathEffect::MakeDashPathEffect(
            pattern.data(), pattern.size(),
            render_state.stroke_state->stroke_dash_offset));
      }
    }
  } else {
    do_paint(render_state.fill);
  }

  if (blend_mode_override_) {
    paint.SetBlendMode(*blend_mode_override_);
  }

  return paint;
}

}  // namespace skity
}  // namespace svg
}  // namespace serval
