// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMarker.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "element/SrSVGPath.h"
#include "parser/SrSVGTraversalState.h"
#include "utils/SrSVGPatternUtils.h"

namespace serval::svg::element {
namespace {

constexpr double kPi = 3.14159265358979323846;
struct Point {
  double x{0};
  double y{0};
  Point operator-(Point other) const { return {x - other.x, y - other.y}; }
};
bool HasDirection(Point p) {
  return p.x != 0 || p.y != 0;
}
Point FirstDirection(std::initializer_list<Point> candidates) {
  for (auto p : candidates) {
    if (HasDirection(p)) {
      return p;
    }
  }
  return {};
}
double Direction(Point p) {
  return std::atan2(p.y, p.x);
}
struct Segment {
  Point start;
  Point end;
  Point outgoing;
  Point incoming;
};

// SVG endpoint-to-center conversion, used only for the endpoint tangents.
void ArcDirections(Point start, const float* a, Point* outgoing,
                   Point* incoming) {
  Point end{a[7], a[8]};
  *outgoing = *incoming = end - start;
  double rx = std::fabs(a[2]), ry = std::fabs(a[3]);
  if (rx == 0 || ry == 0 || !HasDirection(end - start)) {
    return;
  }
  double phi = a[4] * kPi / 180;
  double c = std::cos(phi), s = std::sin(phi);
  double dx = (start.x - end.x) / 2, dy = (start.y - end.y) / 2;
  double x = c * dx + s * dy, y = -s * dx + c * dy;
  double lambda = x * x / (rx * rx) + y * y / (ry * ry);
  if (lambda > 1) {
    rx *= std::sqrt(lambda);
    ry *= std::sqrt(lambda);
  }
  double denominator = rx * rx * y * y + ry * ry * x * x;
  double factor =
      std::sqrt(std::max(0.0, (rx * rx * ry * ry - denominator) / denominator));
  if ((a[5] != 0) == (a[6] != 0)) {
    factor = -factor;
  }
  double cx = factor * rx * y / ry, cy = -factor * ry * x / rx;
  double sign = a[6] != 0 ? 1 : -1;
  auto tangent = [&](double vx, double vy) -> Point {
    double tx = -rx * vy * sign, ty = ry * vx * sign;
    return {c * tx - s * ty, s * tx + c * ty};
  };
  *outgoing = tangent((x - cx) / rx, (y - cy) / ry);
  *incoming = tangent((-x - cx) / rx, (-y - cy) / ry);
}

void AppendSubpath(const std::vector<Segment>& segments, Point start,
                   bool closed, std::vector<SrSVGMarkerPosition>* positions) {
  if (segments.empty()) {
    positions->push_back(
        {static_cast<float>(start.x), static_cast<float>(start.y), 0.f});
    return;
  }
  const size_t n = segments.size();
  // Zero-length segments borrow a direction within the same subpath.
  std::vector<Point> incoming(n + 1), outgoing(n + 1);
  Point in, out;
  if (closed) {
    for (size_t i = n; i > 0; --i) {
      if (HasDirection(segments[i - 1].incoming)) {
        in = segments[i - 1].incoming;
        break;
      }
    }
    for (const auto& segment : segments) {
      if (HasDirection(segment.outgoing)) {
        out = segment.outgoing;
        break;
      }
    }
  }
  incoming[0] = in;
  outgoing[n] = out;
  for (size_t i = 0; i < n; ++i) {
    if (HasDirection(segments[i].incoming)) {
      in = segments[i].incoming;
    }
    incoming[i + 1] = in;
    const auto& segment = segments[n - 1 - i];
    if (HasDirection(segment.outgoing)) {
      out = segment.outgoing;
    }
    outgoing[n - 1 - i] = out;
  }
  for (size_t i = 0; i <= n; ++i) {
    Point p = i == 0 ? start : segments[i - 1].end;
    Point in = incoming[i], out = outgoing[i];
    double angle = Direction(FirstDirection({out, in}));
    if (HasDirection(in) && HasDirection(out)) {
      double a = Direction(in), b = Direction(out);
      // Average the angles along the shorter arc (including a 180 degree
      // reversal), rather than adding vectors whose sum may be zero.
      if (a - b > kPi) {
        b += 2 * kPi;
      } else if (b - a > kPi) {
        a += 2 * kPi;
      }
      angle = (a + b) / 2;
    }
    positions->push_back({static_cast<float>(p.x), static_cast<float>(p.y),
                          static_cast<float>(angle * 180 / kPi)});
  }
}

// Reuses each shape's AsPath geometry and length resolution, but records its
// equivalent path. No platform path flattening or backend-specific inspection
// is needed, and Bezier control points never become marker vertices.
// Primitive construction comes from the shared PathFactory defaults.
//
// TODO: Share the resolved SVG path commands between shape drawing and marker
// placement so both can consume them directly. Migrate one shape and backend
// at a time, checking vertex order, endpoint tangents, and rendered pixels.
class MarkerPathFactory final : public canvas::PathFactory {
 public:
  std::vector<uint8_t> ops;
  std::vector<float> args;
  std::unique_ptr<canvas::Path> CreatePath(uint8_t* commands, uint64_t count,
                                           float* values,
                                           uint64_t value_count) override {
    if (count && commands && (!value_count || values)) {
      ops.assign(commands, commands + count);
      if (value_count) {
        args.assign(values, values + value_count);
      }
    }
    return nullptr;
  }
  std::unique_ptr<canvas::Path> CreateMutable() override { return nullptr; }
  void Op(canvas::Path*, canvas::Path*, canvas::OP) override {}
  std::unique_ptr<canvas::Path> CreateStrokePath(const canvas::Path*, float,
                                                 SrSVGStrokeCap,
                                                 SrSVGStrokeJoin,
                                                 float) override {
    return nullptr;
  }
};

std::string Trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
SrSVGLength ParseReferencePoint(const char* value, bool horizontal) {
  if (std::strcmp(value, "center") == 0) {
    return {50.f, SR_SVG_UNITS_PERCENTAGE};
  }
  if (std::strcmp(value, horizontal ? "right" : "bottom") == 0) {
    return {100.f, SR_SVG_UNITS_PERCENTAGE};
  }
  if (std::strcmp(value, horizontal ? "left" : "top") == 0) {
    return {0.f, SR_SVG_UNITS_PERCENTAGE};
  }
  return make_serval_length(value);
}
}  // namespace

std::vector<SrSVGMarkerPosition> SrMarkerPositions(const SrPathData& path) {
  std::vector<SrSVGMarkerPosition> result;
  std::vector<Segment> segments;
  Point current, start;
  bool has_subpath = false, closed = false;
  size_t offset = 0;
  constexpr size_t counts[] = {2, 2, 6, 4, 9, 0};
  if (!path.ops || (path.n_args && !path.args)) {
    return result;
  }
  for (size_t i = 0; i < path.n_ops; ++i) {
    const uint8_t op = path.ops[i];
    if (op > SPO_CLOSE || offset + counts[op] > path.n_args) {
      break;
    }
    const float* a = path.args ? path.args + offset : nullptr;
    for (size_t j = 0; j < counts[op]; ++j) {
      if (!std::isfinite(a[j])) {
        return {};
      }
    }
    offset += counts[op];
    if (op == SPO_MOVE_TO) {
      if (has_subpath) {
        AppendSubpath(segments, start, closed, &result);
      }
      segments.clear();
      current = start = {a[0], a[1]};
      has_subpath = true;
      closed = false;
      continue;
    }
    if (!has_subpath) {
      return {};
    }
    // A drawing command after closepath starts at the closed subpath's start.
    if (closed && op != SPO_CLOSE) {
      AppendSubpath(segments, start, true, &result);
      segments.clear();
      start = current;
      closed = false;
    }
    Segment segment{current, current, {}, {}};
    switch (op) {
      case SPO_LINE_TO:
        segment.end = {a[0], a[1]};
        segment.outgoing = segment.incoming = segment.end - current;
        break;
      case SPO_CUBIC_BEZ: {
        Point c1{a[0], a[1]}, c2{a[2], a[3]};
        segment.end = {a[4], a[5]};
        segment.outgoing =
            FirstDirection({c1 - current, c2 - current, segment.end - current});
        segment.incoming = FirstDirection(
            {segment.end - c2, segment.end - c1, segment.end - current});
        break;
      }
      case SPO_QUAD_ARC: {
        Point control{a[0], a[1]};
        segment.end = {a[2], a[3]};
        segment.outgoing =
            FirstDirection({control - current, segment.end - current});
        segment.incoming =
            FirstDirection({segment.end - control, segment.end - current});
        break;
      }
      case SPO_ELLIPTICAL_ARC:
        segment.end = {a[7], a[8]};
        // Equal endpoints omit the arc entirely per SVG path implementation.
        if (!HasDirection(segment.end - current)) {
          continue;
        }
        ArcDirections(current, a, &segment.outgoing, &segment.incoming);
        break;
      case SPO_CLOSE:
        if (closed) {
          continue;
        }
        segment.end = start;
        segment.outgoing = segment.incoming = start - current;
        closed = true;
        break;
      default:
        return {};
    }
    current = segment.end;
    segments.push_back(segment);
  }
  if (has_subpath) {
    AppendSubpath(segments, start, closed, &result);
  }
  return result;
}

std::vector<SrSVGMarkerPosition> SrMarkerPositions(const SrSVGNodeBase& shape,
                                                   SrSVGRenderContext context) {
  if (shape.Tag() == SrSVGTag::kPath) {
    const auto* path = static_cast<const SrSVGPath&>(shape).path_data();
    return path ? SrMarkerPositions(*path) : std::vector<SrSVGMarkerPosition>{};
  }
  MarkerPathFactory factory;
  shape.AsPath(&factory, &context, false);
  SrPathData path{};
  path.ops = factory.ops.data();
  path.n_ops = static_cast<uint32_t>(factory.ops.size());
  path.args = factory.args.data();
  path.n_args = static_cast<uint32_t>(factory.args.size());
  return SrMarkerPositions(path);
}

bool SrSVGMarker::ParseReference(const char* value,
                                 std::optional<std::string>* reference) {
  std::string text = Trim(value ? value : "");
  if (text == "inherit" || text == "unset") {
    reference->reset();
    return true;
  }
  if (text == "none" || text == "initial") {
    *reference = "";
    return true;
  }
  if (text.size() < 6 || text.compare(0, 4, "url(") != 0 ||
      text.back() != ')') {
    return false;
  }
  text = Trim(text.substr(4, text.size() - 5));
  if (text.size() >= 2 && (text.front() == '\'' || text.front() == '"') &&
      text.front() == text.back()) {
    text = text.substr(1, text.size() - 2);
  }
  // External references are valid CSS but unsupported by this renderer.
  *reference = text.size() > 1 && text.front() == '#' ? text.substr(1) : "";
  return true;
}

bool SrSVGMarker::ParseAndSetAttribute(const char* name, const char* value) {
  if (std::strcmp(name, "markerWidth") == 0) {
    width_ = make_serval_length(value);
  } else if (std::strcmp(name, "markerHeight") == 0) {
    height_ = make_serval_length(value);
  } else if (std::strcmp(name, "refX") == 0) {
    ref_x_ = ParseReferencePoint(value, true);
  } else if (std::strcmp(name, "refY") == 0) {
    ref_y_ = ParseReferencePoint(value, false);
  } else if (std::strcmp(name, "markerUnits") == 0) {
    if (std::strcmp(value, "strokeWidth") != 0 &&
        std::strcmp(value, "userSpaceOnUse") != 0) {
      return false;
    }
    stroke_units_ = std::strcmp(value, "strokeWidth") == 0;
  } else if (std::strcmp(name, "viewBox") == 0) {
    has_view_box_ = value && value[0];
    view_box_ = make_serval_view_box(value);
  } else if (std::strcmp(name, "preserveAspectRatio") == 0) {
    aspect_ = make_preserve_aspect_radio(value);
  } else if (std::strcmp(name, "overflow") == 0) {
    clip_overflow_ = std::strcmp(value, "visible") != 0;
  } else if (std::strcmp(name, "orient") == 0) {
    std::string text = Trim(value);
    if (text == "auto" || text == "auto-start-reverse") {
      auto_orient_ = true;
      start_reverse_ = text == "auto-start-reverse";
    } else {
      char* end = nullptr;
      double angle = std::strtod(text.c_str(), &end);
      std::string unit = end ? end : "";
      if (end == text.c_str() || !std::isfinite(angle)) {
        return false;
      }
      // Reduce in the source unit before conversion so even finite angles
      // near the double limit cannot overflow into a non-finite transform.
      if (unit == "rad") {
        angle = std::fmod(angle, 2 * kPi) * (180 / kPi);
      } else if (unit == "grad") {
        angle = std::fmod(angle, 400) * 0.9;
      } else if (unit == "turn") {
        angle = std::fmod(angle, 1) * 360;
      } else if (!unit.empty() && unit != "deg") {
        return false;
      }
      auto_orient_ = start_reverse_ = false;
      angle_ = static_cast<float>(std::fmod(angle, 360));
    }
  } else {
    return SrSVGContainer::ParseAndSetAttribute(name, value);
  }
  return true;
}

void SrSVGMarker::RenderInstance(canvas::SrCanvas* canvas,
                                 SrSVGRenderContext context,
                                 const SrSVGMarkerPosition& position,
                                 bool is_start, const SrSVGRenderState& state,
                                 const StrokeTransform* stroke_transform) {
  float width = convert_serval_length_to_float(&width_, &context,
                                               SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float height = convert_serval_length_to_float(&height_, &context,
                                                SR_SVG_LENGTH_TYPE_VERTICAL);
  float scale = stroke_units_ ? state.stroke_width : 1.f;
  if (!std::isfinite(width) || !std::isfinite(height) ||
      !std::isfinite(scale) || width <= 0 || height <= 0 || scale <= 0) {
    return;
  }
  SrSVGBox viewport{0.f, 0.f, width, height};
  SrSVGBox box = has_view_box_ ? view_box_ : viewport;
  if (!std::isfinite(box.left) || !std::isfinite(box.top) ||
      !std::isfinite(box.width) || !std::isfinite(box.height) ||
      box.width <= 0 || box.height <= 0) {
    return;
  }
  context.view_port = viewport;
  context.view_box = box;
  context.width = width;
  context.height = height;
  float ref_x = convert_serval_length_to_float(&ref_x_, &context,
                                               SR_SVG_LENGTH_TYPE_HORIZONTAL);
  float ref_y = convert_serval_length_to_float(&ref_y_, &context,
                                               SR_SVG_LENGTH_TYPE_VERTICAL);
  if (ref_x_.unit == SR_SVG_UNITS_PERCENTAGE) {
    ref_x += box.left;
  }
  if (ref_y_.unit == SR_SVG_UNITS_PERCENTAGE) {
    ref_y += box.top;
  }
  float view_transform[6];
  calculate_view_box_transform(&viewport, &box, aspect_, view_transform);
  float mapped_x = view_transform[0] * ref_x + view_transform[4];
  float mapped_y = view_transform[3] * ref_y + view_transform[5];
  if (!std::isfinite(mapped_x) || !std::isfinite(mapped_y)) {
    return;
  }
  double angle = (auto_orient_ ? position.angle : angle_) * kPi / 180;
  if (auto_orient_ && start_reverse_ && is_start) {
    angle += kPi;
  }
  float c = static_cast<float>(std::cos(angle)) * scale;
  float s = static_cast<float>(std::sin(angle)) * scale;
  float transform[6]{c, s, -s, c, position.x, position.y};
  if (stroke_units_ && stroke_transform) {
    const auto& current = stroke_transform->current;
    const double vx =
        current[0] * std::cos(angle) + current[2] * std::sin(angle);
    const double vy =
        current[1] * std::cos(angle) + current[3] * std::sin(angle);
    const double device_angle = std::atan2(vy, vx);
    c = static_cast<float>(std::cos(device_angle)) * scale;
    s = static_cast<float>(std::sin(device_angle)) * scale;
    const float handedness =
        current[0] * current[3] - current[1] * current[2] < 0 ? -1.f : 1.f;
    float device[6]{c, s, -s * handedness, c * handedness, 0, 0};
    MapPoint(current, position.x, position.y, &device[4], &device[5]);
    ComposeAffineTransform(stroke_transform->inverse, device, transform);
  }
  canvas->Save();
  canvas->Transform(transform);
  canvas->Translate(-mapped_x, -mapped_y);
  if (clip_overflow_) {
    auto clip = canvas->PathFactory()->CreateRect(0, 0, 0, 0, width, height);
    if (clip) {
      canvas->ClipPath(clip.get(), SR_SVG_FILL);
    }
  }
  canvas->Transform(view_transform);
  SrSVGPaint default_fill{};
  default_fill.type = SERVAL_PAINT_COLOR;
  default_fill.content.color = {SERVAL_COLOR, NSVG_RGBA(0, 0, 0, 255)};
  SrSVGPaint default_stroke{};
  default_stroke.type = SERVAL_PAINT_NONE;
  SrSVGPaint context_fill = state.fill ? *state.fill : default_fill;
  SrSVGPaint context_stroke = state.stroke ? *state.stroke : default_stroke;
  // currentColor has already been computed on the context element. Marker
  // descendants must not recompute it using the definition's own color.
  if (context_fill.type == SERVAL_PAINT_COLOR) {
    context_fill.content.color.type = SERVAL_COLOR;
  }
  if (context_stroke.type == SERVAL_PAINT_COLOR) {
    context_stroke.content.color.type = SERVAL_COLOR;
  }
  context.context_fill = &context_fill;
  context.context_stroke = &context_stroke;
  // Deliberately render the marker's own computed style, preserving definition
  // ancestry rather than preparing it as a child of the referencing shape.
  Render(canvas, context);
  canvas->Restore();
}

SrSVGMarker::ResolvedMarkers SrSVGMarker::ResolveMarkers(
    const SrSVGNode& shape, const SrSVGRenderContext& context) {
  ResolvedMarkers markers;
  auto* ids = static_cast<const IDMapper*>(context.id_mapper);
  for (size_t i = 0; i < markers.nodes.size(); ++i) {
    const auto& reference =
        shape.markers_[i] ? shape.markers_[i] : shape.inherit_markers_[i];
    if (!reference || reference->empty()) {
      continue;
    }
    markers.has_references = true;
    if (!ids) {
      continue;
    }
    auto found = ids->find(*reference);
    if (found != ids->end() && found->second &&
        found->second->Tag() == SrSVGTag::kMarker) {
      markers.nodes[i] = static_cast<SrSVGMarker*>(found->second);
    }
  }
  return markers;
}

void SrSVGMarker::RenderMarkers(canvas::SrCanvas* canvas,
                                SrSVGRenderContext& context,
                                const SrSVGNode& shape,
                                const SrSVGRenderState& state,
                                const ResolvedMarkers& markers) {
  if (std::none_of(
          markers.nodes.begin(), markers.nodes.end(),
          [](const SrSVGMarker* marker) { return marker != nullptr; })) {
    return;
  }
  auto positions = SrMarkerPositions(shape, context);
  if (positions.empty()) {
    return;
  }
  StrokeTransform stroke_transform;
  const bool has_stroke_transform =
      state.vector_effect == SR_SVG_VECTOR_EFFECT_NON_SCALING_STROKE &&
      std::any_of(markers.nodes.begin(), markers.nodes.end(),
                  [](const SrSVGMarker* marker) {
                    return marker && marker->stroke_units_;
                  }) &&
      canvas->GetTransform(stroke_transform.current) &&
      InvertAffineTransform(stroke_transform.current, stroke_transform.inverse);
  // All instances use the context element's original computed state. A use
  // inside one instance may re-render the element and mutate its shared paints.
  SrSVGRenderState marker_state = state;
  SrSVGPaint fill{}, stroke{};
  if (state.fill) {
    fill = *state.fill;
    marker_state.fill = &fill;
  }
  if (state.stroke) {
    stroke = *state.stroke;
    marker_state.stroke = &stroke;
  }
  parser::SrSVGTraversalState local;
  SrSVGRenderContext marker_context = context;
  if (marker_state.opacity < 1.f) {
    marker_context.marker_fallback_opacity = &marker_state.opacity;
  }
  if (!marker_context.traversal_state) {
    marker_context.traversal_state = &local;
  }
  auto* traversal =
      static_cast<parser::SrSVGTraversalState*>(marker_context.traversal_state);
  auto draw = [&](size_t kind, size_t index) {
    auto* marker = markers.nodes[kind];
    if (!marker) {
      return;
    }
    if (!traversal->active_markers.insert(marker).second) {
      return;
    }
    marker->RenderInstance(canvas, marker_context, positions[index], kind == 0,
                           marker_state,
                           has_stroke_transform ? &stroke_transform : nullptr);
    traversal->active_markers.erase(marker);
  };
  draw(0, 0);
  for (size_t i = 1; i + 1 < positions.size(); ++i) {
    draw(1, i);
  }
  draw(2, positions.size() - 1);
  canvas->SetRenderContext(&context);
}

}  // namespace serval::svg::element
