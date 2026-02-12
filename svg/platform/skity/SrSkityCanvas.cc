// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/skity/SrSkityCanvas.h"
#include <cstring>
#include <vector>
#include "parser/SrSVGDOM.h"
#include "skity/skity.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

// write Unicode code point into UTF-8
static inline void AppendUtf8(std::string& out, uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

static inline uint16_t LoadU16BE(const uint8_t* p) {
  return static_cast<uint16_t>((uint16_t(p[0]) << 8) | p[1]);
}

// main conversion: UTF-16BE (no BOM) → UTF-8
std::string Utf16BE_To_Utf8_NoBOM(const void* raw, size_t size_bytes) {
  if (size_bytes % 2 != 0) {
    throw std::runtime_error("Invalid UTF-16BE byte length");
  }
  const uint8_t* p = static_cast<const uint8_t*>(raw);
  const uint8_t* end = p + size_bytes;

  std::string out;
  out.reserve(size_bytes);  // rough estimate

  while (p < end) {
    uint16_t w1 = LoadU16BE(p);
    p += 2;

    if (w1 >= 0xD800 && w1 <= 0xDBFF) {
      // high surrogate must be followed by low surrogate
      if (p >= end)
        throw std::runtime_error("Truncated surrogate pair");
      uint16_t w2 = LoadU16BE(p);
      if (w2 < 0xDC00 || w2 > 0xDFFF)
        throw std::runtime_error("Invalid surrogate pair");
      p += 2;
      uint32_t cp =
          0x10000 + (((uint32_t)(w1 - 0xD800) << 10) | (uint32_t)(w2 - 0xDC00));
      AppendUtf8(out, cp);
    } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
      throw std::runtime_error("Unpaired low surrogate");
    } else {
      // Basic Multilingual Plane (BMP)
      AppendUtf8(out, w1);
    }
  }
  return out;
}

#define M_PI 3.14159265358979323846264338327950288 /* pi             */
#define LYNX_DEGREE_TO_RADIANS(X) ((M_PI * X) / 180)

namespace serval {
namespace svg {
namespace skity {

::skity::Matrix CreateAffineMatrix(const float (&xform)[6]) {
  return {xform[0], xform[2], xform[4], xform[1], xform[3],
          xform[5], 0.f,      0.f,      1.f};
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
    return nullptr;
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

void SrSkityCanvas::Save() {
  canvas_->Save();
}

void SrSkityCanvas::Restore() {
  canvas_->Restore();
}

void SrSkityCanvas::DrawLine(const char*, float x1, float y1, float x2,
                             float y2, const SrSVGRenderState& render_state) {
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawLine(x1, y1, x2, y2,
                      ConvertToPaint(render_state, {x1, y1, x2, y2}, true));
  }
}

void SrSkityCanvas::DrawRect(const char* id, float x, float y, float rx,
                             float ry, float width, float height,
                             const SrSVGRenderState& render_state) {
  ::skity::Rect rect = {x, y, x + width, y + height};
  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawRoundRect(rect, rx, ry,
                           ConvertToPaint(render_state, rect, false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawRoundRect(rect, rx, ry,
                           ConvertToPaint(render_state, rect, true));
  }
}

void SrSkityCanvas::DrawCircle(const char*, float cx, float cy, float r,
                               const SrSVGRenderState& render_state) {
  ::skity::Rect rect = {cx - r, cy - r, cx + r, cy + r};
  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawCircle(cx, cy, r, ConvertToPaint(render_state, rect, false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawCircle(cx, cy, r, ConvertToPaint(render_state, rect, true));
  }
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
  if (render_state.fill_rule == SR_SVG_EO_FILL) {
    path.SetFillType(::skity::Path::PathFillType::kEvenOdd);
  } else {
    path.SetFillType(::skity::Path::PathFillType::kWinding);
  }

  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), true));
  }
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

  if (render_state.fill_rule == SR_SVG_EO_FILL) {
    path.SetFillType(::skity::Path::PathFillType::kEvenOdd);
  } else {
    path.SetFillType(::skity::Path::PathFillType::kWinding);
  }

  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), true));
  }
}

void SrSkityCanvas::DrawEllipse(const char*, float center_x, float center_y,
                                float radius_x, float radius_y,
                                const SrSVGRenderState& render_state) {
  ::skity::Rect rect = {center_x - radius_x, center_y - radius_y,
                        center_x + radius_x, center_y + radius_y};
  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawOval(rect, ConvertToPaint(render_state, rect, false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawOval(rect, ConvertToPaint(render_state, rect, true));
  }
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
        path.MoveTo(c1x, c1y);
        path.ArcTo(rx, ry, angle,
                   largeArc ? ::skity::Path::ArcSize::kLarge
                            : ::skity::Path::ArcSize::kSmall,
                   sweep ? ::skity::Path::Direction::kCW
                         : ::skity::Path::Direction::kCCW,
                   x, y);
        break;
      }
      case SPO_CLOSE:
        path.Close();
        break;
      default:
        break;
    }
  }
  if (render_state.fill_rule == SR_SVG_EO_FILL) {
    path.SetFillType(::skity::Path::PathFillType::kEvenOdd);
  } else {
    path.SetFillType(::skity::Path::PathFillType::kWinding);
  }

  if (render_state.fill &&
      render_state.fill->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), false));
  }
  if (render_state.stroke &&
      render_state.stroke->type != SrSVGPaintType::SERVAL_PAINT_NONE) {
    canvas_->DrawPath(path,
                      ConvertToPaint(render_state, path.GetBounds(), true));
  }
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
        canvas_->SaveLayer(::skity::Rect(), ::skity::Paint());
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
  ::skity::Paint paint;
  paint.SetBlendMode(::skity::BlendMode::kClear);
  auto rect = ::skity::Rect::MakeXYWH(x, y, width, height);
  canvas_->DrawRect(rect, paint);
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
}

void SrSkityCanvas::Transform(const float (&form)[6]) {
  canvas_->Concat(CreateAffineMatrix(form));
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
  ::skity::TileMode mode = ::skity::TileMode::kClamp;
  if (linear.spread_mode_ == GradientSpread::reflect) {
    mode = ::skity::TileMode::kMirror;
  } else if (linear.spread_mode_ == GradientSpread::repeat) {
    mode = ::skity::TileMode::kRepeat;
  }
  auto lgs = ::skity::Shader::MakeLinear(pts, colors.data(), offsets.data(),
                                         linear.stop_size(), mode, 0);
  ::skity::Matrix matrix(
      linear.gradient_transformer_[0], linear.gradient_transformer_[1], 0.0f,
      0.0f, linear.gradient_transformer_[2], linear.gradient_transformer_[3],
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, linear.gradient_transformer_[4],
      linear.gradient_transformer_[5], 0.0f, 1.0f);
  lgs->SetLocalMatrix(matrix);
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
  ::skity::TileMode mode = ::skity::TileMode::kClamp;
  if (radial.spread_mode_ == GradientSpread::reflect) {
    mode = ::skity::TileMode::kMirror;
  } else if (radial.spread_mode_ == GradientSpread::repeat) {
    mode = ::skity::TileMode::kRepeat;
  }
  auto lgs = ::skity::Shader::MakeTwoPointConical(
      startCenter, startRadius, endCenter, endRadius, colors.data(),
      offsets.data(), radial.stop_size(), mode, 0);
  ::skity::Matrix matrix(
      radial.gradient_transformer_[0], radial.gradient_transformer_[1], 0.0f,
      0.0f, radial.gradient_transformer_[2], radial.gradient_transformer_[3],
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, radial.gradient_transformer_[4],
      radial.gradient_transformer_[5], 0.0f, 1.0f);
  lgs->SetLocalMatrix(matrix);
  return lgs;
}

::skity::Paint SrSkityCanvas::ConvertToPaint(
    const SrSVGRenderState& render_state, ::skity::Rect bound, bool is_stroke) {
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
            paint.SetShader(
                ConvertToLinearGradientShader(ret_l->second, bound));
            break;
          }

          auto ret_r = rg_models_.find(sr_paint->content.iri);
          if (ret_r != rg_models_.end()) {
            paint.SetShader(
                ConvertToRadialGradientShader(ret_r->second, bound));
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
          (::skity::Paint::Cap)render_state.stroke_state->stroke_line_cap);
      paint.SetStrokeJoin(
          (::skity::Paint::Join)render_state.stroke_state->stroke_line_join);
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

  return paint;
}

#include <algorithm>
#include <string>
#include <vector>
std::string CleanInvalidNulls(const void* raw, size_t size_bytes) {
  const char* src = static_cast<const char*>(raw);
  std::string out;
  out.reserve(size_bytes);  // reserve capacity
  for (size_t i = 0; i < size_bytes; ++i) {
    if (src[i] != 0) {
      out.push_back(src[i]);
    }
  }
  return out;
}

#include <cstdio>

void DumpHeadHex(const void* raw, size_t size_bytes) {
  const uint8_t* b = static_cast<const uint8_t*>(raw);
  for (size_t i = 0; i < 500; ++i) {
    std::printf("%02X ", b[i]);
  }
  std::printf("\n");
}
std::shared_ptr<::skity::Data> SrSkityCanvas::GetSrSvgDrawImageWithData(
    std::shared_ptr<::skity::Data> data, float width, float height,
    SrSkityCanvas::ImageCallback image_callback) {
  if (!data || data->IsEmpty() || !data->RawData()) {
    return nullptr;
  }
  // Create a mutable copy of the data as a string, ensuring null-termination.
  // The parser requires a mutable buffer and relies on null-termination.
  //  std::string svg_string = Utf16LE_To_Utf8_NoBOM(data->RawData(), data->Size());
  //   std::string svg_string(reinterpret_cast<const char*>(data->RawData()),
  //   data->Size());

  DumpHeadHex(data->RawData(), data->Size());

  std::string svg_string = CleanInvalidNulls(data->RawData(), data->Size());

  // SrSVGDOM::make takes a const char* but casts it to char* internally to modify it during parsing.
  // We pass the string's buffer which is mutable.
  auto svg_dom = serval::svg::parser::SrSVGDOM::make(svg_string.c_str(),
                                                     svg_string.length());
  if (!svg_dom) {
    return nullptr;
  }
  ::skity::Bitmap bitmap(static_cast<uint32_t>(width),
                         static_cast<uint32_t>(height));
  auto canvas = ::skity::Canvas::MakeSoftwareCanvas(&bitmap);
  if (!canvas) {
    return nullptr;
  }
  SrSkityCanvas sr_canvas(canvas.get(), image_callback);
  SrSVGBox view_port{0.f, 0.f, width, height};
  svg_dom->Render(&sr_canvas, view_port);

  return ::skity::Data::MakeWithCopy(bitmap.GetPixelAddr(),
                                     bitmap.Height() * bitmap.RowBytes());
}

}  // namespace skity
}  // namespace svg
}  // namespace serval
